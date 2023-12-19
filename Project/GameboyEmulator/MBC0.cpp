#include "pch.h"
#include "MBC0.h"
#include <iostream>

MBC0::MBC0(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
{
}

uint8_t MBC0::ReadByte(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, const uint8_t* memory, const bool ramEnabled, const std::vector<uint8_t>& ramBanks, const bool isRam) const
{
	if (address <= 0x3FFF) //ROM Bank 0
	{
		return m_Rom[address];
	}

	if (InRange(address, 0x4000, 0x7FFF)) //ROM Bank x
	{
		return m_Rom[address + ((romBank - 1) * 0x4000)];
	}

	if (InRange(address, 0xA000, 0xBFFF)) //RAM Bank x
	{
		return m_RamBanks[(ramBank * 0x2000) + (address - 0xA000)];
	}

	return memory[address];
}

void MBC0::WriteByte(uint16_t address, uint8_t data, uint8_t mbc, bool& ramEnabled, uint8_t& romBank, uint8_t& ramBank, bool& isRam, std::vector<uint8_t>& ramBanks)
{
	if (address <= 0x1FFF) //Enable/Disable RAM
	{
		if (mbc <= 1U || (mbc == 2U && !(address & 0x100)))
		{
			ramEnabled = ((data & 0xF) == 0xA);
		}

	}
	else if (InRange(address, (uint16_t)0x2000, (uint16_t)0x3FFF)) //5 bits;Lower 5 bits of ROM Bank
	{
		if (mbc <= 1U || (mbc == 2U && address & 0x100))
		{
			romBank = ((data ? data : 1) & 0x1F);
		}
		
	}
	else if (InRange(address, (uint16_t)0x4000, (uint16_t)0x5FFF)) //2 bits;Ram or upper 2 bits of ROM bank
	{
		ramBank = data;

	}
	else if (InRange(address, (uint16_t)0x6000, (uint16_t)0x7FFF)) //1 bit; Rom or Ram mode of ^
	{
		isRam = data;

	}
	else if (InRange(address, (uint16_t)0xA000, (uint16_t)0xBFFF)) //External RAM
	{
		if (ramEnabled)
		{
			ramBanks[(ramBank * 0x2000) + (address - 0xA000)] = data;
		}

	}
}
