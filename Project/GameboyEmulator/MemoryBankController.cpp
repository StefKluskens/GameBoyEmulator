#include "pch.h"
#include "MemoryBankController.h"

MemoryBankController::MemoryBankController(std::vector<uint8_t>& rom)
	: m_Rom(rom)
{
	//m_RamBanks = std::vector<uint8_t>(0x8000);
}
