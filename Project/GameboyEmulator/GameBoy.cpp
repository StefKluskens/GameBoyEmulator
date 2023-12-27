#include "pch.h"
#include "GameBoy.h"
#include <fstream>
#include <time.h>
#include <iostream>
#include "MBC0.h"
#include "MBC1.h"


GameBoy::GameBoy( const std::string &gameFile ): GameBoy{}
{
	LoadGame( gameFile );
}


void GameBoy::LoadGame( const std::string &gbFile ) {
	std::ifstream file{ gbFile, std::ios::binary };
	assert( file.good() );

	file.seekg( 0, std::ios::end );
	const std::ifstream::pos_type size{ file.tellg() };
	Rom.resize( size, 0 );

	file.seekg( 0, std::ios::beg );
	file.read( (char*)Rom.data(), size );

	file.close();

	const GameHeader header{ ReadHeader() };
	Mbc = header.mbc;

	uint8_t banksNeeded{};
	switch (header.ramSizeValue) {
		case 0x00:
			if (Mbc == mbc2)
			{
				banksNeeded = 1; //256 bytes
			}
			break;
		case 0x01:
		case 0x02:
			banksNeeded = 1;
			break;
		case 0x03:
			banksNeeded = 4;

	}
	RamBanks.resize( banksNeeded * 0x2000, 0 );

	if (m_MBC)
	{
		m_MBC->SetRamBanks(RamBanks);
	}

	Cpu.Reset();
}

void GameBoy::Update() {
	/*If we increase the allowed ticks per update, we'll experience "time jumps" the screen will be drawn slower than the changes are performed
	 * Potentially, for tetris, the game starts and the next frame it's over..
	 */

	const float fps{ 59.73f };
	const float timeAdded{ 1000 / fps };
	bool idle{};

	clock_t lastValidTick{ clock() / (CLOCKS_PER_SEC / 1000) };

	while (IsRunning) 
	{
		const clock_t currentTicks = { clock() / (CLOCKS_PER_SEC / 1000) };

		if (!IsPaused && (lastValidTick + timeAdded) < currentTicks) //I'm keeping fps in mind to ensure a smooth simulation, if we make the cyclebudget infinite, we have 0 control
		{	
			if (AutoSpeed && (currentTicks - (lastValidTick + timeAdded)) >= .5f && SpeedMultiplier >= 2) 
			{
				--SpeedMultiplier;
			}
			CurrentCycles = 0;

			const unsigned int cycleBudget{ unsigned int( ceil( 4194304.0f / fps ) ) * SpeedMultiplier }; //4194304 cycles can be done in a second (standard gameboy)
			while (!IsPaused && CurrentCycles < cycleBudget) 
			{
				unsigned int stepCycles{ CurrentCycles };
				Cpu.ExecuteNextOpcode();
				stepCycles = CurrentCycles - stepCycles;

				if ((IsCycleFrameBound & 2) && (CyclesFramesToRun - stepCycles > CyclesFramesToRun || !(CyclesFramesToRun -= stepCycles))) //If we're cycle bound, subtract cycles and call pause if needed
				{
					(IsCycleFrameBound = 0, IsPaused = true);
				}

				HandleTimers( stepCycles, cycleBudget );
				Cpu.HandleGraphics( stepCycles, cycleBudget,
				                    (!OnlyDrawLast || !(IsCycleFrameBound & 1) || (IsCycleFrameBound & 1 && CyclesFramesToRun == 1)
				                    ) ); //Draw if we don't care, are not frame bound or are on the final frame

				if ((IF & 1) && (IsCycleFrameBound & 1) && !(--CyclesFramesToRun)) //If vblank interrupt and we're frame bound, subtract frames and call pause if needed
				{
					(IsCycleFrameBound = 0, IsPaused = true);
				}

				Cpu.HandleInterupts();
			}
			lastValidTick = currentTicks;
			idle = false;

		} else if (!idle) {
			if (AutoSpeed && ((lastValidTick + timeAdded) - currentTicks) >= .5f) {
				++SpeedMultiplier;
			}
			idle = true;
		}
	}
}

GameHeader GameBoy::ReadHeader() {
	GameHeader header{};
	memcpy( header.title, (void*)(Rom.data() + 0x134), 16 );

	switch (Rom[0x147]) { //Set MBC chip version
		case 0x00:
			std::cout << "No MBC, using MBC0 class\n";
			header.mbc = none;
			m_MBC = new MBC0(Rom);
			break;
		case 0x01:
		case 0x02:
		case 0x03:
			//std::cout << "Using MBC1 class\n";
			header.mbc = mbc1;
			m_MBC = new MBC1(Rom);
			break;
		case 0x05:
		case 0x06:
			header.mbc = mbc2;
			break;
		case 0x0f:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			header.mbc = mbc3;
			break;
		case 0x15:
		case 0x16:
		case 0x17:
			header.mbc = mbc4;
			break;
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
			header.mbc = mbc5;
			break;
		default:
			assert( Rom[0x147] == 0x0 ); //if not 0x0 it's undocumented
	}

	header.ramSizeValue = Rom[0x0149];

	return header;
}

uint8_t GameBoy::ReadMemory( const uint16_t pos ) 
{
	/*if (pos >= 0x4000 && pos <= 0x7FFF)
	{
		if (m_MBC)
		{
			uint16_t newAddress = pos;
			newAddress += ((m_MBC->GetRomBank() - 1) * 0x4000);
			return Rom[newAddress];
		}
		
	}
	else if (pos >= 0xA000 && pos <= 0xBFFF)
	{
		if (m_MBC)
		{
			uint16_t newAddress = pos - 0xA000;
			return m_MBC->GetRamBanks().at(m_MBC->GetRamBank())[&newAddress];
		}
	}
	else if (pos == 0xFF00)
	{
		return GetJoypadState();
	}

	return Memory[pos];*/


	if (pos == 0xFF00)
	{
		return GetJoypadState();
	}
	else if (pos == 0xFF04)
	{
		return DIVTimer;
	}
	else if (pos == 0xFF05)
	{
		return TIMATimer;
	}
	else if (pos == 0xFF06)
	{
		return TMATimer;
	}
	else if (pos == 0xFF07)
	{
		return TACTimer;
	}
	else if (pos == 0xFF0F)
	{
		return Memory[0xFF0F];
	}
	/*else if (pos < 0x100)
	{
		return Memory[pos];
	}*/
	else if (pos < 0x8000 || (pos >= 0xA000 && pos <= 0xBFFF))
	{
		if (m_MBC)
		{
			return m_MBC->ReadByte(pos, Memory);
		}
	}

	

	/*if (Memory[0xff02] == 0x81)
	{
		char c = Memory[0xff01];
		printf("%c", c);
		Memory[0xff02] = 0x0;
	}*/

	

	return Memory[pos];
}

void GameBoy::WriteMemory( uint16_t address, uint8_t data ) 
{
	if ((address >= 0x0000 && address <= 0x7FFF) || (address >= 0xA000 && address < 0xBFFF))
	{
		if (m_MBC)
		{
			m_MBC->WriteByte(address, data, Memory);
		}
	}
	

	if (InRange( address, (uint16_t)0xC000, (uint16_t)0xDFFF )) //Internal RAM
	{ 
		Memory[address] = data;

	} 
	else if (InRange( address, (uint16_t)0xE000, (uint16_t)0xFDFF )) //ECHO RAM
	{ 
		Memory[address] = data;
		Memory[address - 0x2000] = data;

	} 
	else if (InRange( address, (uint16_t)0xFEA0, (uint16_t)0xFEFF )) //Mysterious Restricted Range
	{ 

	} 
	else if (address == 0xFF04) //Reset DIV
	{ 
		DIVTimer = 0;
		DivCycles = 0;

	} 
	else if (address == 0xFF07) //Set timer Clock speed
	{ 
		TACTimer = data & 0x7;

	} 
	else if (address == 0xFF44) //Horizontal scanline reset
	{ 
		Memory[address] = 0;

	}
	else if (address == 0xFF45)
	{
		Memory[address] = data;
	}
	else if (address == 0xFF46) //DMA transfer
	{ 
		const uint16_t src{ uint16_t( uint16_t( data ) << 8 ) };
		for (int i{ 0 }; i < 0xA0; ++i) 
		{
			//WriteMemory( 0xFE00 + i, ReadMemory( src + i ) );
			Memory[0xFE00 + i] = ReadMemory(src + i);
		}

	} 
	else 
	{
		Memory[address] = data;
	}


	if (address == 0xFF40)
	{
		if (!(data & 0x80))
		{
			Memory[0xFF41] &= 0x7C;
			Memory[0xFF44] &= 0x00;
		}
	}
}

void GameBoy::WriteMemoryWord( const uint16_t pos, const uint16_t value ) {
	WriteMemory( pos, value & 0xFF );
	WriteMemory(pos + 1, (value >> 8) & 0xFF);
}

void GameBoy::PushWordOntoStack(uint16_t& sp, uint16_t value)
{
	uint8_t hi = value >> 8;
	uint8_t lo = value & 0xFF;
	sp--;
	WriteMemory(sp, hi);
	sp--;
	WriteMemory(sp, lo);
}

//void GameBoy::RequestInterrupt(uint8_t bit)
//{
//	uint8_t newIF = IF | 0x10;
//	WriteMemory(0xFF0F, newIF);
//}

void GameBoy::RequestInterrupt(Interupts bit)
{
	IF |= (1 << bit);
}

void GameBoy::SetKey( const Key key, const bool pressed ) {
	if (pressed) 
	{
		const uint8_t oldJoyPad{ JoyPadState };
		JoyPadState &= ~(1 << key);

		if ((oldJoyPad & (1 << key))) //Previosuly 1
		{ 
			if (!(Memory[0xff00] & 0x20) && !(key + 1 % 2)) //Button Keys
			{
				RequestInterrupt(joypad);
				/*uint8_t newIF = IF | 0x10;
				WriteMemory(0xFF0F, newIF);*/
			}
			else if (!(Memory[0xff00] & 0x10) && !(key % 2)) //Directional keys
			{
				RequestInterrupt(joypad);
				/*uint8_t newIF = IF | 0x10;
				WriteMemory(0xFF0F, newIF);*/
			}
		}
	} 
	else
	{
		JoyPadState |= (1 << key);
	}
}

void GameBoy::HandleTimers( const unsigned stepCycles, const unsigned cycleBudget ) {// This function may be placed under the cpu class...
	assert( stepCycles<=0xFF );//if this never breaks, which I highly doubt it will, change the type to uint8

	const unsigned cyclesOneDiv{ (cycleBudget / 16384) };
	if ((DivCycles += stepCycles) >= cyclesOneDiv) { //TODO: turn this into a while loop
		DivCycles -= cyclesOneDiv;
		++DIVTimer;
	}

	if (TACTimer & 0x4) {
		TIMACycles += stepCycles;

		unsigned int threshold;
		switch (TACTimer & 0x3) {
			case 0:
				threshold = cycleBudget / 4096;
				break;
			case 1:
				threshold = cycleBudget / 262144;
				break;
			case 2:
				threshold = cycleBudget / 65536;
				break;
			case 3:
				threshold = cycleBudget / 16384;
				break;
			default:
				assert( true );
		}

		while (TIMACycles >= threshold) 
		{
			if (!++TIMATimer) 
			{
				TIMATimer = TMATimer;
				IF |= 0x4;
			}
			TIMACycles -= threshold;
		}
	}
}

uint8_t GameBoy::GetJoypadState() const {
	uint8_t res{ Memory[0xff00] };

	if (!(res & 0x20)) { //Button keys
		res |= !!(JoyPadState & (1 << aButton));
		res |= !!(JoyPadState & (1 << bButton)) << 1;
		res |= !!(JoyPadState & (1 << select)) << 2;
		res |= !!(JoyPadState & (1 << start)) << 3;
	} else if (!(res & 0x10)) {
		res |= !!(JoyPadState & (1 << right));
		res |= !!(JoyPadState & (1 << left)) << 1;
		res |= !!(JoyPadState & (1 << up)) << 2;
		res |= !!(JoyPadState & (1 << down)) << 3;
	}
	return res;
}