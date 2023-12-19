#pragma once
class MemoryBankController
{
public:
	MemoryBankController();
	~MemoryBankController() = default;

	MemoryBankController(const MemoryBankController& rhs) = delete; //Copy constructor
	MemoryBankController(MemoryBankController&& lhs) = delete; //Move Constructor
	MemoryBankController& operator=(const MemoryBankController& rhs) = delete; //Copy Assignment
	MemoryBankController& operator=(MemoryBankController&& lhs) = delete; //Move Assignment

	virtual uint8_t Read(const uint16_t address) const = 0;
	virtual void Write(uint16_t address, uint8_t data) = 0;

protected:
	constexpr bool InRange(const unsigned int value, const unsigned int min, const unsigned int max) const noexcept {
		return (value - min) <= (max - min);
	}
};

