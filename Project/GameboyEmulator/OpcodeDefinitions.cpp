#include "pch.h"
#include "LR35902.h"
#include "GameBoy.h"
//Add the signature to LR35902.h

#pragma region ALU
//FINLINE void LR35902::ADD(uint8_t toAdd)
//{
//	auto result = Register.a + toAdd;
//
//	Register.halfCarryF = ((Register.a & 0xF) + (toAdd & 0xF)) > 0xF;
//	Register.carryF = (result > 0xFF);
//	Register.zeroF = ((result & 0xFF) == 0);
//	Register.subtractF = false;
//
//	Register.a = (result & 0xFF);
//}
//
//FINLINE void LR35902::ADDHL(uint8_t toAdd)
//{
//	ADD16(true, toAdd);
//}
//
//FINLINE void LR35902::ADD(uint16_t* destination, uint16_t toAdd)
//{
//	uint32_t result = *destination + toAdd;
//
//	Register.carryF = (result > 0xFFFF);
//	Register.halfCarryF = ((*destination & 0x0FFF) + (toAdd & 0x0FFF)) > 0x0FFF;
//
//	*destination = (uint16_t)result;
//
//	Register.subtractF = false;
//}

//FINLINE void LR35902::SUB(uint8_t toSub)
//{
//	Register.halfCarryF = ((Register.a - toSub) ^ toSub ^ Register.a) & 0x10;
//	Register.carryF = (int8_t(Register.a) - toSub) < 0;
//	Register.zeroF = !Register.a;
//	Register.subtractF = 1;
//	Register.a -= toSub;
//
//	/*uint8_t result = Register.a - toSub;
//	uint8_t bits = Register.a ^ toSub ^ result;
//
//	Register.a = result;
//
//	Register.subtractF = 1;
//	Register.zeroF = (Register.a == 0);
//	Register.carryF = ((bits & 0x100) != 0);
//	Register.halfCarryF = ((bits & 0x10) != 0);*/
//}

//Add Carry
FINLINE void LR35902::ADC( uint8_t toAdd, bool addCarry ) 
{
	if (addCarry && Register.carryF) 
	{
		++toAdd;
	}

	Register.halfCarryF = (((Register.a & 0xf) + (toAdd & 0xf)) & 0x10);
	Register.carryF = (uint16_t( Register.a + toAdd ) > 0xFF);

	Register.a += toAdd;

	Register.zeroF = !Register.a;
	Register.subtractF = 0;

	//if (addCarry && Register.carryF)
	//{
	//	++toAdd;
	//}
	////uint8_t carry = ( Register.carryF) ? 1 : 0;
	//uint8_t result = Register.a + toAdd;

	//Register.zeroF = (result == 0);
	//Register.carryF = (result > 0xFF);
	//Register.halfCarryF = ((Register.a & 0x0F) + (toAdd & 0x0F)) > 0x0F;
	//Register.subtractF = 0;

	//Register.a = result;

	//uint8_t before = toAdd;
	//uint8_t adding = toAdd;

	//// are we also adding the carry flag?
	//if (addCarry)
	//{
	//	if (TestBit(Register.f, 4))
	//	{
	//		adding++;
	//	}
	//}

	//toAdd += adding;

	//// set the flags
	//Register.f = 0;

	//if (toAdd == 0)
	//{
	//	Register.f = BitSet(Register.f, 7);
	//}

	//uint16_t htest = (before & 0xF);
	//htest += (adding & 0xF);

	//if (htest > 0xF)
	//{
	//	Register.f = BitSet(Register.f, 5);
	//}

	//if ((before + adding) > 0xFF)
	//{
	//	Register.f = BitSet(Register.f, 4);
	//}
}

/**
 * \param addToHL If False, it'll be added to SP and toAdd will be 8 bits signed
 */
FINLINE void LR35902::ADD16( bool addToHL, uint16_t toAdd ) {
	if (addToHL) 
	{
		Register.halfCarryF = (((Register.hl() & 0xfff) + (toAdd & 0xfff)) & 0x0FFF); //h is always 3->4 in the high byte
		Register.carryF = ((uint32_t( Register.hl() ) + toAdd) > 0xFFFF);

		Register.hl( Register.hl() + toAdd );
	} 
	else 
	{
		const int8_t toAddS{ int8_t( toAdd ) };
		if (toAddS >= 0) //Positive
		{ 
			Register.halfCarryF = (((Register.sp & 0xfff) + (toAdd & 0xfff)) & 0x1000); //h is always 3->4 in the high byte
			Register.carryF = ((uint32_t( Register.sp ) + toAdd) > 0xFFFF);
		} 
		else //Negative
		{ 
			Register.halfCarryF = (int16_t( Register.sp & 0xf ) + (toAddS & 0xf)) <
			                      0; // Check if subtracting the last 4 bits goes negative, indicating a borrow //Could also do ((Register.sp+toAddS)^toAddS^Register.sp)&0x10
			Register.carryF = ((uint32_t( Register.sp ) + toAddS) < 0); //If we go negative, it would cause an underflow on unsigned, setting the carry
		}
		Register.sp += toAddS;
		Register.zeroF = 0;
	}
	Register.subtractF = 0; //even tho we might've done a subtraction...
}

FINLINE void LR35902::SBC( uint8_t toSub, bool subCarry ) 
{
	uint8_t carry = (Register.f & Register.carryF) ? 1 : 0;

	if (subCarry)
		toSub += carry;

	int16_t result = Register.a - toSub - carry;

	Register.halfCarryF = ((Register.a & 0x0F) - (toSub & 0x0F) - carry) < 0;
	Register.carryF = (result < 0);
	Register.zeroF = ((result & 0xFF) == 0);
	Register.subtractF = true;

	Register.a = (result & 0xFF);
}

FINLINE void LR35902::OR( const uint8_t toOr ) {
	Register.f = 0;
	Register.a |= toOr;
	Register.zeroF = !Register.a;

}

FINLINE void LR35902::INC( uint8_t &toInc ) {
	Register.subtractF = 0;
	//Register.carryF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	Register.halfCarryF = uint8_t( (toInc & 0xF) == 0xF );
	Register.zeroF = !(++toInc);
}

FINLINE void LR35902::INC( uint16_t &toInc ) {
	//No flags for 16 bit variant
	++toInc;
}


FINLINE void LR35902::AND( uint8_t toAnd ) {
	Register.f = 0;
	Register.halfCarryF = 1; //Why tho?
	Register.a &= toAnd;
	Register.zeroF = !Register.a;
}

FINLINE void LR35902::DEC( uint8_t &toDec ) {
	Register.subtractF = 1;
	//Register.carryF; //Does not affect the carry flag! The INC/DEC opcode is often used to control loops; Loops are often used for multiple precision arithmetic so, to prevent having to push the carry state after every loop they just made the instruction ignore it! :D
	Register.halfCarryF = !(toDec & 0xF);
	Register.zeroF = !(--toDec);

}

FINLINE void LR35902::DEC( uint16_t &toDec ) { //No flags for 16 bit variant
	--toDec;
}


FINLINE void LR35902::XOR( const uint8_t toXor ) {
	Register.f = 0;
	Register.a ^= toXor;
	Register.zeroF = !Register.a;
}

FINLINE void LR35902::CP( uint8_t toCompare ) {
	Register.subtractF = 1;
	Register.zeroF = !(Register.a - toCompare);
	Register.carryF = (Register.a < toCompare);
	Register.halfCarryF = ((int16_t( Register.a & 0xF ) - (toCompare & 0xF)) < 0);
}

//DecimalAdjustA, implementation heavily inspired by Richeson's paper
FINLINE void LR35902::DAA() 
{
	//int newA{ Register.a };

	//if (!Register.subtractF) 
	//{
	//	if (Register.halfCarryF || (newA & 0xF) > 9) //if we did an initial overflow on the lower nibble or have exceeded 9 (the max value in BCD)
	//	{
	//		newA += 0x06; //Overflow (15-9)
	//	}

	//	if (Register.carryF || (newA > 0x90))
	//	{
	//		newA += 0x60; //same overflow for the higher nibble
	//	}
	//} 
	//else //The last operation was a subtraction, we need to honor this
	//{ 
	//	if (Register.halfCarryF) 
	//	{
	//		/*newA -= 0x06;
	//		newA &= 0xFF;*/
	//		newA = (newA - 0x06) & 0xFF;
	//	}
	//	if (Register.carryF)
	//	{
	//		newA -= 0x60;
	//	}
	//}

	//Register.halfCarryF = false;

	//if ((newA & 0x100) == 0x100)
	//{
	//	Register.carryF = true;
	//}

	//newA &= 0xFF;

	////Register.carryF = (newA > 0xFF);
	//Register.a = (uint8_t)newA;
	//Register.zeroF = (Register.a == 0);


	uint8_t newA{ Register.a };

	if (Register.subtractF)
	{
		if (Register.carryF) //if we did an initial overflow on the lower nibble or have exceeded 9 (the max value in BCD)
		{
			newA -= 0x60; //Overflow (15-9)
		}

		if (Register.halfCarryF)
		{
			newA -= 0x6; //same overflow for the higher nibble
		}
	}
	else //The last operation was a subtraction, we need to honor this
	{
		if (Register.carryF || newA > 0x99)
		{
			newA += 0x60;
			Register.carryF = true;
		}
		if (Register.halfCarryF || (newA & 0xF) > 0x9)
		{
			newA += 0x6;
		}
	}

	Register.a = newA;
	Register.carryF = !newA;
	Register.halfCarryF = false;
}
FINLINE void LR35902::POP_AF()
{
	//Register.af() = Gameboy->
	//PopFromStack(Register.af());
	//Register.f &= 0F0;

	Register.af(Gameboy.ReadMemoryWord(Register.sp));
	Register.f &= 0xF0;
	
}
FINLINE void LR35902::ClearFlags()
{
	Register.carryF = 0;
	Register.halfCarryF = 0;
	Register.subtractF = 0;
	Register.zeroF = 0;
}

FINLINE bool LR35902::TestBit(uint8_t reg, size_t pos)
{
	uint8_t lMsk = 1 << pos;
	return (reg & lMsk) ? true: false;
}

FINLINE uint8_t LR35902::BitSet(uint8_t reg, size_t pos)
{
	uint8_t lMsk = 1 << pos;
	reg |= lMsk;
	return reg;
}
#pragma endregion

#pragma region Loads
/**
 * \note writes \b DIRECTLY
 */
FINLINE void LR35902::LD( uint8_t &dest, const uint8_t data ) {
	dest = data;
}

FINLINE void LR35902::LD( uint16_t *const dest, const uint16_t data ) { *dest = data; }
FINLINE void LR35902::LD( const uint16_t destAddrs, const uint8_t data ) { Gameboy.WriteMemory( destAddrs, data ); }

FINLINE void LR35902::ADD(uint8_t toAdd)
{
	auto result = Register.a + toAdd;

	Register.halfCarryF = ((Register.a & 0xF) + (toAdd & 0xF)) > 0xF;
	Register.carryF = (result > 0xFF);
	Register.zeroF = ((result & 0xFF) == 0);
	Register.subtractF = false;

	Register.a = (result & 0xFF);
}

//FINLINE void LR35902::LD(uint16_t& high, uint16_t& low)
//{
//	low = Gameboy.ReadMemory(Register.pc++);
//	high = Gameboy.ReadMemory(Register.pc++);
//}

FINLINE void LR35902::PUSH( const uint16_t data ) { //little endian
	--Register.sp;
	Gameboy.WriteMemory( Register.sp, data >> 8 );
	--Register.sp;
	Gameboy.WriteMemory( Register.sp, data & 0xFF );
}

FINLINE void LR35902::POP( uint16_t &dest ) {
	dest = Gameboy.ReadMemory( Register.sp++ );
	dest |= (uint16_t( Gameboy.ReadMemory( Register.sp++ ) ) << 8);
}
#pragma endregion

#pragma region Rotates and Shifts
//RotateLeft
FINLINE void LR35902::RL(uint8_t& toRotate)
{
	/*Register.f = 0;

	const bool msb = bool(toRotate & 0x80);
	const bool originalCarry = Register.carryF;

	toRotate <<= 1;
	toRotate |= uint8_t(originalCarry);
	Register.carryF = msb;*/

	bool isCarrySet = TestBit(Register.f, 4);
	bool isMSBSet = TestBit(toRotate, 7);

	Register.f = 0;

	toRotate <<= 1;

	if (isMSBSet)
	{
		Register.f = BitSet(Register.f, 4);
	}

	if (isCarrySet)
	{
		toRotate = BitSet(toRotate, 0);
	}

	if (toRotate == 0)
	{
		Register.f = BitSet(Register.f, 7);
	}
}

//RotateLeftCarry
FINLINE void LR35902::RLC( uint8_t &toRotate ) {
	Register.f = 0;
	const bool msb{ bool( toRotate & 0x80 ) };

	toRotate <<= 1;
	toRotate |= uint8_t( msb );
	Register.carryF = msb;
	Register.zeroF = !toRotate;
}

//RotateRight
FINLINE void LR35902::RR( uint8_t &toRotate ) {
	const bool lsb{ bool( toRotate & 0x1 ) };
	toRotate >>= 1;
	toRotate |= (Register.carryF << 7);

	Register.f = 0;
	Register.carryF = lsb;
	Register.zeroF = !toRotate;
}

//RotateRightCarry
FINLINE void LR35902::RRCarry(uint8_t& toRotate)
{
	const bool lsb = bool(toRotate & 0x1);

	toRotate >>= 1;
	toRotate |= (lsb << 7);

	Register.f = 0;
	Register.carryF = lsb;
	Register.zeroF = !toRotate;
}

//ShiftLeftArithmetic (even though it's a logical shift..)
FINLINE void LR35902::SLA( uint8_t &toShift ) {
	/*Register.f = 0;
	Register.carryF = toShift & 0x80;
	toShift <<= 1;
	Register.zeroF = !toShift;*/

	bool isMSBSet = TestBit(toShift, 7);

	toShift <<= 1;

	Register.f = 0;

	if (isMSBSet)
	{
		Register.f = BitSet(Register.f, 4);
	}

	if (toShift == 0)
	{
		Register.f = BitSet(Register.f, 7);
	}
}

//ShiftRight
FINLINE void LR35902::SRA(uint8_t& toShift)
{
	/*Register.f = 0;
	const bool signBit = toShift & 0x80;
	toShift = (toShift >> 1) | signBit;
	Register.carryF = toShift & 0x1;
	Register.zeroF = !toShift;*/

	bool isLSBSet = TestBit(toShift, 0);
	bool isMSBSet = TestBit(toShift, 7);

	Register.f = 0;

	toShift >>= 1;

	if (isMSBSet)
	{
		toShift = BitSet(toShift, 7);
	}

	if (isLSBSet)
	{
		Register.f = BitSet(Register.f, 4);
	}

	if (toShift == 0)
	{
		Register.f = BitSet(Register.f, 7);
	}
}

//ShiftRightLogical
FINLINE void LR35902::SRL( uint8_t &toShift ) {
	Register.f = 0;
	Register.carryF = toShift & 0x1;
	toShift >>= 1;
	Register.zeroF = !toShift;
}
#pragma endregion

#pragma region Bits
FINLINE void LR35902::BIT( const uint8_t bit, const uint8_t data ) {
	Register.zeroF = !((data >> bit) & 1);
	Register.subtractF = 0;
	Register.halfCarryF = 1;
}

FINLINE void LR35902::RES( const uint8_t bit, uint8_t &data ) {
	data &= ~(1 << bit);
}

FINLINE void LR35902::SET( const uint8_t bit, uint8_t &data ) {
	data |= (1 << bit);
}
#pragma endregion

#pragma region Misc
FINLINE void LR35902::SWAP( uint8_t &data ) {
	data = (data >> 4) | (data << 4);
	Register.f = 0;
	Register.zeroF = !data;
}

//ComplementCarryFlag
FINLINE void LR35902::CCF() {
	Register.halfCarryF = Register.subtractF = 0;
	Register.carryF = !Register.carryF;
}

FINLINE void LR35902::SCF() {
	Register.subtractF = Register.halfCarryF = 0;
	Register.carryF = 1;
}

FINLINE void LR35902::HALT() { --Register.pc; } //Until an interrupt
FINLINE void LR35902::STOP() { --Register.pc; } //Until button press
FINLINE void LR35902::DI() { *(uint8_t*)&InteruptChangePending = 1; }
FINLINE void LR35902::EI() { *((uint8_t*)&InteruptChangePending) = 8; }
FINLINE void LR35902::NOP() {}

//ComPlemenT
FINLINE void LR35902::CPL() {
	Register.a = ~Register.a;
	Register.subtractF = Register.halfCarryF = 1;
}
#pragma endregion

#pragma region Calls and Jumps
/**
 * \note	<b>This function handles adding the cycles!</b>
 */
FINLINE void LR35902::CALL( const uint16_t address, bool doCall ) {
	if (doCall) {
		Gameboy.WriteMemoryWord( Register.sp -= 2, Register.pc );
		Register.pc = address;
		Gameboy.AddCycles( 24 ); //CodeSlinger:12
	} else
		Gameboy.AddCycles( 12 );
}
/**
 * \note	<b>This function handles adding the cycles!</b>
 */
FINLINE void LR35902::JR( const int8_t offset, bool doJump ) {
	if (doJump) {
		Register.pc += offset;
		Gameboy.AddCycles( 12 ); //CodeSlinger:8
	} else
		Gameboy.AddCycles( 8 );
}

FINLINE void LR35902::JP( const uint16_t address, const bool doJump, bool handleCycles ) {
	if (doJump) {
		Register.pc = address;
		Gameboy.AddCycles( handleCycles ? 16 : 0 ); //CodeSlinger:12:0
	} else
		Gameboy.AddCycles( handleCycles ? 12 : 0 );
}

FINLINE void LR35902::RET( bool doReturn, bool handleCycles ) {
	if (doReturn) {
		Register.pc = Gameboy.ReadMemoryWord( Register.sp );
		Gameboy.AddCycles( handleCycles ? 20 : 0 ); //CodeSlinger:8
	} else
		Gameboy.AddCycles( handleCycles ? 8 : 0 );
}

FINLINE void LR35902::RETI() {
	Register.pc = Gameboy.ReadMemoryWord( Register.sp );
	InteruptsEnabled = true;
}

FINLINE void LR35902::RST( const uint8_t address ) { //same as call.. (apart from address size)
	Gameboy.WriteMemoryWord( Register.sp -= 2, Register.pc );
	Register.pc = address;
}
#pragma endregion
