#pragma once
#include "MemoryBankController.h"

class MBC1 : public MemoryBankController
{
public:
	MBC1(const std::vector<uint8_t>& rom);
	~MBC1() = default;

	MBC1(const MBC1& rhs) = delete; //Copy constructor
	MBC1(MBC1&& lhs) = delete; //Move Constructor
	MBC1& operator=(const MBC1& rhs) = delete; //Copy Assignment
	MBC1& operator=(MBC1&& lhs) = delete; //Move Assignment

	// Inherited via MemoryBankController
	virtual uint8_t ReadByte(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, const uint8_t* memory, const bool ramEnabled, const std::vector<uint8_t>& ramBanks, const bool isRam) const override;
	virtual void WriteByte(uint16_t address, uint8_t data, uint8_t mbc, bool& ramEnabled, uint8_t& romBank, uint8_t& ramBank, bool& isRam, std::vector<uint8_t>& ramBanks) override;
};