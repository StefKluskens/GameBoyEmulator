#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
#include "OpcodeDefinitions.cpp" //Inlines..
#ifdef _DEBUG
//#define VERBOSE
#endif
#include <iostream>


LR35902::LR35902( GameBoy &gameboy ): Gameboy{ gameboy } {
	//TODO: Investigate Threading Bottleneck
	/*for (uint8_t i{ 0 }; i < 10; ++i) {
		DrawThreads[i] = std::thread{&LR35902::ThreadWork, this, i, &DrawDataStruct};
	}*/
}

void LR35902::Reset(const bool skipBoot) {
	//http://bgb.bircd.org/pandocs.htm#powerupsequence

	if (skipBoot) {
		//The state after executing the Boot rom
		Register.af(0x01b0);
		Register.bc(0x0013);
		Register.de(0x00d8);
		Register.hl(0x014D);

		Register.pc = 0x100; //First instruction after boot rom;
		Register.sp = 0xFFFE;

		Register.zeroF = true;
		Register.halfCarryF = true;
		Register.carryF = true;
		Register.subtractF = false;

		m_Halted = false;
		m_ime = false;

		//io and hram state
		uint8_t* memory{ Gameboy.GetRawMemory() };
		memory[0xFF05] = 0x00;
		memory[0xFF06] = 0x00;
		memory[0xFF07] = 0x00;
		memory[0xFF10] = 0x80;
		memory[0xFF11] = 0xBF;
		memory[0xFF12] = 0xF3;
		memory[0xFF14] = 0xBF;
		memory[0xFF16] = 0x3F;
		memory[0xFF17] = 0x00;
		memory[0xFF19] = 0xBF;
		memory[0xFF1A] = 0x7F;
		memory[0xFF1B] = 0xFF;
		memory[0xFF1C] = 0x9F;
		memory[0xFF1E] = 0xBF;
		memory[0xFF20] = 0xFF;
		memory[0xFF21] = 0x00;
		memory[0xFF22] = 0x00;
		memory[0xFF23] = 0xBF;
		memory[0xFF24] = 0x77;
		memory[0xFF25] = 0xF3;
		memory[0xFF26] = 0xF1;
		memory[0xFF40] = 0x91;
		memory[0xFF42] = 0x00;
		memory[0xFF43] = 0x00;
		memory[0xFF45] = 0x00;
		memory[0xFF47] = 0xFC;
		memory[0xFF48] = 0xFF;
		memory[0xFF49] = 0xFF;
		memory[0xFF4A] = 0x00;
		memory[0xFF4B] = 0x00;
		memory[0xFFFF] = 0x00;
	}
}

void LR35902::ExecuteNextOpcode() 
{
	

#ifdef VERBOSE
	static bool ok{};
	if(ok) {
		static std::ofstream file{"log.txt",std::ios::trunc};
		file << std::to_string(Register.pc) << ", " << std::hex << '[' << LookUp[opcode]<<']'<< int(opcode) <<", " << std::to_string(Register.f)<< ", "<<
																														std::to_string( Register.a )+", "<<
																														std::to_string( Register.b )+", "<<
																														std::to_string( Register.c )+", "<<
																														std::to_string( Register.d )+", "<<
																														std::to_string( Register.e )+", "<<
																														std::to_string( Register.h )+", "<<
																														std::to_string( Register.l )+", "<<
																														"IF:"+std::to_string( Gameboy.GetIF() )+", "<<
																														"SP:"+std::to_string( Gameboy.ReadMemory( Register.sp ) )+':'+std::to_string( Gameboy.ReadMemory( Register.sp+1 ) )<< std::endl;

	}
#endif
	const uint8_t opcode{ Gameboy.ReadMemory(Register.pc++) };
	ExecuteOpcode(opcode);

	if (InteruptChangePending) 
	{
		if ((*(uint8_t*)&InteruptChangePending & 8) && Gameboy.ReadMemory( Register.pc - 1 ) != 0xFB) 
		{
			InteruptsEnabled = true;
			InteruptChangePending = false;
		} 
		else if ((*(uint8_t*)&InteruptChangePending & 1) && Gameboy.ReadMemory( Register.pc - 1 ) != 0xF3) 
		{
			InteruptsEnabled = false;
			InteruptChangePending = false;
		}
	}
}

void LR35902::HandleInterupts() 
{
	const uint8_t ints{ uint8_t( Gameboy.GetIF() & Gameboy.GetIE() ) };

	if (InteruptsEnabled && ints) 
	{
		for (int bit{ 0 }; bit < 5; ++bit) 
		{
			if ((ints >> bit) & 0x1) 
			{
				Gameboy.WriteMemoryWord( Register.sp -= 2, Register.pc );
				switch (bit) {
					case 0: 
						Register.pc = 0x40;
						break;//V-Blank
					case 1: 
						Register.pc = 0x48;
						break;//LCD State
					case 2: 
						Register.pc = 0x50;
						//m_Halted = false;
						break;//Timer
					case 3: 
						Register.pc = 0x58;
						break;//Serial
					case 4: 
						Register.pc = 0x60;
						break;//Joypad
				}

				InteruptsEnabled = false;
				Gameboy.GetIF() &= ~(1 << bit);
				break;
			}
		}
	}
}

void LR35902::HandleGraphics( const unsigned cycles, const unsigned cycleBudget, const bool draw ) noexcept 
{
	const unsigned cyclesOneDraw{ 456 };
	ConfigureLCDStatus(); //This is why we can't "speed this up" in the traditional sense, games are sensitive to this

	if (Gameboy.GetLY() > 153) 
	{
		Gameboy.GetLY() = 0;
	}

	if ((Gameboy.GetLCDC() & 0x80)) 
	{
		if ((LCDCycles += cycles) >= cyclesOneDraw) 
		{ //LCD enabled and we're at our cycle mark
			LCDCycles = 0;
			uint8_t dirtyLY;

			if ((dirtyLY = ++Gameboy.GetLY()) == 144) 
			{
				Gameboy.RequestInterrupt( vBlank );

				/*uint8_t newIF = Gameboy.GetIF() | 0x01;
				Gameboy.WriteMemory(0xFF0F, newIF);*/
			}

			if (Gameboy.GetLY() > 153)
			{
				Gameboy.GetLY() = 0;
			}

			if (dirtyLY < 144 && draw) 
			{
				DrawLine();
			}
		}
	}
}


//Yes, I know, visibility/debugging will suffer...
//Tho, once the macro has been proven to work, it works...
//Also, due to MSVC no longer ignoring trailing commas(VS2019) and refusing to parse ## or the _opt_ semantic, you have to use 2 commas when using the variadic part.. :/
//Note: These multi-line macros might be regarded as unsafe but I don't wanna risk branching so ima leave it like this, yes it's likely the compiler will optimize the if statement out but im not gonna risk it..
//#define OPCYCLE(func, cycl) func; cycles = cycl; break
//#define BASICOPS(A, B, C, D, E, H, L, cycles, funcName, ...) \
//	case A: OPCYCLE( funcName( Register.a __VA_ARGS__), cycles ); \
//	case B: OPCYCLE( funcName( Register.b __VA_ARGS__), cycles ); \
//	case C: OPCYCLE( funcName( Register.c __VA_ARGS__), cycles ); \
//	case D: OPCYCLE( funcName( Register.d __VA_ARGS__), cycles ); \
//	case E: OPCYCLE( funcName( Register.e __VA_ARGS__), cycles ); \
//	case H: OPCYCLE( funcName( Register.h __VA_ARGS__), cycles ); \
//	case L: OPCYCLE( funcName( Register.l __VA_ARGS__), cycles )

void LR35902::ExecuteOpcode( uint8_t opcode ) 
{
	assert( Gameboy.ReadMemory(Register.pc-1) == opcode ); //pc is pointing to first argument
	uint8_t cycles{};

	switch (opcode) {
		case 0x00:
			cycles = 4;
			break;
		case 0x01:
		{
			uint16_t temp{ Register.bc() };
			LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
			cycles = 12;
			Register.bc(temp);
			break;
		}
		case 0x02:
			LD(Register.bc(), Register.a);
			cycles = 8;
			break;
		case 0x03:
		{
			uint16_t temp{ Register.bc() };
			INC( temp );
			cycles = 8;
			Register.bc( temp );
			break;
		}
		case 0x04:
			INC(Register.b);
			cycles = 4;
			break;
		case 0x05:
			DEC(Register.b);
			cycles = 4;
			break;
		case 0x06:
			LD(Register.b, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x07:
			RLC(Register.a);
			cycles = 8;
			break;
		case 0x08: 
		{
			Gameboy.WriteMemoryWord( Gameboy.ReadMemoryWord( Register.pc ), Register.sp );
			cycles = 20;
			break;
		}
		case 0x09:
		{
			ADD16(true, Register.bc());
			cycles = 8;
			break;
		}
		case 0x0A:
			LD(Register.a, Gameboy.ReadMemory(Register.bc()));
			cycles = 8;
			break;
		case 0x0B:
		{
			uint16_t temp{ Register.bc() };
			DEC(temp);
			Register.bc(temp);
			cycles = 8;
			break;
		}
		case 0x0C:
			INC(Register.c);
			cycles = 4;
			break;
		case 0x0D:
			DEC(Register.c);
			cycles = 4;
			break;
		case 0x0E:
			LD(Register.c, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x0F:
			RR(Register.a);
			cycles = 4;
			break;
		case 0x10:
			//STOP();
			Register.pc++;
			cycles = 4;
			break;
		case 0x11:
		{
			uint16_t temp{ Register.de() };
			LD( &temp, Gameboy.ReadMemoryWord( Register.pc ) );
			cycles = 12;
			Register.de( temp );
			break;
		}
		case 0x12:
			LD(Register.de(), Register.a);
			cycles = 8;
			break;
		case 0x13:
		{
			uint16_t temp{ Register.de() };
			INC(temp);
			Register.de(temp);
			cycles = 8;
			break;
		}
		case 0x14:
			INC(Register.d);
			cycles = 4;
			break;
		case 0x15:
			DEC(Register.d);
			cycles = 4;
			break;
		case 0x16:
			LD(Register.d, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x17:
			RL(Register.a);
			cycles = 4;
			break;
		case 0x18:
			//Handles cycles internally
			JR(Gameboy.ReadMemory(Register.pc++), true);
			break;
		case 0x19:
		{
			ADD16(true, Register.de());
			cycles = 8;
			break;
		}
		case 0x1A:
			LD(Register.a, Gameboy.ReadMemory(Register.de()));
			cycles = 8;
			break;
		case 0x1B:
		{
			uint16_t temp{ Register.de() };
			DEC(temp);
			Register.de(temp);
			cycles = 8;
			break;
		}
		case 0x1C:
			INC(Register.e);
			cycles = 4;
			break;
		case 0x1D:
			DEC(Register.e);
			cycles = 4;
			break;
		case 0x1E:
			LD(Register.e, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x1F:
			RR(Register.a);
			cycles = 4;
			break;
		case 0x20:
			JR(Gameboy.ReadMemory(Register.pc++), !Register.zeroF);
			break;
		case 0x21:
		{
			uint16_t temp{ Register.hl() };
			LD(&temp, Gameboy.ReadMemoryWord(Register.pc));
			cycles = 12;
			Register.hl(temp);
			break;
		}
		case 0x22:
			LD(Register.hl(), Register.a);
			Register.hl(Register.hl() + 1);
			cycles = 8;
			break;
		case 0x23:
		{
			uint16_t temp{ Register.hl() };
			INC(temp);
			Register.hl(temp);
			cycles = 8;
			break;
		}
		case 0x24:
			INC(Register.h);
			cycles = 4;
			break;
		case 0x25:
			DEC(Register.h);
			cycles = 4;
			break;
		case 0x26:
			LD(Register.h, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x27:
		{
			DAA();
			cycles = 4;
			break;
		}
		case 0x28:
			JR(Gameboy.ReadMemory(Register.pc++), Register.zeroF);
			break;
		case 0x29:
		{
			ADD16(true, Register.hl());
			cycles = 8;
			break;
		}
		case 0x2A:
			LD(Register.a, Gameboy.ReadMemory(Register.hl()));
			Register.hl(Register.hl() + 1);
			cycles = 8;
			break;
		case 0x2B:
		{
			uint16_t temp{ Register.hl() };
			DEC(temp);
			Register.hl(temp);
			cycles = 8;
			break;
		}
		case 0x2C:
			INC(Register.l);
			cycles = 4;
			break;
		case 0x2D:
			DEC(Register.l);
			cycles = 4;
			break;
		case 0x2E:
			LD(Register.l, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x2F:
			CPL();
			cycles = 4;
			break;
		case 0x30:
			JR(Gameboy.ReadMemory(Register.pc++), !Register.carryF);
			break;
		case 0x31:
			LD(&Register.sp, Gameboy.ReadMemoryWord(Register.pc));
			cycles = 12;
			break;
		case 0x32:
			LD(Register.hl(), Register.a);
			Register.hl(Register.hl() - 1);
			cycles = 8;
			break;
		case 0x33:
			INC(Register.sp);
			cycles = 8;
			break;
		case 0x34:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			INC(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 12;
			break;
		}
		case 0x35:
		{
			uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
			DEC(hlDeRef);
			Gameboy.WriteMemory(Register.hl(), hlDeRef);
			cycles = 12;
			break;
		}
		case 0x36:
			LD(Register.hl(), Gameboy.ReadMemory(Register.pc++));
			cycles = 12;
			break;
		case 0x37:
			SCF();
			cycles = 4;
			break;
		case 0x38:
			JR(Gameboy.ReadMemory(Register.pc++), Register.carryF);
			break;
		case 0x39:
		{
			ADD16(true, Register.sp);
			cycles = 8;
			break;
		}
		case 0x3A:
		{
			Register.a = Gameboy.ReadMemory(Register.hl());
			Register.hl(Register.hl() - 1);
			cycles = 8;
			break;
		}
		case 0x3B:
			DEC(Register.sp);
			cycles = 8;
			break;
		case 0x3C:
			INC(Register.a);
			cycles = 4;
			break;
		case 0x3D:
			DEC(Register.a);
			cycles = 4;
			break;
		case 0x3E:
			LD(Register.a, Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0x3F:
			CCF();
			cycles = 4;
			break;
		case 0x40:
			LD(Register.b, Register.b);
			cycles = 4;
			break;
		case 0x41:
			LD(Register.b, Register.c);
			cycles = 4;
			break;
		case 0x42:
			LD(Register.b, Register.d);
			cycles = 4;
			break;
		case 0x43:
			LD(Register.b, Register.e);
			cycles = 4;
			break;
		case 0x44:
			LD(Register.b, Register.h);
			cycles = 4;
			break;
		case 0x45:
			LD(Register.b, Register.l);
			cycles = 4;
			break;
		case 0x46:
			LD(Register.b, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x47:
			LD(Register.b, Register.a);
			cycles = 4;
			break;
		case 0x48:
			LD(Register.c, Register.b);
			cycles = 4;
			break;
		case 0x49:
			LD(Register.c, Register.c);
			cycles = 4;
			break;
		case 0x4A:
			LD(Register.c, Register.d);
			cycles = 4;
			break;
		case 0x4B:
			LD(Register.c, Register.e);
			cycles = 4;
			break;
		case 0x4C:
			LD(Register.c, Register.h);
			cycles = 4;
			break;
		case 0x4D:
			LD(Register.c, Register.l);
			cycles = 4;
			break;
		case 0x4E:
			LD(Register.c, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x4F:
			LD(Register.c, Register.a);
			cycles = 4;
			break;
		case 0x50:
			LD(Register.d, Register.b);
			cycles = 4;
			break;
		case 0x51:
			LD(Register.d, Register.c);
			cycles = 4;
			break;
		case 0x52:
			LD(Register.d, Register.d);
			cycles = 4;
			break;
		case 0x53:
			LD(Register.d, Register.e);
			cycles = 4;
			break;
		case 0x54:
			LD(Register.d, Register.h);
			cycles = 4;
			break;
		case 0x55:
			LD(Register.d, Register.l);
			cycles = 4;
			break;
		case 0x56:
			LD(Register.d, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x57:
			LD(Register.d, Register.a);
			cycles = 4;
			break;
		case 0x58:
			LD(Register.e, Register.b);
			cycles = 4;
			break;
		case 0x59:
			LD(Register.e, Register.c);
			cycles = 4;
			break;
		case 0x5A:
			LD(Register.e, Register.d);
			cycles = 4;
			break;
		case 0x5B:
			LD(Register.e, Register.e);
			cycles = 4;
			break;
		case 0x5C:
			LD(Register.e, Register.h);
			cycles = 4;
			break;
		case 0x5D:
			LD(Register.e, Register.l);
			cycles = 4;
			break;
		case 0x5E:
			LD(Register.e, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x5F:
			LD(Register.e, Register.a);
			cycles = 4;
			break;
		case 0x60:
			LD(Register.h, Register.b);
			cycles = 4;
			break;
		case 0x61:
			LD(Register.h, Register.c);
			cycles = 4;
			break;
		case 0x62:
			LD(Register.h, Register.d);
			cycles = 4;
			break;
		case 0x63:
			LD(Register.h, Register.e);
			cycles = 4;
			break;
		case 0x64:
			LD(Register.h, Register.h);
			cycles = 4;
			break;
		case 0x65:
			LD(Register.h, Register.l);
			cycles = 4;
			break;
		case 0x66:
			LD(Register.h, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x67:
			LD(Register.h, Register.a);
			cycles = 4;
			break;
		case 0x68:
			LD(Register.l, Register.b);
			cycles = 4;
			break;
		case 0x69:
			LD(Register.l, Register.c);
			cycles = 4;
			break;
		case 0x6A:
			LD(Register.l, Register.d);
			cycles = 4;
			break;
		case 0x6B:
			LD(Register.l, Register.e);
			cycles = 4;
			break;
		case 0x6C:
			LD(Register.l, Register.h);
			cycles = 4;
			break;
		case 0x6D:
			LD(Register.l, Register.l);
			cycles = 4;
			break;
		case 0x6E:
			LD(Register.l, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x6F:
			LD(Register.l, Register.a);
			cycles = 4;
			break;
		case 0x70:
			LD(Register.hl(), Register.b);
			cycles = 8;
			break;
		case 0x71:
			LD(Register.hl(), Register.c);
			cycles = 8;
			break;
		case 0x72:
			LD(Register.hl(), Register.d);
			cycles = 8;
			break;
		case 0x73:
			LD(Register.hl(), Register.e);
			cycles = 8;
			break;
		case 0x74:
			LD(Register.hl(), Register.h);
			cycles = 8;
			break;
		case 0x75:
			LD(Register.hl(), Register.l);
			cycles = 8;
			break;
		case 0x76:
			HALT();
			cycles = 4;
			break;
		case 0x77:
			LD(Register.hl(), Register.a);
			cycles = 8;
			break;
		case 0x78:
			LD(Register.a, Register.b);
			cycles = 4;
			break;
		case 0x79:
			LD(Register.a, Register.c);
			cycles = 4;
			break;
		case 0x7A:
			LD(Register.a, Register.d);
			cycles = 4;
			break;
		case 0x7B:
			LD(Register.a, Register.e);
			cycles = 4;
			break;
		case 0x7C:
			LD(Register.a, Register.h);
			cycles = 4;
			break;
		case 0x7D:
			LD(Register.a, Register.l);
			cycles = 4;
			break;
		case 0x7E:
			LD(Register.a, Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x7F:
			LD(Register.a, Register.a);
			cycles = 4;
			break;
		case 0x80:
			ADD(Register.b);
			cycles = 4;
			break;
		case 0x81:
			ADD(Register.c);
			cycles = 4;
			break;
		case 0x82:
			ADD(Register.d);
			cycles = 4;
			break;
		case 0x83:
			ADD(Register.e);
			cycles = 4;
			break;
		case 0x84:
			ADD(Register.h);
			cycles = 4;
			break;
		case 0x85:
			ADD(Register.l);
			cycles = 4;
			break;
		case 0x86:
			ADD(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x87:
			ADD(Register.a);
			cycles = 4;
			break;
		case 0x88:
			ADC(Register.b);
			cycles = 4;
			break;
		case 0x89:
			ADC(Register.c);
			cycles = 4;
			break;
		case 0x8A:
			ADC(Register.d);
			cycles = 4;
			break;
		case 0x8B:
			ADC(Register.e);
			cycles = 4;
			break;
		case 0x8C:
			ADC(Register.h);
			cycles = 4;
			break;
		case 0x8D:
			ADC(Register.l);
			cycles = 4;
			break;
		case 0x8E:
			ADC(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x8F:
			ADC(Register.a);
			cycles = 4;
			break;
		case 0x90:
			SUB(Register.b);
			cycles = 4;
			break;
		case 0x91:
			SUB(Register.c);
			cycles = 4;
			break;
		case 0x92:
			SUB(Register.d);
			cycles = 4;
			break;
		case 0x93:
			SUB(Register.e);
			cycles = 4;
			break;
		case 0x94:
			SUB(Register.h);
			cycles = 4;
			break;
		case 0x95:
			SUB(Register.l);
			cycles = 4;
			break;
		case 0x96:
			SUB(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x97:
			SUB(Register.a);
			cycles = 4;
			break;
		case 0x98:
			SBC(Register.b);
			cycles = 4;
			break;
		case 0x99:
			SBC(Register.c);
			cycles = 4;
			break;
		case 0x9A:
			SBC(Register.d);
			cycles = 4;
			break;
		case 0x9B:
			SBC(Register.e);
			cycles = 4;
			break;
		case 0x9C:
			SBC(Register.h);
			cycles = 4;
			break;
		case 0x9D:
			SBC(Register.l);
			cycles = 4;
			break;
		case 0x9E:
			SBC(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0x9F:
			SBC(Register.a);
			cycles = 4;
			break;
		case 0xA0:
			AND(Register.b);
			cycles = 4;
			break;
		case 0xA1:
			AND(Register.c);
			cycles = 4;
			break;
		case 0xA2:
			AND(Register.d);
			cycles = 4;
			break;
		case 0xA3:
			AND(Register.e);
			cycles = 4;
			break;
		case 0xA4:
			AND(Register.h);
			cycles = 4;
			break;
		case 0xA5:
			AND(Register.l);
			cycles = 4;
			break;
		case 0xA6:
			AND(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0xA7:
			AND(Register.a);
			cycles = 4;
			break;
		case 0xA8:
			XOR(Register.b);
			cycles = 4;
			break;
		case 0xA9:
			XOR(Register.c);
			cycles = 4;
			break;
		case 0xAA:
			XOR(Register.d);
			cycles = 4;
			break;
		case 0xAB:
			XOR(Register.e);
			cycles = 4;
			break;
		case 0xAC:
			XOR(Register.h);
			cycles = 4;
			break;
		case 0xAD:
			XOR(Register.l);
			cycles = 4;
			break;
		case 0xAE:
			XOR(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0xAF:
			XOR(Register.a);
			cycles = 4;
			break;
		case 0xB0:
			OR(Register.b);
			cycles = 4;
			break;
		case 0xB1:
			OR(Register.c);
			cycles = 4;
			break;
		case 0xB2:
			OR(Register.d);
			cycles = 4;
			break;
		case 0xB3:
			OR(Register.e);
			cycles = 4;
			break;
		case 0xB4:
			OR(Register.h);
			cycles = 4;
			break;
		case 0xB5:
			OR(Register.l);
			cycles = 4;
			break;
		case 0xB6:
			OR(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0xB7:
			OR(Register.a);
			cycles = 4;
			break;
		case 0xB8:
			CP(Register.b);
			cycles = 4;
			break;
		case 0xB9:
			CP(Register.c);
			cycles = 4;
			break;
		case 0xBA:
			CP(Register.d);
			cycles = 4;
			break;
		case 0xBB:
			CP(Register.e);
			cycles = 4;
			break;
		case 0xBC:
			CP(Register.h);
			cycles = 4;
			break;
		case 0xBD:
			CP(Register.l);
			cycles = 4;
			break;
		case 0xBE:
			CP(Gameboy.ReadMemory(Register.hl()));
			cycles = 8;
			break;
		case 0xBF:
			CP(Register.a);
			cycles = 4;
			break;
		case 0xC0:
			RET(!Register.zeroF);
			break;
		case 0xC1:
		{
			uint16_t temp{};
			POP(temp);
			Register.bc(temp);
			cycles = 12;
			break;
		}
		case 0xC2:
			JP(Gameboy.ReadMemoryWord(Register.pc), !Register.zeroF);
			break;
		case 0xC3:
			JP(Gameboy.ReadMemoryWord(Register.pc), true);
			break;
		case 0xC4:
			CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.zeroF);
			break;
		case 0xC5:
			PUSH(Register.bc());
			cycles = 16;
			break;
		case 0xC6:
			ADD(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xC7:
			RST(0x00);
			cycles = 16;
			break;
		case 0xC8:
			RET(Register.zeroF);
			break;
		case 0xC9:
			RET(true, false);
			cycles = 16;
			break;
		case 0xCA:
			JP(Gameboy.ReadMemoryWord(Register.pc), Register.zeroF);
			break;
		case 0xCB:
		{
			auto extendedOP = Gameboy.ReadMemory(Register.pc++);
			cycles = ExecuteExtendedOpcode(extendedOP);
			break;
		}
		case 0xCC:
			CALL(Gameboy.ReadMemoryWord(Register.pc), Register.zeroF);
			break;
		case 0xCD:
			CALL(Gameboy.ReadMemoryWord(Register.pc), true);
			break;
		case 0xCE:
			ADC(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xCF:
			RST(0x08);
			cycles = 16;
			break;
		case 0xD0:
			RET(!Register.carryF);
			break;
		case 0xD1:
		{
			uint16_t temp;
			POP(temp);
			Register.de(temp);
			cycles = 12;
			break;
		}
		case 0xD2:
			JP(Gameboy.ReadMemoryWord(Register.pc), !Register.carryF);
			break;
		case 0xD4:
			CALL(Gameboy.ReadMemoryWord(Register.pc), !Register.carryF);
			break;
		case 0xD5:
			PUSH(Register.de());
			cycles = 16;
			break;
		case 0xD6:
			SUB(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xD7:
			RST(0x10);
			cycles = 16;
			break;
		case 0xD8:
			RET(Register.carryF);
			break;
		case 0xD9:
			RETI();
			cycles = 16;
			break;
		case 0xDA:
			JP(Gameboy.ReadMemoryWord(Register.pc), Register.carryF);
			break;
		case 0xDC:
			CALL(Gameboy.ReadMemoryWord(Register.pc), Register.carryF);
			break;
		case 0xDE:
			SBC(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xDF:
			RST(0x18);
			cycles = 16;
			break;
		case 0xE0:
			LD(uint16_t(0xFF00 + Gameboy.ReadMemory(Register.pc++)), Register.a);
			cycles = 12;
			break;
		case 0xE1:
		{
			uint16_t temp;
			POP(temp);
			Register.hl(temp);
			cycles = 12;
			break;
		}
		case 0xE2:
			LD(uint16_t(0xFF00 + Register.c), Register.a);
			cycles = 8;
			break;
		case 0xE5:
			PUSH(Register.hl());
			cycles = 16;
			break;
		case 0xE6:
			AND(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xE7:
			RST(0x20);
			cycles = 16;
			break;
		case 0xE8:
			ADD16(false, Gameboy.ReadMemory(Register.pc++));
			cycles = 16;
			break;
		case 0xE9:
			JP(Register.hl(), true, false);
			cycles = 4;
			break;
		case 0xEA:
		{
			const uint16_t adrs{ Gameboy.ReadMemoryWord(Register.pc) };
			LD(adrs, Register.a);
			cycles = 16;
			break;
		}
		case 0xEE:
			XOR(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xEF:
			RST(0x28);
			cycles = 16;
			break;
		case 0xF0:
		{
			const uint8_t val{ Gameboy.ReadMemory(Register.pc++) };
			LD(Register.a, Gameboy.ReadMemory(uint16_t(0xFF00 + val)));
			cycles = 12;
			break;
		}
		case 0xF1:
		{
			/*uint16_t temp;
			POP(temp);
			Register.af(temp);*/
			POP_AF();
			cycles = 12;
			break;
		}
		case 0xF2:
			LD(Register.a, Gameboy.ReadMemory(0xFF00 + Register.c));
			cycles = 8;
			break;
		case 0xF3:
			DI();
			cycles = 4;
			break;
		case 0xF5:
			PUSH(Register.af());
			cycles = 16;
			break;
		case 0xF6:
			OR(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xF7:
			RST(0x30);
			cycles = 16;
			break;
		case 0xF8:
		{
			uint16_t temp{ Register.hl() };
			LD(&temp, Register.sp + (int8_t)Gameboy.ReadMemory(Register.pc++));
			cycles = 12;
			Register.hl(temp);
			break;
		}
		case 0xF9:
			LD(Register.sp, Register.hl());
			cycles = 8;
			break;
		case 0xFA:
			LD(Register.a, Gameboy.ReadMemory(Gameboy.ReadMemoryWord(Register.pc)));
			cycles = 16;
			break;
		case 0xFB:
			EI();
			cycles = 4;
			break;
		case 0xFE:
			CP(Gameboy.ReadMemory(Register.pc++));
			cycles = 8;
			break;
		case 0xFF:
			RST(0x38);
			cycles = 16;
			break;
#ifdef _DEBUG
		default:
			assert( false );
			break;
#endif
	}

	Gameboy.AddCycles( cycles );
}

uint8_t LR35902::ExecuteExtendedOpcode(uint8_t opcode)
{
#define OPCYCLE(func, cycl) func; cycles = cycl; break
#define BASICOPS(A, B, C, D, E, H, L, cycles, funcName, ...) \
	case A: OPCYCLE( funcName( Register.a __VA_ARGS__), cycles ); \
	case B: OPCYCLE( funcName( Register.b __VA_ARGS__), cycles ); \
	case C: OPCYCLE( funcName( Register.c __VA_ARGS__), cycles ); \
	case D: OPCYCLE( funcName( Register.d __VA_ARGS__), cycles ); \
	case E: OPCYCLE( funcName( Register.e __VA_ARGS__), cycles ); \
	case H: OPCYCLE( funcName( Register.h __VA_ARGS__), cycles ); \
	case L: OPCYCLE( funcName( Register.l __VA_ARGS__), cycles )


	uint8_t cycles{};
	switch (opcode)
	{
	case 0x00:
		RLC(Register.b);
		cycles = 8;
		break;
	case 0x01:
		RLC(Register.c);
		cycles = 8;
		break;
	case 0x02:
		RLC(Register.d);
		cycles = 8;
		break;
	case 0x03:
		RLC(Register.e);
		cycles = 8;
		break;
	case 0x04:
		RLC(Register.h);
		cycles = 8;
		break;
	case 0x05:
		RLC(Register.l);
		cycles = 8;
		break;
	case 0x06:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RLC(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x07:
		RLC(Register.a);
		cycles = 8;
		break;
	case 0x08:
		RRC(Register.b);
		cycles = 8;
		break;
	case 0x09:
		RRC(Register.c);
		cycles = 8;
		break;
	case 0x0A:
		RRC(Register.d);
		cycles = 8;
		break;
	case 0x0B:
		RRC(Register.e);
		cycles = 8;
		break;
	case 0x0C:
		RRC(Register.h);
		cycles = 8;
		break;
	case 0x0D:
		RRC(Register.l);
		cycles = 8;
		break;
	case 0x0E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RRC(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x0F:
		RRC(Register.a);
		cycles = 8;
		break;
	case 0x10:
		RL(Register.b);
		cycles = 8;
		break;
	case 0x11:
		RL(Register.c);
		cycles = 8;
		break;
	case 0x12:
		RL(Register.d);
		cycles = 8;
		break;
	case 0x13:
		RL(Register.e);
		cycles = 8;
		break;
	case 0x14:
		RL(Register.h);
		cycles = 8;
		break;
	case 0x15:
		RL(Register.l);
		cycles = 8;
		break;
	case 0x16:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RL(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x17:
		RL(Register.a);
		cycles = 8;
		break;
	case 0x18:
		RR(Register.b);
		cycles = 8;
		break;
	case 0x19:
		RR(Register.c);
		cycles = 8;
		break;
	case 0x1A:
		RR(Register.d);
		cycles = 8;
		break;
	case 0x1B:
		RR(Register.e);
		cycles = 8;
		break;
	case 0x1C:
		RR(Register.h);
		cycles = 8;
		break;
	case 0x1D:
		RR(Register.l);
		cycles = 8;
		break;
	case 0x1E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RR(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x1F:
		RR(Register.a);
		cycles = 8;
		break;
	case 0x20:
		SLA(Register.b);
		cycles = 8;
		break;
	case 0x21:
		SLA(Register.c);
		cycles = 8;
		break;
	case 0x22:
		SLA(Register.d);
		cycles = 8;
		break;
	case 0x23:
		SLA(Register.e);
		cycles = 8;
		break;
	case 0x24:
		SLA(Register.h);
		cycles = 8;
		break;
	case 0x25:
		SLA(Register.l);
		cycles = 8;
		break;
	case 0x26:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SLA(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x27:
		SLA(Register.a);
		cycles = 8;
		break;
	case 0x28:
		SRA(Register.b);
		cycles = 8;
		break;
	case 0x29:
		SRA(Register.c);
		cycles = 8;
		break;
	case 0x2A:
		SRA(Register.d);
		cycles = 8;
		break;
	case 0x2B:
		SRA(Register.e);
		cycles = 8;
		break;
	case 0x2C:
		SRA(Register.h);
		cycles = 8;
		break;
	case 0x2D:
		SRA(Register.l);
		cycles = 8;
		break;
	case 0x2E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SRA(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x2F:
		SRA(Register.a);
		cycles = 8;
		break;
	case 0x30:
		SWAP(Register.b);
		cycles = 8;
		break;
	case 0x31:
		SWAP(Register.c);
		cycles = 8;
		break;
	case 0x32:
		SWAP(Register.d);
		cycles = 8;
		break;
	case 0x33:
		SWAP(Register.e);
		cycles = 8;
		break;
	case 0x34:
		SWAP(Register.h);
		cycles = 8;
		break;
	case 0x35:
		SWAP(Register.l);
		cycles = 8;
		break;
	case 0x36:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SWAP(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x37:
		SWAP(Register.a);
		cycles = 8;
		break;
	case 0x38:
		SRL(Register.b);
		cycles = 8;
		break;
	case 0x39:
		SRL(Register.c);
		cycles = 8;
		break;
	case 0x3A:
		SRL(Register.d);
		cycles = 8;
		break;
	case 0x3B:
		SRL(Register.e);
		cycles = 8;
		break;
	case 0x3C:
		SRL(Register.h);
		cycles = 8;
		break;
	case 0x3D:
		SRL(Register.l);
		cycles = 8;
		break;
	case 0x3E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SRL(hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x3F:
		SRL(Register.a);
		cycles = 8;
		break;
	case 0x40:
		BIT(0, Register.b);
		cycles = 8;
		break;
	case 0x41:
		BIT(0, Register.c);
		cycles = 8;
		break;
	case 0x42:
		BIT(0, Register.d);
		cycles = 8;
		break;
	case 0x43:
		BIT(0, Register.e);
		cycles = 8;
		break;
	case 0x44:
		BIT(0, Register.h);
		cycles = 8;
		break;
	case 0x45:
		BIT(0, Register.l);
		cycles = 8;
		break;
	case 0x46:
	{
		BIT(0, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x47:
		BIT(0, Register.a);
		cycles = 8;
		break;
	case 0x48:
		BIT(1, Register.b);
		cycles = 8;
		break;
	case 0x49:
		BIT(1, Register.c);
		cycles = 8;
		break;
	case 0x4A:
		BIT(1, Register.d);
		cycles = 8;
		break;
	case 0x4B:
		BIT(1, Register.e);
		cycles = 8;
		break;
	case 0x4C:
		BIT(1, Register.h);
		cycles = 8;
		break;
	case 0x4D:
		BIT(1, Register.l);
		cycles = 8;
		break;
	case 0x4E:
	{
		BIT(1, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x4F:
		BIT(1, Register.a);
		cycles = 8;
		break;
	case 0x50:
		BIT(2, Register.b);
		cycles = 8;
		break;
	case 0x51:
		BIT(2, Register.c);
		cycles = 8;
		break;
	case 0x52:
		BIT(2, Register.d);
		cycles = 8;
		break;
	case 0x53:
		BIT(2, Register.e);
		cycles = 8;
		break;
	case 0x54:
		BIT(2, Register.h);
		cycles = 8;
		break;
	case 0x55:
		BIT(2, Register.l);
		cycles = 8;
		break;
	case 0x56:
	{
		BIT(2, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x57:
		BIT(2, Register.a);
		cycles = 8;
		break;
	case 0x58:
		BIT(3, Register.b);
		cycles = 8;
		break;
	case 0x59:
		BIT(3, Register.c);
		cycles = 8;
		break;
	case 0x5A:
		BIT(3, Register.d);
		cycles = 8;
		break;
	case 0x5B:
		BIT(3, Register.e);
		cycles = 8;
		break;
	case 0x5C:
		BIT(3, Register.h);
		cycles = 8;
		break;
	case 0x5D:
		BIT(3, Register.l);
		cycles = 8;
		break;
	case 0x5E:
	{
		BIT(3, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x5F:
		BIT(3, Register.a);
		cycles = 8;
		break;
	case 0x60:
		BIT(4, Register.b);
		cycles = 8;
		break;
	case 0x61:
		BIT(4, Register.c);
		cycles = 8;
		break;
	case 0x62:
		BIT(4, Register.d);
		cycles = 8;
		break;
	case 0x63:
		BIT(4, Register.e);
		cycles = 8;
		break;
	case 0x64:
		BIT(4, Register.h);
		cycles = 8;
		break;
	case 0x65:
		BIT(4, Register.l);
		cycles = 8;
		break;
	case 0x66:
	{
		BIT(4, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x67:
		BIT(4, Register.a);
		cycles = 8;
		break;
	case 0x68:
		BIT(5, Register.b);
		cycles = 8;
		break;
	case 0x69:
		BIT(5, Register.c);
		cycles = 8;
		break;
	case 0x6A:
		BIT(5, Register.d);
		cycles = 8;
		break;
	case 0x6B:
		BIT(5, Register.e);
		cycles = 8;
		break;
	case 0x6C:
		BIT(5, Register.h);
		cycles = 8;
		break;
	case 0x6D:
		BIT(5, Register.l);
		cycles = 8;
		break;
	case 0x6E:
	{
		BIT(5, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x6F:
		BIT(5, Register.a);
		cycles = 8;
		break;
	case 0x70:
		BIT(6, Register.b);
		cycles = 8;
		break;
	case 0x71:
		BIT(6, Register.c);
		cycles = 8;
		break;
	case 0x72:
		BIT(6, Register.d);
		cycles = 8;
		break;
	case 0x73:
		BIT(6, Register.e);
		cycles = 8;
		break;
	case 0x74:
		BIT(6, Register.h);
		cycles = 8;
		break;
	case 0x75:
		BIT(6, Register.l);
		cycles = 8;
		break;
	case 0x76:
	{
		BIT(6, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x77:
		BIT(6, Register.a);
		cycles = 8;
		break;
	case 0x78:
		BIT(7, Register.b);
		cycles = 8;
		break;
	case 0x79:
		BIT(7, Register.c);
		cycles = 8;
		break;
	case 0x7A:
		BIT(7, Register.d);
		cycles = 8;
		break;
	case 0x7B:
		BIT(7, Register.e);
		cycles = 8;
		break;
	case 0x7C:
		BIT(7, Register.h);
		cycles = 8;
		break;
	case 0x7D:
		BIT(7, Register.l);
		cycles = 8;
		break;
	case 0x7E:
	{
		BIT(7, Gameboy.ReadMemory(Register.hl()));
		cycles = 16;
		break;
	}
	case 0x7F:
		BIT(7, Register.a);
		cycles = 8;
		break;
	case 0x80:
		RES(0, Register.b);
		cycles = 8;
		break;
	case 0x81:
		RES(0, Register.c);
		cycles = 8;
		break;
	case 0x82:
		RES(0, Register.d);
		cycles = 8;
		break;
	case 0x83:
		RES(0, Register.e);
		cycles = 8;
		break;
	case 0x84:
		RES(0, Register.h);
		cycles = 8;
		break;
	case 0x85:
		RES(0, Register.l);
		cycles = 8;
		break;
	case 0x86:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(0, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x87:
		RES(0, Register.a);
		cycles = 8;
		break;
	case 0x88:
		RES(1, Register.b);
		cycles = 8;
		break;
	case 0x89:
		RES(1, Register.c);
		cycles = 8;
		break;
	case 0x8A:
		RES(1, Register.d);
		cycles = 8;
		break;
	case 0x8B:
		RES(1, Register.e);
		cycles = 8;
		break;
	case 0x8C:
		RES(1, Register.h);
		cycles = 8;
		break;
	case 0x8D:
		RES(1, Register.l);
		cycles = 8;
		break;
	case 0x8E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(1, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x8F:
		RES(1, Register.a);
		cycles = 8;
		break;
	case 0x90:
		RES(2, Register.b);
		cycles = 8;
		break;
	case 0x91:
		RES(2, Register.c);
		cycles = 8;
		break;
	case 0x92:
		RES(2, Register.d);
		cycles = 8;
		break;
	case 0x93:
		RES(2, Register.e);
		cycles = 8;
		break;
	case 0x94:
		RES(2, Register.h);
		cycles = 8;
		break;
	case 0x95:
		RES(2, Register.l);
		cycles = 8;
		break;
	case 0x96:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(2, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x97:
		RES(2, Register.a);
		cycles = 8;
		break;
	case 0x98:
		RES(3, Register.b);
		cycles = 8;
		break;
	case 0x99:
		RES(3, Register.c);
		cycles = 8;
		break;
	case 0x9A:
		RES(3, Register.d);
		cycles = 8;
		break;
	case 0x9B:
		RES(3, Register.e);
		cycles = 8;
		break;
	case 0x9C:
		RES(3, Register.h);
		cycles = 8;
		break;
	case 0x9D:
		RES(3, Register.l);
		cycles = 8;
		break;
	case 0x9E:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(3, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0x9F:
		RES(3, Register.a);
		cycles = 8;
		break;
	case 0xA0:
		RES(4, Register.b);
		cycles = 8;
		break;
	case 0xA1:
		RES(4, Register.c);
		cycles = 8;
		break;
	case 0xA2:
		RES(4, Register.d);
		cycles = 8;
		break;
	case 0xA3:
		RES(4, Register.e);
		cycles = 8;
		break;
	case 0xA4:
		RES(4, Register.h);
		cycles = 8;
		break;
	case 0xA5:
		RES(4, Register.l);
		cycles = 8;
		break;
	case 0xA6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(4, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xA7:
		RES(4, Register.a);
		cycles = 8;
		break;
	case 0xA8:
		RES(5, Register.b);
		cycles = 8;
		break;
	case 0xA9:
		RES(5, Register.c);
		cycles = 8;
		break;
	case 0xAA:
		RES(5, Register.d);
		cycles = 8;
		break;
	case 0xAB:
		RES(5, Register.e);
		cycles = 8;
		break;
	case 0xAC:
		RES(5, Register.h);
		cycles = 8;
		break;
	case 0xAD:
		RES(5, Register.l);
		cycles = 8;
		break;
	case 0xAE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(5, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xAF:
		RES(5, Register.a);
		cycles = 8;
		break;
	case 0xB0:
		RES(6, Register.b);
		cycles = 8;
		break;
	case 0xB1:
		RES(6, Register.c);
		cycles = 8;
		break;
	case 0xB2:
		RES(6, Register.d);
		cycles = 8;
		break;
	case 0xB3:
		RES(6, Register.e);
		cycles = 8;
		break;
	case 0xB4:
		RES(6, Register.h);
		cycles = 8;
		break;
	case 0xB5:
		RES(6, Register.l);
		cycles = 8;
		break;
	case 0xB6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(6, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xB7:
		RES(6, Register.a);
		cycles = 8;
		break;
	case 0xB8:
		RES(7, Register.b);
		cycles = 8;
		break;
	case 0xB9:
		RES(7, Register.c);
		cycles = 8;
		break;
	case 0xBA:
		RES(7, Register.d);
		cycles = 8;
		break;
	case 0xBB:
		RES(7, Register.e);
		cycles = 8;
		break;
	case 0xBC:
		RES(7, Register.h);
		cycles = 8;
		break;
	case 0xBD:
		RES(7, Register.l);
		cycles = 8;
		break;
	case 0xBE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		RES(7, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xBF:
		RES(7, Register.a);
		cycles = 8;
		break;
	case 0xC0:
		SET(0, Register.b);
		cycles = 8;
		break;
	case 0xC1:
		SET(0, Register.c);
		cycles = 8;
		break;
	case 0xC2:
		SET(0, Register.d);
		cycles = 8;
		break;
	case 0xC3:
		SET(0, Register.e);
		cycles = 8;
		break;
	case 0xC4:
		SET(0, Register.h);
		cycles = 8;
		break;
	case 0xC5:
		SET(0, Register.l);
		cycles = 8;
		break;
	case 0xC6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(0, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xC7:
		SET(0, Register.a);
		cycles = 8;
		break;
	case 0xC8:
		SET(1, Register.b);
		cycles = 8;
		break;
	case 0xC9:
		SET(1, Register.c);
		cycles = 8;
		break;
	case 0xCA:
		SET(1, Register.d);
		cycles = 8;
		break;
	case 0xCB:
		SET(1, Register.e);
		cycles = 8;
		break;
	case 0xCC:
		SET(1, Register.h);
		cycles = 8;
		break;
	case 0xCD:
		SET(1, Register.l);
		cycles = 8;
		break;
	case 0xCE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(1, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xCF:
		SET(1, Register.a);
		cycles = 8;
		break;
	case 0xD0:
		SET(2, Register.b);
		cycles = 8;
		break;
	case 0xD1:
		SET(2, Register.c);
		cycles = 8;
		break;
	case 0xD2:
		SET(2, Register.d);
		cycles = 8;
		break;
	case 0xD3:
		SET(2, Register.e);
		cycles = 8;
		break;
	case 0xD4:
		SET(2, Register.h);
		cycles = 8;
		break;
	case 0xD5:
		SET(2, Register.l);
		cycles = 8;
		break;
	case 0xD6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(2, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xD7:
		SET(2, Register.a);
		cycles = 8;
		break;
	case 0xD8:
		SET(3, Register.b);
		cycles = 8;
		break;
	case 0xD9:
		SET(3, Register.c);
		cycles = 8;
		break;
	case 0xDA:
		SET(3, Register.d);
		cycles = 8;
		break;
	case 0xDB:
		SET(3, Register.e);
		cycles = 8;
		break;
	case 0xDC:
		SET(3, Register.h);
		cycles = 8;
		break;
	case 0xDD:
		SET(3, Register.l);
		cycles = 8;
		break;
	case 0xDE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(3, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xDF:
		SET(3, Register.a);
		cycles = 8;
		break;
	case 0xE0:
		SET(4, Register.b);
		cycles = 8;
		break;
	case 0xE1:
		SET(4, Register.c);
		cycles = 8;
		break;
	case 0xE2:
		SET(4, Register.d);
		cycles = 8;
		break;
	case 0xE3:
		SET(4, Register.e);
		cycles = 8;
		break;
	case 0xE4:
		SET(4, Register.h);
		cycles = 8;
		break;
	case 0xE5:
		SET(4, Register.l);
		cycles = 8;
		break;
	case 0xE6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(4, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xE7:
		SET(4, Register.a);
		cycles = 8;
		break;
	case 0xE8:
		SET(5, Register.b);
		cycles = 8;
		break;
	case 0xE9:
		SET(5, Register.c);
		cycles = 8;
		break;
	case 0xEA:
		SET(5, Register.d);
		cycles = 8;
		break;
	case 0xEB:
		SET(5, Register.e);
		cycles = 8;
		break;
	case 0xEC:
		SET(5, Register.h);
		cycles = 8;
		break;
	case 0xED:
		SET(5, Register.l);
		cycles = 8;
		break;
	case 0xEE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(5, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xEF:
		SET(5, Register.a);
		cycles = 8;
		break;
	case 0xF0:
		SET(6, Register.b);
		cycles = 8;
		break;
	case 0xF1:
		SET(6, Register.c);
		cycles = 8;
		break;
	case 0xF2:
		SET(6, Register.d);
		cycles = 8;
		break;
	case 0xF3:
		SET(6, Register.e);
		cycles = 8;
		break;
	case 0xF4:
		SET(6, Register.h);
		cycles = 8;
		break;
	case 0xF5:
		SET(6, Register.l);
		cycles = 8;
		break;
	case 0xF6:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(6, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xF7:
		SET(6, Register.a);
		cycles = 8;
		break;
	case 0xF8:
		SET(7, Register.b);
		cycles = 8;
		break;
	case 0xF9:
		SET(7, Register.c);
		cycles = 8;
		break;
	case 0xFA:
		SET(7, Register.d);
		cycles = 8;
		break;
	case 0xFB:
		SET(7, Register.e);
		cycles = 8;
		break;
	case 0xFC:
		SET(7, Register.h);
		cycles = 8;
		break;
	case 0xFD:
		SET(7, Register.l);
		cycles = 8;
		break;
	case 0xFE:
	{
		uint8_t hlDeRef{ Gameboy.ReadMemory(Register.hl()) };
		SET(7, hlDeRef);
		Gameboy.WriteMemory(Register.hl(), hlDeRef);
		cycles = 16;
		break;
	}
	case 0xFF:
		SET(7, Register.a);
		cycles = 8;
		break;
	default:
		//std::cout << "No extended opcode found\n";
		break;
	}

	return cycles;
}

void LR35902::ConfigureLCDStatus() 
{
	if (!(Gameboy.GetLCDC() & 0x80)) 
	{
		//LCD Disabled
		Gameboy.GetLY() = LCDCycles = 0;
		Gameboy.GetLCDS() = ((Gameboy.GetLCDS() & 0xFC) | 1); //Set mode to vblank
	}

	uint8_t oldMode{ uint8_t( Gameboy.GetLCDS() & 3 ) };
	if (Gameboy.GetLY() >= 144) 
	{
		//Mode 1 VBlank >4
		Gameboy.GetLCDS() &= 0xFC;
		Gameboy.GetLCDS() |= 0x1;
		oldMode |= (bool( Gameboy.GetLCDS() & 0x10 ) << 2); //can we interrupt?
	} 
	else if (LCDCycles <= 80) 
	{
		//Mode 2 OAM >5
		Gameboy.GetLCDS() &= 0xFC;
		Gameboy.GetLCDS() |= 0x2;
		oldMode |= (bool( Gameboy.GetLCDS() & 0x20 ) << 2);
	} 
	else if (LCDCycles <= 252) //80+172
	{ 
		//Mode DataTransfer 3
		Gameboy.GetLCDS() |= 0x3;
	} 
	else 
	{
		//Mode Hblank 0
		Gameboy.GetLCDS() &= 0xFC;
		oldMode |= (bool( Gameboy.GetLCDS() & 8 ) << 2);
	}

	if (oldMode != (Gameboy.GetLCDC() & 3) && (oldMode & 8)) //Our mode changed and we can interrupt
	{
		Gameboy.RequestInterrupt(lcdStat);
	}

	if (Gameboy.GetLY() == Gameboy.GetRawMemory()[0xFF45]) 
	{
		//Compare LY to LYC to see if we're requesting the current line
		Gameboy.GetLCDS() |= 4;

		if (Gameboy.GetLCDS() & 0x40)
		{
			Gameboy.RequestInterrupt(lcdStat);
		}
	} 
	else
	{
		Gameboy.GetLCDS() &= ~(unsigned(1) << 2);
	}
}

void LR35902::DrawLine()  
{
	DrawBackground();
	DrawWindow(); //window == ui
	DrawSprites();
}

void LR35902::DrawBackground()  
{
	assert( Gameboy.GetLCDC() & 0x80 );
	const uint16_t tileSetAdress{ uint16_t( bool( Gameboy.GetLCDC() & 16 ) ? 0x8000 : 0x8800 ) };
	const uint16_t tileMapAddress{ uint16_t( bool( Gameboy.GetLCDC() & 8 ) ? 0x9C00 : 0x9800 ) };
	const uint8_t scrollX{ Gameboy.ReadMemory( 0xFF43 ) }; //scroll

	const uint8_t yWrap{ uint8_t( (Gameboy.ReadMemory( 0xFF42 ) + Gameboy.GetLY()) % 256 ) };
	const uint8_t tileY{ uint8_t( yWrap % 8 ) }; //The y pixel of the tile
	const uint16_t tileMapOffset{ uint16_t( tileMapAddress + uint16_t( yWrap / 8 ) * 32 ) };
	const uint16_t fbOffset{ uint16_t( (Gameboy.GetLY() - 1) * 160 ) };
	uint8_t colors[4];
	ConfigureColorArray( colors, Gameboy.ReadMemory( 0xFF47 ) );

	std::bitset<160 * 144 * 2> &fBuffer{ Gameboy.GetFramebuffer() };

	for (uint8_t x{ 0 }; x < 160; ++x) {
		const uint8_t xWrap{ uint8_t( (x + scrollX) % 256 ) };
		const uint16_t tileNumAddress{ uint16_t( tileMapOffset + (xWrap / 8) ) };
		const uint16_t tileData{ uint16_t( tileSetAdress + Gameboy.ReadMemory( tileNumAddress ) * 16 ) };

		const uint8_t paletteResult{ ReadPalette( tileData, uint8_t( xWrap % 8 ), tileY ) };
		fBuffer[(fbOffset + x) * 2] = (colors[paletteResult] & 2);
		fBuffer[((fbOffset + x) * 2) + 1] = (colors[paletteResult] & 1);
	}
}

void LR35902::DrawWindow()  
{if (!(Gameboy.GetLCDC() & 32)) return; //window disabled

	const uint8_t wndX{ Gameboy.ReadMemory( 0xFF4B ) }; //scroll
	const uint8_t wndY{ Gameboy.ReadMemory( 0xFF4A ) };

	if (wndX < 7 || wndX >= (160 + 7) || wndY < 0 || wndY >= 144) 
	{
		return;
	}
	if (wndY > Gameboy.GetLY()) 
	{
		return;
	}

	const uint16_t tileSetAdress{ uint16_t( bool( Gameboy.GetLCDC() & 16 ) ? 0x8000 : 0x8800 ) }; //If 0x8800 then the tile identifier is in signed TileSET
	const uint16_t tileMapAddress{ uint16_t( bool( Gameboy.GetLCDC() & 64 ) ? 0x9C00 : 0x9800 ) }; //TileMAP
	const uint8_t yWrap{ uint8_t( (wndY + Gameboy.GetLY()) % 256 ) };
	const uint8_t tileY{ uint8_t( yWrap % 8 ) }; //The y pixel of the tile
	const uint16_t tileRow{ uint16_t( yWrap / 8 ) };
	const uint16_t tileMapOffset{ uint16_t( tileMapAddress + tileRow * 32 ) };
	const uint8_t fbOffset{ uint8_t( (Gameboy.GetLY() - 1) * 160 ) };

	uint8_t colors[4];
	ConfigureColorArray( colors, Gameboy.ReadMemory( 0xFF47 ) );

	for (uint8_t x{ uint8_t( (wndX < 0) ? 0 : wndX ) }; x < 160; ++x) 
	{
		const uint8_t tileX{ uint8_t( x % 8 ) }; //The x pixel of the tile
		const uint16_t tileColumn{ uint16_t( x / 8 ) };

		const uint16_t tileNumAddress{ uint16_t( tileMapOffset + tileColumn ) };
		const int16_t tileNumber{ Gameboy.ReadMemory( tileNumAddress ) };

		const uint16_t tileAddress{ uint16_t( tileSetAdress + tileNumber * 16 ) }; //todo: Verify

		const uint8_t paletteResult{ ReadPalette( tileAddress, tileX, tileY ) };
		Gameboy.GetFramebuffer()[(fbOffset + x) * 2] = (colors[paletteResult] & 2);
		Gameboy.GetFramebuffer()[((fbOffset + x) * 2) + 1] = (colors[paletteResult] & 1);
	}
}

void LR35902::DrawSprites() 
{
	if (Gameboy.GetLCDC() & 2) {
		//todo: support 8x16
#pragma loop(hint_parallel(20))
		for (uint16_t oamAddress{ 0xFE9C }; oamAddress >= 0xFE00; oamAddress -= 4) {
			const uint8_t yCoord{ uint8_t( Gameboy.ReadMemory( oamAddress ) - 0x10 ) };
			const uint8_t xCoord{ uint8_t( Gameboy.ReadMemory( int16_t( oamAddress + 1 ) ) - 0x8 ) };

			if (xCoord >= 160 || yCoord >= 144)
				continue;

			uint8_t tileY{ uint8_t( Gameboy.GetLY() - yCoord ) };
			if (yCoord > Gameboy.GetLY() || tileY >= 8)
				continue;

			const uint16_t spriteNum{ Gameboy.ReadMemory( uint16_t( oamAddress + 2 ) ) };
			const uint16_t optionsByte{ Gameboy.ReadMemory( uint16_t( oamAddress + 3 ) ) };
			const bool xFlip{ (optionsByte & 0x20) != 0 };
			const bool yFlip{ (optionsByte & 0x40) != 0 };
			const bool priority{ (optionsByte & 0x80) != 0 };
			const uint16_t paletteAddress{ uint16_t( ((optionsByte & 0x10) > 0) ? 0xFF49 : 0xff48 ) };
			const uint8_t palette{ Gameboy.ReadMemory( paletteAddress ) };

			if (yFlip)
				tileY = (7 - tileY);
			const uint16_t tileAddress{ uint16_t( 0x8000 + 0x10 * spriteNum ) };
			for (uint8_t x{ 0 }; x < 8; ++x) {
				if (xCoord + x < 0 || xCoord + x >= 160) //outside range
					continue;

				const uint8_t tileX{ uint8_t( xFlip ? (7 - x) : x ) };
				const uint8_t paletteIndex{ ReadPalette( tileAddress, tileX, tileY ) };
				uint8_t spriteColors[4];
				uint8_t bgColors[4];

				ConfigureColorArray( spriteColors, palette );
				ConfigureColorArray( bgColors, Gameboy.ReadMemory( 0xFF47 ) );
				if (spriteColors[paletteIndex] > 0) { //White is transparent
					const int16_t i{ int16_t( (Gameboy.GetLY() - 1) * 160 + xCoord + x ) };
					if (priority || (Gameboy.GetPixelColor( i ) != bgColors[3])) { //if the pixel has priority or the underlying pixel is not "black"
						Gameboy.GetFramebuffer()[i * 2] = spriteColors[paletteIndex] & 2;
						Gameboy.GetFramebuffer()[i * 2 + 1] = spriteColors[paletteIndex] & 1;
					}
				}
			}
		}
	}
}

uint8_t LR35902::ReadPalette( const uint16_t pixelData, const uint8_t xPixel, const uint8_t yPixel ) const 
{
	const uint8_t low{ Gameboy.ReadMemory( pixelData + (yPixel * 2) ) };
	const uint8_t hi{ Gameboy.ReadMemory( pixelData + (yPixel * 2) + 1 ) };

	return ((hi >> (7 - xPixel) & 1) << 1) | ((low >> (7 - xPixel)) & 1);
}

void LR35902::ConfigureColorArray( uint8_t *const colorArray, uint8_t palette ) const 
{

	colorArray[0] = palette & 0x3;
	palette >>= 2;
	colorArray[1] = palette & 0x3;
	palette >>= 2;
	colorArray[2] = palette & 0x3;
	palette >>= 2;
	colorArray[3] = palette & 0x3;
}

//TODO: Investigate Threading Bottleneck..
/*void LR35902::ThreadWork( const uint8_t id, LR35902::DrawData **const drawData ) {
	std::unique_lock<std::mutex> mtx{ ConditionalVariableMutex };

	while (true) {
		ActivateDrawers.
			wait( mtx, [&drawData, id]()
			{
				return (*drawData && !((*drawData)->doneCounter & (1 << id)));
			} ); //needs mutex to not miss the signal/check condition; Checks if this worker's bit is already set
		//std::cout << int(id)<<" Started\n";
		mtx.unlock(); //Let the other threads do their thing

		const uint8_t fin{ uint8_t( id * 16 + 16 ) };
		std::bitset<160 * 144 * 2> *fBuffer{ (std::bitset<160 * 144 * 2>*)(*drawData)->bitset };
		for (uint8_t x{ uint8_t( id * 16 ) }; x < fin; ++x) {
			const uint8_t xWrap{ uint8_t( (x + (*drawData)->scrollX) % 256 ) };
			const uint16_t tileNumAddress{ uint16_t( (*drawData)->tileMapOffset + (xWrap / 8) ) };

			const uint16_t tileData{ uint16_t( (*drawData)->tileSetAdress + Gameboy.ReadMemory( tileNumAddress ) * 16 ) }; //todo: Verify

			const uint8_t paletteResult{ ReadPalette( tileData, uint8_t( xWrap % 8 ), (*drawData)->tileY ) };
			(*fBuffer)[((*drawData)->fbOffset + x) * 2] = ((*drawData)->colors[paletteResult] & 2);
			(*fBuffer)[(((*drawData)->fbOffset + x) * 2) + 1] = ((*drawData)->colors[paletteResult] & 1);
		}

		(*drawData)->doneCounter |= (1 << id);
		mtx.lock(); //If not, possible deadlock
		//std::cout << int(id)<<" Released\n";
		ActivateDrawers.notify_all();
	}
}*/
