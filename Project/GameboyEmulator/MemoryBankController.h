#pragma once
#include <vector>
class MemoryBankController
{
public:
	MemoryBankController(std::vector<uint8_t>& rom);
	~MemoryBankController() = default;

	MemoryBankController(const MemoryBankController& rhs) = delete; //Copy constructor
	MemoryBankController(MemoryBankController&& lhs) = delete; //Move Constructor
	MemoryBankController& operator=(const MemoryBankController& rhs) = delete; //Copy Assignment
	MemoryBankController& operator=(MemoryBankController&& lhs) = delete; //Move Assignment

	virtual uint8_t Read(const uint16_t address, uint8_t& romBank, uint8_t& ramBank, std::vector<uint8_t>& ramBanks, bool& ramEnabled) const = 0;
	virtual void Write(uint16_t address, uint8_t data, uint8_t& romBank, uint8_t& ramBank, std::vector<uint8_t>& ramBanks, bool& ramEnabled) = 0;

	/*std::vector<uint8_t> GetRamBanks() const { return m_RamBanks; }
	void SetRamBanks(std::vector<uint8_t> ramBanks) { m_RamBanks = ramBanks; }*/

protected:
	constexpr bool InRange(const unsigned int value, const unsigned int min, const unsigned int max) const noexcept {
		return (value - min) <= (max - min);
	}

	std::vector<uint8_t> m_Rom{};
	//std::vector<uint8_t> m_RamBanks{};

	/*uint8_t m_RomBank = 1;
	uint8_t m_RamBank = 0;*/
	bool m_RamEnabled = false;
	//bool m_RamAccess = false;
};

