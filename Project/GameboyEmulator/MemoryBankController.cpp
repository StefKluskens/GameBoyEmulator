#include "pch.h"
#include "MemoryBankController.h"
#include <iostream>

MemoryBankController::MemoryBankController(const std::vector<uint8_t>& rom)
	: m_Rom(rom)
{
	//Get ROM size
	switch (m_Rom[0x0148])
	{
	case 0x00:
		m_RomSize = 32 * 1024;
		m_NumRomBanks = 2;
		break;
	case 0x01:
		m_RomSize = 64 * 1024;
		m_NumRomBanks = 4;
		break;
	case 0x02:
		m_RomSize = 128 * 1024;
		m_NumRomBanks = 8;
		break;
	case 0x03:
		m_RomSize = 256 * 1024;
		m_NumRomBanks = 16;
		break;
	case 0x04:
		m_RomSize = 512 * 1024;
		m_NumRomBanks = 32;
		break;
	case 0x05:
		m_RomSize = 1024 * 1024;
		m_NumRomBanks = 64;
		break;
	case 0x06:
		m_RomSize = 2 * 1024 * 1024;
		m_NumRomBanks = 128;
		break;
	default:
		break;
	}

	//Get RAM size
	switch (m_Rom[0x0149])
	{
	case 0x00:
		m_RamSize = 0;
		m_NumRamBanks = 0;
		break;
	case 0x02:
		m_RamSize = 8 * 1024;
		m_NumRamBanks = 1;
		break;
	case 0x03:
		m_RamSize = 32 * 1024;
		m_NumRamBanks = 4;
		break;
	default:
		break;
	}
}