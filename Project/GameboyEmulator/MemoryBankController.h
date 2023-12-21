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

	virtual uint8_t ReadByte(const uint16_t address, const uint8_t* memory) const = 0;
	virtual void WriteByte(uint16_t address, uint8_t data) = 0;

	void SetRamBanks(std::vector<uint8_t>& ramBanks) { m_RamBanks = ramBanks; }

protected:
	constexpr bool InRange(const unsigned int value, const unsigned int min, const unsigned int max) const noexcept {
		return (value - min) <= (max - min);
	}

	std::vector<uint8_t> m_Rom{};
	std::vector<uint8_t> m_RamBanks{};

	uint32_t m_RomSize{};
	uint32_t m_NumRomBanks{};
	uint32_t m_RamSize{};
	uint32_t m_NumRamBanks{};

	uint8_t m_RomBank{ 1 };
	uint8_t m_RamBank{};
	bool m_RamBankEnabled{};
};