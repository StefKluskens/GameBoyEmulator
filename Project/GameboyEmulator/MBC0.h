#pragma once
#include "MemoryBankController.h"

class MBC0 : public MemoryBankController
{
public:
	MBC0(const std::vector<uint8_t>& rom);
	~MBC0() = default;

	MBC0(const MBC0& rhs) = delete; //Copy constructor
	MBC0(MBC0&& lhs) = delete; //Move Constructor
	MBC0& operator=(const MBC0& rhs) = delete; //Copy Assignment
	MBC0& operator=(MBC0&& lhs) = delete; //Move Assignment

	uint8_t ReadByte(const uint16_t address, const uint8_t* memory) const override;
	void WriteByte(uint16_t address, uint8_t data) override;
};

