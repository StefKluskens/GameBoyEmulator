#pragma once
#include <vector>
class MemoryBankController
{
public:
	MemoryBankController(const std::vector<uint8_t>& rom);
	~MemoryBankController() = default;

	MemoryBankController(const MemoryBankController& rhs) = delete; //Copy constructor
	MemoryBankController(MemoryBankController&& lhs) = delete; //Move Constructor
	MemoryBankController& operator=(const MemoryBankController& rhs) = delete; //Copy Assignment
	MemoryBankController& operator=(MemoryBankController&& lhs) = delete; //Move Assignment

	virtual uint8_t ReadByte(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, const uint8_t* memory, const bool ramEnabled, const std::vector<uint8_t>& ramBanks, const bool isRam) const = 0;
	virtual void WriteByte(uint16_t address, uint8_t data, uint8_t mbc, bool& ramEnabled, uint8_t& romBank, uint8_t& ramBank, bool& isRam, std::vector<uint8_t>& ramBanks) = 0;

	void SetRamBanks(std::vector<uint8_t>& ramBanks) { m_RamBanks = ramBanks; }

protected:
	constexpr bool InRange(const unsigned int value, const unsigned int min, const unsigned int max) const noexcept {
		return (value - min) <= (max - min);
	}

	std::vector<uint8_t> m_Rom{};
	std::vector<uint8_t> m_RamBanks{};
};

