#include "pch.h"
#include "MBC0.h"
#include <iostream>

MBC0::MBC0(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
{
}

uint8_t MBC0::ReadByte(const uint16_t address, const uint8_t* memory) const
{
	//if (address <= 0x3FFF) //ROM Bank 0
	//{
	//	return m_Rom[address];
	//}

	//if (InRange(address, 0x4000, 0x7FFF)) //ROM Bank x
	//{
	//	return m_Rom[address + ((m_RomBank - 1) * 0x4000)];
	//}

	//if (InRange(address, 0xA000, 0xBFFF)) //RAM Bank x
	//{
	//	return m_RamBanks[(m_RamBank * 0x2000) + (address - 0xA000)];
	//}

	//return memory[address];

	if (address < 0x8000)
	{
		return m_Rom[address];
	}
	return 0;
}

void MBC0::WriteByte(uint16_t address, uint8_t data, uint8_t* memory)
{
	//if (address <= 0x1FFF) //Enable/Disable RAM
	//{
	//	if (mbc <= 1U || (mbc == 2U && !(address & 0x100)))
	//	{
	//		ramEnabled = ((data & 0xF) == 0xA);
	//	}

	//}
	//else if (InRange(address, (uint16_t)0x2000, (uint16_t)0x3FFF)) //5 bits;Lower 5 bits of ROM Bank
	//{
	//	if (mbc <= 1U || (mbc == 2U && address & 0x100))
	//	{
	//		romBank = ((data ? data : 1) & 0x1F);
	//	}
	//	
	//}
	//else if (InRange(address, (uint16_t)0x4000, (uint16_t)0x5FFF)) //2 bits;Ram or upper 2 bits of ROM bank
	//{
	//	ramBank = data;

	//}
	//else if (InRange(address, (uint16_t)0x6000, (uint16_t)0x7FFF)) //1 bit; Rom or Ram mode of ^
	//{
	//	isRam = data;

	//}
	//else if (InRange(address, (uint16_t)0xA000, (uint16_t)0xBFFF)) //External RAM
	//{
	//	if (ramEnabled)
	//	{
	//		ramBanks[(ramBank * 0x2000) + (address - 0xA000)] = data;
	//	}

	//}
}
