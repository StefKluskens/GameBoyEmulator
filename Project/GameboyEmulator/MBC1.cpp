#include "pch.h"
#include "MBC1.h"

MBC1::MBC1()
	: MemoryBankController()
{
}

uint8_t MBC1::Read(const uint16_t address) const
{
	if (InRange(address, 0x0000, 0x3FFF))
	{

	}
	return 0;
}

void MBC1::Write(uint16_t address, uint8_t data)
{
}
