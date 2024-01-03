#pragma once
#include "MemoryBankController.h"

class MBC1 final : public MemoryBankController
{
public:
	MBC1(const std::vector<uint8_t>& rom);
	~MBC1() = default;

	MBC1(const MBC1& rhs) = delete; //Copy constructor
	MBC1(MBC1&& lhs) = delete; //Move Constructor
	MBC1& operator=(const MBC1& rhs) = delete; //Copy Assignment
	MBC1& operator=(MBC1&& lhs) = delete; //Move Assignment

	// Inherited via MemoryBankController
	virtual uint8_t ReadByte(const uint16_t address, const uint8_t* memory) const override;
	virtual void WriteByte(uint16_t address, uint8_t data, uint8_t* memory) override;

private:
	//If true, RAM-banking
	//If false, ROM-banking
	bool m_BankingMode{};
};