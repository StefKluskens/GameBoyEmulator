#pragma once
#include "MemoryBankController.h"

class MBC1 : public MemoryBankController
{
public:
	MBC1();
	~MBC1() = default;

	MBC1(const MBC1& rhs) = delete; //Copy constructor
	MBC1(MBC1&& lhs) = delete; //Move Constructor
	MBC1& operator=(const MBC1& rhs) = delete; //Copy Assignment
	MBC1& operator=(MBC1&& lhs) = delete; //Move Assignment

	uint8_t Read(const uint16_t address) const override;
	void Write(uint16_t address, uint8_t data) override;	
};

