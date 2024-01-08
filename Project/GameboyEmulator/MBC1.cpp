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

	m_RamBankEnabled = false;
	m_RamBanks.resize(m_NumRamBanks * 8 * 1024, 0);
}

uint8_t MBC1::ReadByte(const uint16_t address, const uint8_t* memory) const
{
	if (address < 0x4000)
	{
		int bank = m_BankingMode * (m_RamBank << 5) % m_NumRomBanks;
		return m_Rom[bank * 0x4000 + address];
	}
	else if (address < 0x8000)
	{
		int bank = ((m_RamBank << 5) | m_RomBank) % m_NumRomBanks;
		return m_Rom[bank * 0x4000 + address - 0x4000];
	}
	else if (address >= 0xA000 && address < 0xC000)
	{
		if (m_RamBankEnabled)
		{
			int bank = m_BankingMode * m_RamBank % m_NumRamBanks;
			return m_RamBanks[bank * 0x2000 + address - 0xA000];
		}
	}

	return 0xFF;
}

void MBC1::WriteByte(uint16_t address, uint8_t data, uint8_t* memory)
{
	if (address < 0x2000)
	{
		m_RamBankEnabled = (data & 0x0F) == 0xA;
	}
	else if (address < 0x4000)
	{
		data &= 0x1F;
		if (data == 0)
		{
			data = 1;
		}
		m_RomBank = data;
		//std::cout<<"Rom bank: " << unsigned(m_RomBank) << '\n';
	}
	else if (address < 0x6000)
	{
		m_RamBank = data & 0x3;
	}
	else if (address < 0x8000)
	{
		m_BankingMode = data & 0x1;
	}
	else if (address >= 0xA000 && address < 0xC000)
	{
		if (m_RamBankEnabled && !m_RamBanks.empty())
		{
			int bank = (m_RamBank * m_BankingMode) % m_NumRamBanks;
			m_RamBanks[bank * 0x2000 + address - 0xA000] = data;
		}
	}
	else
	{
		std::cout << "Not in write range\n";
	}
}