#include "pch.h"
#include "MBC1.h"
#include <iostream>

MBC1::MBC1(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
	, m_BankingMode(false)
{
	std::cout << "MBC1 constructor body\n";
	std::cout << "Rom size: " << m_RomSize << '\n';
	std::cout << "Num rom banks: " << m_NumRomBanks << '\n';
	std::cout << "Ram size: " << m_RamSize << '\n';
	std::cout << "Num ram banks: " << m_NumRamBanks << '\n';
}

uint8_t MBC1::ReadByte(const uint16_t address, const uint8_t* memory) const
{
	if (address <= 0x3FFF)
	{
		//std::cout << "ReadByte - Range 0x3FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		return m_Rom[address];
	}
	else if (address <= 0x7FFF)
	{
		//std::cout << "ReadByte - Range 0x7FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		uint32_t bankAddress = (m_RomBank % m_NumRomBanks) * 0x4000;
		return m_Rom[bankAddress + (address - 0x4000)];
	}
	else
	{
		if (m_RamBankEnabled && !m_RamBanks.empty())
		{
			uint32_t bankAddress = m_RamBank * 0x2000;
			return m_RamBanks[bankAddress + (address - 0xA000)];
		}
		return 0x00;
	}
	//return memory[address];
}

void MBC1::WriteByte(uint16_t address, uint8_t data)
{
	//std::cout << "WriteByte - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
	if (address <= 0x1FFF)
	{
		//std::cout << "WriteByte - Range 0x1FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;

		m_RamBankEnabled = (data & 0x0F) == 0x0A;
	}
	else if (address <= 0x3FFF)
	{
		//std::cout << "WriteByte - Range 0x3FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		data &= 0x1F;
		if (data == 0)
		{
			data = 1;
		}

		m_RomBank = (m_RomBank & 0xE0) | data;
	}
	else if (address <= 0x5FFF)
	{
		//std::cout << "WriteByte - Range 0x5FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		data &= 0x03;

		if (m_BankingMode)
		{
			m_RamBank = data;
		}
		else
		{
			m_RomBank = (m_RomBank & 0x1F) | (data << 5);
		}
	}
	else if (address <= 0x7FFF)
	{
		//std::cout << "WriteByte - Range 0x7FFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		m_BankingMode = data & 0x01;
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		//std::cout << "WriteByte - Range A000 - BFFF - Address: " << std::hex << address << ", ROM Bank: " << (int)m_RomBank << ", RAM Bank: " << (int)m_RamBank << std::dec << std::endl;
		if (m_RamBankEnabled)
		{
			uint32_t bankAddress = m_RamBank * 0x2000;
			m_RamBanks[bankAddress + (address - 0xA000)] = data;
		}
	}
}