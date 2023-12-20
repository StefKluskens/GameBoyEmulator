#include "pch.h"
#include "MBC1.h"
#include <iostream>

MBC1::MBC1(const std::vector<uint8_t>& rom)
	: MemoryBankController(rom)
{
}

uint8_t MBC1::ReadByte(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, const uint8_t* memory, const bool ramEnabled, const std::vector<uint8_t>& ramBanks, const bool isRam) const
{
	//std::cout << "ReadByte called for address " << std::hex << address << std::dec << std::endl;

	if (address <= 0x3FFF)
	{
		//std::cout << "Read from ROM Bank 0 at address " << std::hex << address << std::dec << std::endl;
		return m_Rom[address];
	}
	else if (InRange(address, 0x4000, 0x7FFF))
	{
		size_t offset = (romBank & 0x1F) * 0x4000;
		std::cout << "Read from ROM Bank " << (int)romBank << " at address " << std::hex << address << std::dec << std::endl;
		return m_Rom[address - 0x4000 + offset];
	}
	else if (InRange(address, 0xA000, 0xBFFF))
	{
		if (ramEnabled)
		{
			size_t ramAddress = (ramBank * 0x2000) + (address - 0xA000);
			std::cout << "Read from RAM Bank " << (int)ramBank << " at address " << std::hex << address << std::dec << std::endl;
			return ramBanks[ramAddress];
		}
		else
		{
			// RAM is not enabled, return a default value
			std::cout << "RAM not enabled at address " << std::hex << address << std::dec << std::endl;
			return 0x00;
		}
	}
	else
	{
		// Handle other cases
		//std::cout << "Read from unhandled address " << std::hex << address << std::dec << std::endl;
		return 0x00;
	}
}

void MBC1::WriteByte(uint16_t address, uint8_t data, uint8_t mbc, bool& ramEnabled, uint8_t& romBank, uint8_t& ramBank, bool& isRam, std::vector<uint8_t>& ramBanks)
{
	//std::cout << "Start of write function\n";
	if (InRange(address, 0x0000, 0x1FFF)) // Enable/Disable RAM
	{
		ramEnabled = !ramBanks.empty() && (((data & 0x0F) == 0x09) || ((data & 0x0F) == 0x00));
		/*std::cout << "WriteByte called for address " << std::hex << address << " with data " << (int)data << std::dec << std::endl;
		std::cout << "Lower 4 bits of data: " << std::hex << (int)(data & 0x0F) << std::dec << std::endl;*/
		std::cout << "RAM " << (ramEnabled ? "enabled" : "disabled") << " at address " << std::hex << address << std::dec << std::endl;
		return;
	}
	else if (InRange(address, 0x2000, 0x3FFF))
	{
		/*romBank = (romBank & 0xE0) | (data & 0x1F);
		if ((romBank & 0x1F) == 0 || (romBank & 0x1F) == 0x20 || (romBank & 0x1F) == 0x40)
		{
			romBank++;
		}*/
		romBank = data & 0x1F;
		if (romBank == 0)
		{
			romBank = 1;
		}
		std::cout << "Write to ROM Bank " << (int)romBank << " at address " << std::hex << address << std::dec << " from range 2000 & 3FFF\n";
		return;
	}
	else if (InRange(address, 0x4000, 0x5FFF))
	{
		//if (isRam)
		//{
		//	// If in RAM mode, set RAM bank number
		//	ramBank = data & 0x03;
		//	std::cout << "Write to RAM Bank " << (int)ramBank << " at address " << std::hex << address << std::dec << std::endl;
		//}
		//else
		//{
		//	// If in ROM mode, set upper bits of ROM bank
		//	romBank = (romBank & 0x1F) | ((data & 0x03) << 5);
		//	std::cout << "Write to ROM Bank " << (int)romBank << " at address " << std::hex << address << std::dec << " from range 4000 & 5FFF\n";
		//}
		ramBank = data & 0x03;
		std::cout << "Write to RAM Bank " << (int)ramBank << " at address " << std::hex << address << std::dec << " from range 4000 & 5FFF\n";
		return;
	}
	else if (InRange(address, 0x6000, 0x7FFF)) // RAM/ROM Mode Select
	{
		isRam = (data & 0x01) != 0;
		std::cout << "Write to RAM/ROM mode select: " << (isRam ? "RAM" : "ROM") << " at address " << std::hex << address << std::dec << std::endl;
		return;
	}
	else if (InRange(address, 0xA000, 0xBFFF)) // External RAM
	{
		if (ramEnabled)
		{
			size_t ramAddress = (ramBank * 0x2000) + (address - 0xA000);
			ramBanks[(address - 0xA000)] = data;
			std::cout << "Write to external RAM Bank " << (int)ramBank << " at address " << std::hex << address << std::dec << std::endl;
			return;
		}
	}
}
