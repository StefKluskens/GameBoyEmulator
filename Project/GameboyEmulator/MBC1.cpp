#include "pch.h"
#include "MBC1.h"
#include <iostream>

MBC1::MBC1(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
	, m_BankingMode(false)
	, m_RomOffset(0x4000)
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

	//if (address <= 0x3FFF)
	//{
	//	int bank = m_BankingMode * (m_RamBank << 5) % m_NumRomBanks;
	//	return m_Rom[bank * 0x4000 + address];
	//}
	//else if (address >= 0x4000 && address <= 0x7FFF)
	//{
	//	//std::cout << "Rom bank: " << unsigned(m_RomBank) << "\n\n";
	//	int bank = ((m_RamBank << 5) | m_RomBank) % m_NumRomBanks;
	//	return m_Rom[bank * 0x4000 + address - 0x4000];
	//}
	//else if (address >= 0xA000 && address <= 0xBFFF)
	//{
	//	if (m_RamBankEnabled)
	//	{
	//		int bank = m_BankingMode * m_RamBank % m_NumRamBanks;
	//		return m_RamBanks[bank * 0x2000 + address - 0xA000];
	//	}
	//}

	//return 0xFF;
}

void MBC1::WriteByte(uint16_t address, uint8_t data, uint8_t* memory)
{
	/*if (address <= 0x1FFF)
	{
		m_RamBankEnabled = (data & 0x0F) == 0xA;
	}
	else if (address >= 0x2000 && address <= 0x3FFF)
	{
		data &= 0x1F;
		if (data == 0)
		{
			data = 1;
		}

		m_RomBank = data;
	}
	else if (address >= 0x4000 && address <= 0x5FFF)
	{
		m_RamBank = data & 0x03;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (m_RamBankEnabled && !m_RamBanks.empty())
		{
			m_RamBanks[(address - 0xA000) + m_RamOffset] = data;

			int bank = (m_RamBank * m_BankingMode) % m_NumRamBanks;
			m_RamBanks[bank * 0x2000 + address - 0xA000] = data;
		}
	}*/

	if (address <= 0x1FFF)
	{
		if ((data & 0xF) == 0xA)
		{
			m_RamBankEnabled = true;
		}
		else
		{
			m_RamBankEnabled = false;
		}
	}
	else if ((address >= 0x2000) && (address <= 0x3FFF))
	{
		if (data == 0x00)
		{
			data++;
		}

		data &= 31;

		// Turn off the lower 5-bits.
		m_RomBank &= 224;

		// Combine the written data with the register.
		m_RomBank |= data;
	}
	else if ((address >= 0x4000) && (address <= 0x5FFF))
	{
		if (m_UsingModel)
		{
			// in this mode we can only use Ram Bank 0
			m_RamBank = 0;

			data &= 3;
			data <<= 5;

			if ((m_RomBank & 31) == 0)
			{
				data++;
			}

			// Turn off bits 5 and 6, and 7 if it somehow got turned on.
			m_RomBank &= 31;

			// Combine the written data with the register.
			m_RomBank |= data;
		}
		else
		{
			m_RamBank = data & 0x3;
		}
	}
	else if ((address >= 0x6000) && (address <= 0x7FFF))
	{
		data &= 1;
		if (data == 1)
		{
			m_RamBank = 0;
			m_UsingModel = false;
		}
		else
		{
			m_UsingModel = true;
		}
	}
	else if ((address >= 0xA000) && (address <= 0xBFFF))
	{
		if (m_RamBankEnabled)
		{
			uint16_t newAddress = address - 0xA000;
			m_RamBanks.at(m_RamBank)[&newAddress] = data;
		}
	}
}