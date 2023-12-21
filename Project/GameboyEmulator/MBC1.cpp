#include "pch.h"
#include "MBC1.h"
#include <iostream>

MBC1::MBC1(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
	, m_BankingMode(false)
{
	//std::cout << "MBC1 constructor body\n";
	std::cout << "Rom size: " << m_RomSize << '\n';
	std::cout << "Num rom banks: " << m_NumRomBanks << '\n';
	std::cout << "Rom bank: " << unsigned(m_RomBank) << "\n\n";
	std::cout << "Ram size: " << m_RamSize << '\n';
	std::cout << "Num ram banks: " << m_NumRamBanks << '\n';
	std::cout << "Ram bank: " << unsigned(m_RamBank) << '\n';

	m_RamBanks.resize(m_NumRamBanks * 8 * 1024, 0);
}

uint8_t MBC1::ReadByte(const uint16_t address, const uint8_t* memory) const
{
	if (address <= 0x3FFF)
	{
		return m_Rom[address];
	}
	else if (InRange(address, 0x4000, 0x7FFF))
	{
		const uint32_t romBankOffset = (m_RomBank % m_NumRomBanks) * 0x4000;
		const uint32_t romAddress = romBankOffset + (address - 0x4000);
		return m_Rom[romAddress % m_RomSize];
		
	}
	else if (InRange(address, 0xA000, 0xBFFF))
	{
		if (m_BankingMode)
		{
			const uint32_t ramAddress = (m_RamBank * 0x2000) + (address - 0xA000);
			return m_RamBanks[ramAddress % m_RamSize];
		}
	}

	return memory[address];
}

void MBC1::WriteByte(uint16_t address, uint8_t data, uint8_t* memory)
{
	if (address <= 0x1FFF)
	{
		m_RamBankEnabled = ((data & 0xF) == 0xA);
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		data &= 0x1F;
		if (data == 0)
		{
			data = 1;
		}

		m_RomBank = (m_RomBank & 0xE0) | data;
	}
	else if (address >= 0x4000 && address <= 0x5FFF)
	{
		if (m_BankingMode)
		{
			m_RamBank = data & 0x03;
		}
		else
		{
			m_RomBank = ((data & 0x03) << 5) | (m_RomBank & 0x1F);
		}
	}
	else if (address >= 0x6000 && address <= 0x7FFF)
	{
		m_BankingMode = (data & 0x01) != 0;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (m_RamBankEnabled && !m_RamBanks.empty())
		{
			uint32_t ramAddress = m_RamBank * 0x2000;
			m_RamBanks[ramAddress + (address - 0xA000)] = data;
		}
	}
}