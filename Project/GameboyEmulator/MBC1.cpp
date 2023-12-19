#include "pch.h"
#include "MBC1.h"
#include <iostream>

MBC1::MBC1(std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
{
}

uint8_t MBC1::Read(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, std::vector<uint8_t>& ramBanks, bool& ramEnabled) const
{
	if (InRange(address, 0x0000, 0x3FFF))
	{
		return m_Rom[address];
	}
	else if (InRange(address, 0x4000, 0x7FFF))
	{
		uint8_t temp = romBank;
		int offset = address - 0x4000;
		int lookup = (temp * 0x4000) + offset;

		return m_Rom[lookup];
	}
	else if (InRange(address, 0xA000, 0xBFFF))
	{
		/*if (!m_RamAccess)
		{
			return 0xFF;
		}*/

		uint8_t temp{};
		if (ramEnabled)
		{
			temp = ramBank;
		}
		else
		{
			temp = 0x00;
		}

		int offset = address - 0xA000;
		int lookup = (temp * 0x2000) + offset;

		return ramBanks[lookup];
	}

	return m_Rom[address];
}

void MBC1::Write(uint16_t address, uint8_t data, uint8_t& romBank, uint8_t& ramBank, std::vector<uint8_t>& ramBanks, bool& ramEnabled)
{
	if (address <= 0x1FFF)
	{
		std::cout << "In first range write\n";
		if ((data & 0x0A) > 0)
		{
			ramEnabled = true;
		}
		else
		{
			ramEnabled = false;
		}
	}
	else if (InRange(address, (uint16_t)0x2000, (uint16_t)0x3FFF))
	{
		std::cout << "In second range write\n";
		uint8_t bankNumber = data & 0x1F;

		romBank = (romBank & 0xE0) | bankNumber;

		switch (romBank)
		{
		case 0x00:
		case 0x20:
		case 0x40:
		case 0x60:
			romBank++;
			break;
		default:
			break;
		}
	}
	else if (InRange(address, (uint16_t)0x4000, (uint16_t)0x5FFF))
	{
		std::cout << "In third range write\n";
		uint8_t bankNumber = data & 0x03;

		if (ramEnabled)
		{
			ramBank = bankNumber;
		}
		else
		{
			romBank = romBank | (bankNumber << 5);

			switch (romBank)
			{
			case 0x00:
			case 0x20:
			case 0x40:
			case 0x60:
				romBank++;
				break;
			default:
				break;
			}
		}
	}
	else if (InRange(address, (uint16_t)0x6000, (uint16_t)0x7FFF))
	{
		std::cout << "In fourth range write\n";
		if ((data >> 0) & 1)
		{
			ramEnabled = true;
		}
		else
		{
			ramEnabled = false;
		}
	}
	else if (InRange(address, (uint16_t)0xA000, (uint16_t)0xBFFF))
	{
		std::cout << "In fifth range write\n";
		if (ramEnabled)
		{
			int offset = address - 0xA000;
			int lookup = (ramBank * 0x2000) + offset;
			ramBanks[lookup] = data;
		}
	}

	//std::cout << "In no range write\n";
}
