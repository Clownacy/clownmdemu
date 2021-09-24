#include "m68k.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "error.h"

#define DEBUG_STUFF

#ifdef DEBUG_STUFF
#include <stdio.h>
#endif

#define SIGN_EXTEND(value, sign_bit) (((value) & ((1ul << (sign_bit)) - 1ul)) - ((value) & (1ul << (sign_bit))))
#define UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(value, mask) (((value) & (((unsigned long)(mask) >> 1) + 1)) ? -(long)(-(value) & (mask)) : (long)((value) & (mask)))
#define SIGNED_NATIVE_TO_UNSIGNED_TWOS_COMPLEMENT(value) ((value) < 0 ? -(unsigned long)-(value) : (unsigned long)(value))
#define UNIMPLEMENTED_INSTRUCTION(instruction) PrintError("Unimplemented instruction " instruction " used at 0x%X", state->program_counter)

typedef enum AddressMode
{
	ADDRESS_MODE_DATA_REGISTER                                = 0,
	ADDRESS_MODE_ADDRESS_REGISTER                             = 1,
	ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT                    = 2,
	ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT = 3,
	ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT  = 4,
	ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT  = 5,
	ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX         = 6,
	ADDRESS_MODE_SPECIAL                                      = 7
} AddressMode;

enum
{
	ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_SHORT                    = 0,
	ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_LONG                     = 1,
	ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_DISPLACEMENT = 2,
	ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX        = 3,
	ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE                         = 4
};

typedef enum Condition
{
	CONDITION_TRUE             =  0 << 8,
	CONDITION_FALSE            =  1 << 8,
	CONDITION_HIGHER           =  2 << 8,
	CONDITION_LOWER_OR_SAME    =  3 << 8,
	CONDITION_CARRY_CLEAR      =  4 << 8,
	CONDITION_CARRY_SET        =  5 << 8,
	CONDITION_NOT_EQUAL        =  6 << 8,
	CONDITION_EQUAL            =  7 << 8,
	CONDITION_OVERFLOW_CLEAR   =  8 << 8,
	CONDITION_OVERFLOW_SET     =  9 << 8,
	CONDITION_PLUS             = 10 << 8,
	CONDITION_MINUS            = 11 << 8,
	CONDITION_GREATER_OR_EQUAL = 12 << 8,
	CONDITION_LESS_THAN        = 13 << 8,
	CONDITION_GREATER_THAN     = 14 << 8,
	CONDITION_LESS_OR_EQUAL    = 15 << 8
} Condition;

enum
{
	CONDITION_CODE_CARRY    = 1 << 0,
	CONDITION_CODE_OVERFLOW = 1 << 1,
	CONDITION_CODE_ZERO     = 1 << 2,
	CONDITION_CODE_NEGATIVE = 1 << 3,
	CONDITION_CODE_EXTEND   = 1 << 4
};

typedef enum Instruction
{
	INSTRUCTION_ABCD,
	INSTRUCTION_ADD,
	INSTRUCTION_ADDA,
	INSTRUCTION_ADDI,
	INSTRUCTION_ADDQ,
	INSTRUCTION_ADDX,
	INSTRUCTION_AND,
	INSTRUCTION_ANDI,
	INSTRUCTION_ANDI_TO_CCR,
	INSTRUCTION_ANDI_TO_SR,
	INSTRUCTION_ASD_MEMORY,
	INSTRUCTION_ASD_REGISTER,
	INSTRUCTION_BCC,
	INSTRUCTION_BCHG_DYNAMIC,
	INSTRUCTION_BCHG_STATIC,
	INSTRUCTION_BCLR_DYNAMIC,
	INSTRUCTION_BCLR_STATIC,
	INSTRUCTION_BRA,
	INSTRUCTION_BSET_DYNAMIC,
	INSTRUCTION_BSET_STATIC,
	INSTRUCTION_BSR,
	INSTRUCTION_BTST_DYNAMIC,
	INSTRUCTION_BTST_STATIC,
	INSTRUCTION_CHK,
	INSTRUCTION_CLR,
	INSTRUCTION_CMP,
	INSTRUCTION_CMPA,
	INSTRUCTION_CMPI,
	INSTRUCTION_CMPM,
	INSTRUCTION_DBCC,
	INSTRUCTION_DIVS,
	INSTRUCTION_DIVU,
	INSTRUCTION_EOR,
	INSTRUCTION_EORI,
	INSTRUCTION_EORI_TO_CCR,
	INSTRUCTION_EORI_TO_SR,
	INSTRUCTION_EXG,
	INSTRUCTION_EXT,
	INSTRUCTION_ILLEGAL,
	INSTRUCTION_JMP,
	INSTRUCTION_JSR,
	INSTRUCTION_LEA,
	INSTRUCTION_LINK,
	INSTRUCTION_LSD_MEMORY,
	INSTRUCTION_LSD_REGISTER,
	INSTRUCTION_MOVE,
	INSTRUCTION_MOVE_FROM_SR,
	INSTRUCTION_MOVE_TO_CCR,
	INSTRUCTION_MOVE_TO_SR,
	INSTRUCTION_MOVE_USP,
	INSTRUCTION_MOVEA,
	INSTRUCTION_MOVEM,
	INSTRUCTION_MOVEP,
	INSTRUCTION_MOVEQ,
	INSTRUCTION_MULS,
	INSTRUCTION_MULU,
	INSTRUCTION_NBCD,
	INSTRUCTION_NEG,
	INSTRUCTION_NEGX,
	INSTRUCTION_NOP,
	INSTRUCTION_NOT,
	INSTRUCTION_OR,
	INSTRUCTION_ORI,
	INSTRUCTION_ORI_TO_CCR,
	INSTRUCTION_ORI_TO_SR,
	INSTRUCTION_PEA,
	INSTRUCTION_RESET,
	INSTRUCTION_ROD_MEMORY,
	INSTRUCTION_ROD_REGISTER,
	INSTRUCTION_ROXD_MEMORY,
	INSTRUCTION_ROXD_REGISTER,
	INSTRUCTION_RTE,
	INSTRUCTION_RTR,
	INSTRUCTION_RTS,
	INSTRUCTION_SBCD,
	INSTRUCTION_SCC,
	INSTRUCTION_STOP,
	INSTRUCTION_SUB,
	INSTRUCTION_SUBA,
	INSTRUCTION_SUBI,
	INSTRUCTION_SUBQ,
	INSTRUCTION_SUBX,
	INSTRUCTION_SWAP,
	INSTRUCTION_TAS,
	INSTRUCTION_TRAP,
	INSTRUCTION_TRAPV,
	INSTRUCTION_TST,
	INSTRUCTION_UNLK,

	INSTRUCTION_UNKNOWN
} Instruction;

/* Memory reads */

#define READ_BYTES(callbacks, address, value, total_bytes)\
{\
	unsigned int i;\
\
	value = 0;\
	for (i = 0; i < (total_bytes); ++i)\
		value |= (callbacks)->read_callback((callbacks)->user_data, ((address) + i) & 0xFFFFFFFF) << (((total_bytes) - 1 - i) * 8);\
}

static unsigned long ReadByte(const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	unsigned long value;

	READ_BYTES(callbacks, address, value, 1)

	return value;
}

static unsigned long ReadWord(const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	unsigned long value;

	READ_BYTES(callbacks, address, value, 2)

	return value;
}

static unsigned long ReadLongWord(const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	unsigned long value;

	READ_BYTES(callbacks, address, value, 4)

	return value;
}

#undef READ_BYTES

/* Memory writes */

#define WRITE_BYTES(callbacks, address, value, total_bytes)\
{\
	unsigned long i;\
\
	for (i = 0; i < (total_bytes); ++i)\
		(callbacks)->write_callback((callbacks)->user_data, ((address) + i) & 0xFFFFFFFF, ((value) >> (((total_bytes) - 1 - i) * 8)) & 0xFF);\
}

static void WriteByte(const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	WRITE_BYTES(callbacks, address, value, 1)
}

static void WriteWord(const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	WRITE_BYTES(callbacks, address, value, 2)
}

static void WriteLongWord(const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	WRITE_BYTES(callbacks, address, value, 4)
}

#undef WRITE_BYTES

/* Misc. utility */

static unsigned long DecodeMemoryAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned char operation_size_in_bytes, AddressMode address_mode, unsigned char reg)
{
	/* The stack pointer moves two bytes instead of one byte, for alignment purposes */
	const unsigned char increment_decrement_size = (reg == 7 && operation_size_in_bytes == 1) ? 2 : operation_size_in_bytes;

	unsigned long address;

	if (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_SHORT)
	{
		/* Absolute short */
		const unsigned short short_address = ReadWord(callbacks, state->program_counter);

		address = SIGN_EXTEND(short_address, 15); /* & 0xFFFFFFFF; TODO - Is this AND unnecessary? */
		state->program_counter += 2;
	}
	else if (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_LONG)
	{
		/* Absolute long */
		address = ReadLongWord(callbacks, state->program_counter);
		state->program_counter += 4;
	}
	else if (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
	{
		/* Immediate value */
		if (operation_size_in_bytes == 1)
		{
			/* A byte-sized immediate value occupies two bytes of space */
			address = state->program_counter + 1;
			state->program_counter += 2;
		}
		else
		{
			address = state->program_counter;
			state->program_counter += operation_size_in_bytes;
		}
	}
	else if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT)
	{
		/* Address register indirect */
		address = state->address_registers[reg];
	}
	else if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
	{
		/* Address register indirect with predecrement */
		state->address_registers[reg] -= increment_decrement_size;
		address = state->address_registers[reg];
	}
	else if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT)
	{
		/* Address register indirect with postincrement */
		address = state->address_registers[reg];
		state->address_registers[reg] += increment_decrement_size;
	}
	else
	{
		if (address_mode == ADDRESS_MODE_SPECIAL && (reg == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_DISPLACEMENT || reg == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX))
		{
			/* Program counter used as base address */
			address = state->program_counter;
		}
		else
		{
			/* Address register used as base address */
			address = state->address_registers[reg];
		}

		if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT || (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_DISPLACEMENT))
		{
			/* Add displacement */
			address += ReadWord(callbacks, state->program_counter);
			state->program_counter += 2;
		}
		else if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX || (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX))
		{
			/* Add index register and index literal */
			const unsigned short extension_word = ReadWord(callbacks, state->program_counter);
			const cc_bool is_address_register = !!(extension_word & 0x8000);
			const unsigned char displacement_reg = (extension_word >> 12) & 7;
			const cc_bool is_longword = !!(extension_word & 0x0800);
			const signed char displacement_literal_value = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(extension_word, 0xFF);
			const long displacement_reg_value = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE((is_address_register ? state->address_registers : state->data_registers)[displacement_reg], is_longword ? 0xFFFFFFFF : 0xFFFF);

			address += displacement_reg_value;
			address += displacement_literal_value;
			state->program_counter += 2;
		}
	}

	return address;
}

typedef struct DecodedAddressMode
{
	enum
	{
		DECODED_ADDRESS_MODE_TYPE_REGISTER,
		DECODED_ADDRESS_MODE_TYPE_MEMORY
	} type;
	union
	{
		struct
		{
			unsigned long *address;
			unsigned long operation_size_bitmask;
		} reg;
		struct
		{
			unsigned long address;
			unsigned char operation_size_in_bytes;
		} memory;
	} data;
} DecodedAddressMode;

static void DecodeAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode, unsigned char operation_size_in_bytes, AddressMode address_mode, unsigned char reg)
{
	switch (address_mode)
	{
		case ADDRESS_MODE_DATA_REGISTER:
		case ADDRESS_MODE_ADDRESS_REGISTER:
			/* Register */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_REGISTER;
			decoded_address_mode->data.reg.address = &(address_mode == ADDRESS_MODE_ADDRESS_REGISTER ? state->address_registers : state->data_registers)[reg];
			decoded_address_mode->data.reg.operation_size_bitmask = 0xFFFFFFFF >> (32 - operation_size_in_bytes * 8);
			break;

		default:
			/* Memory access */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_MEMORY;
			decoded_address_mode->data.memory.address = DecodeMemoryAddressMode(state, callbacks, operation_size_in_bytes, address_mode, reg);
			decoded_address_mode->data.memory.operation_size_in_bytes = operation_size_in_bytes;
			break;
	}
}

static unsigned long GetValueUsingDecodedAddressMode(const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode)
{
	unsigned long value = 0;

	switch (decoded_address_mode->type)
	{
		case DECODED_ADDRESS_MODE_TYPE_REGISTER:
			value = *decoded_address_mode->data.reg.address & decoded_address_mode->data.reg.operation_size_bitmask;
			break;

		case DECODED_ADDRESS_MODE_TYPE_MEMORY:
		{
			const unsigned long address = decoded_address_mode->data.memory.address;

			switch (decoded_address_mode->data.memory.operation_size_in_bytes)
			{
				case 1:
					value = ReadByte(callbacks, address);
					break;

				case 2:
					value = ReadWord(callbacks, address);
					break;

				case 4:
					value = ReadLongWord(callbacks, address);
					break;
			}

			break;
		}
	}

	return value;
}

static void SetValueUsingDecodedAddressMode(const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode, unsigned long value)
{
	switch (decoded_address_mode->type)
	{
		case DECODED_ADDRESS_MODE_TYPE_REGISTER:
		{
			const unsigned long destination_value = *decoded_address_mode->data.reg.address;
			const unsigned long operation_size_bitmask = decoded_address_mode->data.reg.operation_size_bitmask;
			*decoded_address_mode->data.reg.address = (value & operation_size_bitmask) | (destination_value & ~operation_size_bitmask);
			break;
		}

		case DECODED_ADDRESS_MODE_TYPE_MEMORY:
		{
			const unsigned long address = decoded_address_mode->data.memory.address;

			switch (decoded_address_mode->data.memory.operation_size_in_bytes)
			{
				case 1:
					WriteByte(callbacks, address, value);
					break;

				case 2:
					WriteWord(callbacks, address, value);
					break;

				case 4:
					WriteLongWord(callbacks, address, value);
					break;
			}

			break;
		}
	}
}

static cc_bool IsOpcodeConditionTrue(M68k_State *state, unsigned short opcode)
{
	const cc_bool carry = !!(state->status_register & CONDITION_CODE_CARRY);
	const cc_bool overflow = !!(state->status_register & CONDITION_CODE_OVERFLOW);
	const cc_bool zero = !!(state->status_register & CONDITION_CODE_ZERO);
	const cc_bool negative = !!(state->status_register & CONDITION_CODE_NEGATIVE);

	switch ((Condition)(opcode & 0x0F00))
	{
		case CONDITION_TRUE:
			return cc_true;

		case CONDITION_FALSE:
			return cc_false;

		case CONDITION_HIGHER:
			return !carry && !zero;

		case CONDITION_LOWER_OR_SAME:
			return carry || zero;

		case CONDITION_CARRY_CLEAR:
			return !carry;

		case CONDITION_CARRY_SET:
			return carry;

		case CONDITION_NOT_EQUAL:
			return !zero;

		case CONDITION_EQUAL:
			return zero;

		case CONDITION_OVERFLOW_CLEAR:
			return !overflow;

		case CONDITION_OVERFLOW_SET:
			return overflow;

		case CONDITION_PLUS:
			return !negative;

		case CONDITION_MINUS:
			return negative;

		case CONDITION_GREATER_OR_EQUAL:
			return (negative && overflow) || (!negative && !overflow);

		case CONDITION_LESS_THAN:
			return (negative && !overflow) || (!negative && overflow);

		case CONDITION_GREATER_THAN:
			return (negative && overflow && !zero) || (!negative && !overflow && !zero);

		case CONDITION_LESS_OR_EQUAL:
			return zero || (negative && !overflow) || (!negative && overflow);

		default:
			return cc_false;
	}
}

/* API */

void M68k_Reset(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks)
{
	state->address_registers[7] = ReadLongWord(callbacks, 0);
	state->program_counter = ReadLongWord(callbacks, 4);
}

void M68k_Interrupt(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned char level)
{
	assert(level >= 1 && level <= 7);

	state->address_registers[7] -= 4;
	WriteLongWord(callbacks, state->address_registers[7], state->program_counter);
	state->address_registers[7] -= 2;
	WriteWord(callbacks, state->address_registers[7], state->status_register);

	state->program_counter = ReadLongWord(callbacks, 24 + level * 4);
}

void M68k_DoCycle(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks)
{
	/* Wait for current instruction to finish */
	if (state->cycles_left_in_instruction != 0)
	{
		--state->cycles_left_in_instruction;
	}
	else
	{
		/* Process new instruction */
		const unsigned short opcode = ReadWord(callbacks, state->program_counter);

		const unsigned char opcode_bits_6_and_7 = (opcode >> 6) & 3;
		const cc_bool opcode_bit_8 = !!(opcode & 0x100);

		const unsigned char opcode_primary_register = (opcode >> 0) & 7;
		const AddressMode opcode_primary_address_mode = (opcode >> 3) & 7;
		const AddressMode opcode_secondary_address_mode = (opcode >> 6) & 7;
		const unsigned char opcode_secondary_register = (opcode >> 9) & 7;

		unsigned char operation_size;
		DecodedAddressMode source_decoded_address_mode, destination_decoded_address_mode;
		unsigned long source_value, destination_value, result_value;
		Instruction instruction = INSTRUCTION_UNKNOWN;

		state->program_counter += 2;

		/* Set to default values to shut up dumbass compiler warnings */
		operation_size = 1; /* Cannot be 0 otherwise a later subtraction will underflow */
		source_value = destination_value = result_value = 0;

		/* Figure out which instruction this is */
		switch (opcode & 0xF000)
		{
			case 0x0000:
				if (opcode_bit_8)
				{
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
					{
						instruction = INSTRUCTION_MOVEP;
					}
					else
					{
						switch (opcode_bits_6_and_7)
						{
							case 0:
								instruction = INSTRUCTION_BTST_DYNAMIC;
								break;

							case 1:
								instruction = INSTRUCTION_BCHG_DYNAMIC;
								break;

							case 2:
								instruction = INSTRUCTION_BCLR_DYNAMIC;
								break;

							case 3:
								instruction = INSTRUCTION_BSET_DYNAMIC;
								break;
						}
					}
				}
				else
				{
					switch (opcode_secondary_register)
					{
						case 0:
							if (opcode_primary_address_mode == ADDRESS_MODE_SPECIAL && opcode_primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
							{
								switch (opcode_bits_6_and_7)
								{
									case 0:
										instruction = INSTRUCTION_ORI_TO_CCR;
										break;

									case 1:
										instruction = INSTRUCTION_ORI_TO_SR;
										break;
								}
							}
							else
							{
								instruction = INSTRUCTION_ORI;
							}

							break;

						case 1:
							if (opcode_primary_address_mode == ADDRESS_MODE_SPECIAL && opcode_primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
							{
								switch (opcode_bits_6_and_7)
								{
									case 0:
										instruction = INSTRUCTION_ANDI_TO_CCR;
										break;

									case 1:
										instruction = INSTRUCTION_ANDI_TO_SR;
										break;
								}
							}
							else
							{
								instruction = INSTRUCTION_ANDI;
							}

							break;

						case 2:
							instruction = INSTRUCTION_SUBI;
							break;

						case 3:
							instruction = INSTRUCTION_ADDI;
							break;

						case 4:
							switch (opcode_bits_6_and_7)
							{
								case 0:
									instruction = INSTRUCTION_BTST_STATIC;
									break;

								case 1:
									instruction = INSTRUCTION_BCHG_STATIC;
									break;

								case 2:
									instruction = INSTRUCTION_BCLR_STATIC;
									break;

								case 3:
									instruction = INSTRUCTION_BSET_STATIC;
									break;
							}

							break;

						case 5:
							if (opcode_primary_address_mode == ADDRESS_MODE_SPECIAL && opcode_primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
							{
								switch (opcode_bits_6_and_7)
								{
									case 0:
										instruction = INSTRUCTION_EORI_TO_CCR;
										break;

									case 1:
										instruction = INSTRUCTION_EORI_TO_SR;
										break;
								}
							}
							else
							{
								instruction = INSTRUCTION_EORI;
							}

							break;

						case 6:
							instruction = INSTRUCTION_CMPI;
							break;
					}
				}

				break;

			case 0x1000:
			case 0x2000:
			case 0x3000:
				if ((opcode & 0x01C0) == 0x0040)
					instruction = INSTRUCTION_MOVEA;
				else
					instruction = INSTRUCTION_MOVE;

				break;

			case 0x4000:
				if (opcode_bit_8)
				{
					switch (opcode_bits_6_and_7)
					{
						case 3:
							instruction = INSTRUCTION_LEA;
							break;

						case 2:
							instruction = INSTRUCTION_CHK;
							break;

						default:
							break;
					}
				}
				else if (!(opcode & 0x0800))
				{
					if (opcode_bits_6_and_7 == 3)
					{
						switch (opcode_secondary_register)
						{
							case 0:
								instruction = INSTRUCTION_MOVE_FROM_SR;
								break;

							case 2:
								instruction = INSTRUCTION_MOVE_TO_CCR;
								break;

							case 3:
								instruction = INSTRUCTION_MOVE_TO_SR;
								break;
						}
					}
					else
					{
						switch (opcode_secondary_register)
						{
							case 0:
								instruction = INSTRUCTION_NEGX;
								break;

							case 1:
								instruction = INSTRUCTION_CLR;
								break;

							case 2:
								instruction = INSTRUCTION_NEG;
								break;

							case 3:
								instruction = INSTRUCTION_NOT;
								break;
						}
					}
				}
				else if ((opcode & 0x0200) == 0)
				{
					if ((opcode & 0x01B8) == 0x0080)
						instruction = INSTRUCTION_EXT;
					else if ((opcode & 0x01C0) == 0x0000)
						instruction = INSTRUCTION_NBCD;
					else if ((opcode & 0x01F8) == 0x0040)
						instruction = INSTRUCTION_SWAP;
					else if ((opcode & 0x01C0) == 0x0040)
						instruction = INSTRUCTION_PEA;
					else if ((opcode & 0x0B80) == 0x0880)
						instruction = INSTRUCTION_MOVEM;
				}
				else if (opcode == 0x4AFC)
				{
					instruction = INSTRUCTION_ILLEGAL;
				}
				else if ((opcode & 0x0FC0) == 0x0AC0)
				{
					instruction = INSTRUCTION_TAS;
				}
				else if ((opcode & 0x0F00) == 0x0A00)
				{
					instruction = INSTRUCTION_TST;
				}
				else if ((opcode & 0x0FF0) == 0x0E40)
				{
					instruction = INSTRUCTION_TRAP;
				}
				else if ((opcode & 0x0FF8) == 0x0E50)
				{
					instruction = INSTRUCTION_LINK;
				}
				else if ((opcode & 0x0FF8) == 0x0E58)
				{
					instruction = INSTRUCTION_UNLK;
				}
				else if ((opcode & 0x0FF0) == 0x0E60)
				{
					instruction = INSTRUCTION_MOVE_USP;
				}
				else if ((opcode & 0x0FF8) == 0x0E70)
				{
					switch (opcode_primary_register)
					{
						case 0:
							instruction = INSTRUCTION_RESET;
							break;

						case 1:
							instruction = INSTRUCTION_NOP;
							break;

						case 2:
							instruction = INSTRUCTION_STOP;
							break;

						case 3:
							instruction = INSTRUCTION_RTE;
							break;

						case 5:
							instruction = INSTRUCTION_RTS;
							break;

						case 6:
							instruction = INSTRUCTION_TRAPV;
							break;

						case 7:
							instruction = INSTRUCTION_RTR;
							break;
					}
				}
				else if ((opcode & 0x0FC0) == 0x0E80)
				{
					instruction = INSTRUCTION_JSR;
				}
				else if ((opcode & 0x0FC0) == 0x0EC0)
				{
					instruction = INSTRUCTION_JMP;
				}

				break;

			case 0x5000:
				if (opcode_bits_6_and_7 == 3)
				{
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
						instruction = INSTRUCTION_DBCC;
					else
						instruction = INSTRUCTION_SCC;
				}
				else
				{
					if (opcode_bit_8)
						instruction = INSTRUCTION_SUBQ;
					else
						instruction = INSTRUCTION_ADDQ;
				}

				break;

			case 0x6000:
				if (opcode_secondary_register == 0)
				{
					if (opcode_bit_8)
						instruction = INSTRUCTION_BSR;
					else
						instruction = INSTRUCTION_BRA;
				}
				else
				{
					instruction = INSTRUCTION_BCC;
				}

				break;

			case 0x7000:
				instruction = INSTRUCTION_MOVEQ;
				break;

			case 0x8000:
				if (opcode_bits_6_and_7 == 3)
				{
					if (opcode_bit_8)
						instruction = INSTRUCTION_DIVS;
					else
						instruction = INSTRUCTION_DIVU;
				}
				else
				{
					if ((opcode & 0x0170) == 0x0100)
						instruction = INSTRUCTION_SBCD;
					else
						instruction = INSTRUCTION_OR;
				}

				break;

			case 0x9000:
				if (opcode_bits_6_and_7 == 3)
					instruction = INSTRUCTION_SUBA;
				else if ((opcode & 0x0170) == 0x0100)
					instruction = INSTRUCTION_SUBX;
				else
					instruction = INSTRUCTION_SUB;

				break;

			case 0xB000:
				if (opcode_bits_6_and_7 == 3)
					instruction = INSTRUCTION_CMPA;
				else if (!opcode_bit_8)
					instruction = INSTRUCTION_CMP;
				else if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
					instruction = INSTRUCTION_CMPM;
				else
					instruction = INSTRUCTION_EOR;

				break;

			case 0xC000:
				if (opcode_bits_6_and_7 == 3)
				{
					if (opcode_bit_8)
						instruction = INSTRUCTION_MULS;
					else
						instruction = INSTRUCTION_MULU;
				}
				else if ((opcode & 0x0170) == 0x0100)
				{
					if (opcode_bits_6_and_7 == 0)
						instruction = INSTRUCTION_ABCD;
					else
						instruction = INSTRUCTION_EXG;
				}
				else
				{
					instruction = INSTRUCTION_AND;
				}

				break;

			case 0xD000:
				if (opcode_bits_6_and_7 == 3)
					instruction = INSTRUCTION_ADDA;
				else if ((opcode & 0x0170) == 0x0100)
					instruction = INSTRUCTION_ADDX;
				else
					instruction = INSTRUCTION_ADD;
				
				break;

			case 0xE000:
				if (opcode_bits_6_and_7 == 3)
				{
					switch (opcode_secondary_register)
					{
						case 0:
							instruction = INSTRUCTION_ASD_MEMORY;
							break;

						case 1:
							instruction = INSTRUCTION_LSD_MEMORY;
							break;

						case 2:
							instruction = INSTRUCTION_ROXD_MEMORY;
							break;

						case 3:
							instruction = INSTRUCTION_ROD_MEMORY;
							break;
					}
				}
				else
				{
					switch (opcode & 0x0018)
					{
						case 0x0000:
							instruction = INSTRUCTION_ASD_REGISTER;
							break;

						case 0x0008:
							instruction = INSTRUCTION_LSD_REGISTER;
							break;

						case 0x0010:
							instruction = INSTRUCTION_ROXD_REGISTER;
							break;

						case 0x0018:
							instruction = INSTRUCTION_ROD_REGISTER;
							break;
					}
				}
				
				break;
		}

		/* Determine operation sizes */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_TAS:
			case INSTRUCTION_SCC:
			case INSTRUCTION_SBCD:
			case INSTRUCTION_ABCD:
				/* Hardcoded to a byte */
				operation_size = 1;
				break;

			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_LINK:
			case INSTRUCTION_MOVEM:
			case INSTRUCTION_CHK:
			case INSTRUCTION_DBCC:
			case INSTRUCTION_DIVU:
			case INSTRUCTION_DIVS:
			case INSTRUCTION_MULU:
			case INSTRUCTION_MULS:
			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
			case INSTRUCTION_STOP:
				/* Hardcoded to a word */
				operation_size = 2;
				break;

			case INSTRUCTION_SWAP:
			case INSTRUCTION_LEA:
			case INSTRUCTION_MOVEQ:
				/* Hardcoded to a longword */
				operation_size = 4;
				break;

			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
				/* 4 if register - 1 if memory */
				operation_size = opcode_primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;
				break;

			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE:
				/* Derived from an odd bitfield */
				switch (opcode & 0x3000)
				{
					case 0x1000:
						operation_size = 1;
						break;

					case 0x2000:
						operation_size = 4; /* Yup, this isn't a typo */
						break;

					case 0x3000:
						operation_size = 2;
						break;
				}

				break;

			case INSTRUCTION_EXT:
				operation_size = opcode & 0x0040 ? 4 : 2;
				break;

			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
				if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
					operation_size = 4;
				else
					operation_size = 1 << opcode_bits_6_and_7;

				break;

			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_ADDA:
				/* Word or longword based on bit 8 */
				operation_size = opcode_bit_8 ? 4 : 2;
				break;

			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_SUBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_EORI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_CLR:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_TST:
			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_EOR:
			case INSTRUCTION_CMPM:
			case INSTRUCTION_CMP:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
			case INSTRUCTION_ADDX:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
				/* Standard */
				operation_size = 1 << opcode_bits_6_and_7;
				break;

			case INSTRUCTION_MOVEP:
			case INSTRUCTION_PEA:
			case INSTRUCTION_ILLEGAL:
			case INSTRUCTION_TRAP:
			case INSTRUCTION_UNLK:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_RESET:
			case INSTRUCTION_NOP:
			case INSTRUCTION_RTE:
			case INSTRUCTION_RTS:
			case INSTRUCTION_TRAPV:
			case INSTRUCTION_RTR:
			case INSTRUCTION_JSR:
			case INSTRUCTION_JMP:
			case INSTRUCTION_BRA:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BCC:
			case INSTRUCTION_EXG:
			case INSTRUCTION_UNKNOWN:
				/* Doesn't have any sizes */
				break;
		}

		/* Obtain source value */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_SUBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_EORI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_LINK:
			case INSTRUCTION_MOVEM:
			case INSTRUCTION_DBCC:
			case INSTRUCTION_STOP:
				/* Immediate value (any size) */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
			case INSTRUCTION_EOR:
				/* Secondary data register */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
				/* Immediate value (byte) */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVE_FROM_SR:
				/* Status register */
				source_value = state->status_register;
				break;

			case INSTRUCTION_PEA:
			case INSTRUCTION_JSR:
			case INSTRUCTION_JMP:
			case INSTRUCTION_LEA:
				/* Effective address */
				source_value = DecodeMemoryAddressMode(state, callbacks, 0, opcode_primary_address_mode, opcode_primary_register);
				break;

			case INSTRUCTION_BRA:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BCC:
				if ((opcode & 0x00FF) == 0)
				{
					DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
					source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				}

				break;

			case INSTRUCTION_SBCD:
			case INSTRUCTION_ABCD:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_ADDX:
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode_primary_register);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
				/* Primary address mode or secondary data register, based on direction bit */
				if (opcode_bit_8)
					DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
				else
					DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode_primary_address_mode, opcode_primary_register);

				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
					
				break;

			case INSTRUCTION_CMPM:
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode_primary_register);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE:
			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_CHK:
			case INSTRUCTION_DIVU:
			case INSTRUCTION_DIVS:
			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMP:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_MULU:
			case INSTRUCTION_MULS:
			case INSTRUCTION_ADDA:
				/* Primary address mode */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode_primary_address_mode, opcode_primary_register);
				source_value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVEP:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_CLR:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_EXT:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_SWAP:
			case INSTRUCTION_ILLEGAL:
			case INSTRUCTION_TAS:
			case INSTRUCTION_TST:
			case INSTRUCTION_TRAP:
			case INSTRUCTION_UNLK:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_RESET:
			case INSTRUCTION_NOP:
			case INSTRUCTION_RTE:
			case INSTRUCTION_RTS:
			case INSTRUCTION_TRAPV:
			case INSTRUCTION_RTR:
			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
			case INSTRUCTION_SCC:
			case INSTRUCTION_MOVEQ:
			case INSTRUCTION_EXG:
			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
			case INSTRUCTION_UNKNOWN:
				/* Doesn't have a source value */
				break;
		}

		/* Decode destination address mode */
		switch (instruction)
		{
			case INSTRUCTION_EXT:
			case INSTRUCTION_SWAP:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
				/* Data register (primary) */
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode_primary_register);
				break;

			case INSTRUCTION_MOVEQ:
			case INSTRUCTION_CMP:
				/* Data register (secondary) */
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
				break;

			case INSTRUCTION_LEA:
				/* Address register (secondary) */
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER, opcode_secondary_register);
				break;

			case INSTRUCTION_MOVE:
			case INSTRUCTION_MOVEP:
			case INSTRUCTION_MOVEA:
				/* Secondary address mode */ 
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode_secondary_address_mode, opcode_secondary_register);				
				break;

			case INSTRUCTION_SBCD:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_ABCD:
			case INSTRUCTION_ADDX:
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
				break;

			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
				/* Primary address mode or secondary data register, based on direction bit */
				if (opcode_bit_8)
					DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode_primary_address_mode, opcode_primary_register);
				else
					DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
					
				break;

			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_ADDA:
				/* Full secondary address register */
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode_secondary_register);
				break;

			case INSTRUCTION_CMPM:
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode_secondary_register);
				break;

			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_SUBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_EORI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_TAS:
			case INSTRUCTION_TST:
			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_CLR:
			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
			case INSTRUCTION_SCC:
			case INSTRUCTION_EOR:
			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
				/* Using primary address mode */
				DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode_primary_address_mode, opcode_primary_register);
				break;

			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_PEA:
			case INSTRUCTION_ILLEGAL:
			case INSTRUCTION_TRAP:
			case INSTRUCTION_LINK:
			case INSTRUCTION_UNLK:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_RESET:
			case INSTRUCTION_NOP:
			case INSTRUCTION_STOP:
			case INSTRUCTION_RTE:
			case INSTRUCTION_RTS:
			case INSTRUCTION_TRAPV:
			case INSTRUCTION_RTR:
			case INSTRUCTION_JSR:
			case INSTRUCTION_JMP:
			case INSTRUCTION_MOVEM:
			case INSTRUCTION_CHK:
			case INSTRUCTION_DBCC:
			case INSTRUCTION_BRA:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BCC:
			case INSTRUCTION_DIVU:
			case INSTRUCTION_DIVS:
			case INSTRUCTION_MULU:
			case INSTRUCTION_MULS:
			case INSTRUCTION_EXG:
			case INSTRUCTION_UNKNOWN:
				/* Doesn't have a destination address mode to decode */
				break;
		}

		/* Obtain destination value */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_EORI_TO_SR:
				/* Read from status register */
				destination_value = state->status_register;
				break;

			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_SUBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_EORI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_TAS:
			case INSTRUCTION_TST:
			case INSTRUCTION_EXT:
			case INSTRUCTION_SWAP:
			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
			case INSTRUCTION_SBCD:
			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_EOR:
			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMPM:
			case INSTRUCTION_CMP:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_ABCD:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
			case INSTRUCTION_ADDX:
			case INSTRUCTION_ADDA:
			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
				/* Read destination from decoded address mode */
				destination_value = GetValueUsingDecodedAddressMode(callbacks, &destination_decoded_address_mode);
				break;

			case INSTRUCTION_MOVE:
			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_PEA:
			case INSTRUCTION_ILLEGAL:
			case INSTRUCTION_TRAP:
			case INSTRUCTION_LINK:
			case INSTRUCTION_UNLK:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_RESET:
			case INSTRUCTION_NOP:
			case INSTRUCTION_STOP:
			case INSTRUCTION_RTE:
			case INSTRUCTION_RTS:
			case INSTRUCTION_TRAPV:
			case INSTRUCTION_RTR:
			case INSTRUCTION_JSR:
			case INSTRUCTION_JMP:
			case INSTRUCTION_MOVEM:
			case INSTRUCTION_MOVEP:
			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_CLR:
			case INSTRUCTION_LEA:
			case INSTRUCTION_CHK:
			case INSTRUCTION_SCC:
			case INSTRUCTION_DBCC:
			case INSTRUCTION_BRA:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BCC:
			case INSTRUCTION_MOVEQ:
			case INSTRUCTION_DIVU:
			case INSTRUCTION_DIVS:
			case INSTRUCTION_MULU:
			case INSTRUCTION_MULS:
			case INSTRUCTION_EXG:
			case INSTRUCTION_UNKNOWN:
				/* Doesn't read its destination (if it even has one) */
				break;
		}

		/* Do the actual instruction */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ORI:
			case INSTRUCTION_OR:
				result_value = destination_value | source_value;
				break;

			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_AND:
				result_value = destination_value & source_value;
				break;

			case INSTRUCTION_SUBI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_SUB:
			case INSTRUCTION_CMPM:
			case INSTRUCTION_CMP:
				result_value = destination_value - source_value;
				break;

			case INSTRUCTION_ADDI:
			case INSTRUCTION_ADD:
				result_value = destination_value + source_value;
				break;

			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_EORI:
			case INSTRUCTION_EOR:
				result_value = destination_value ^ source_value;
				break;

			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
				/* Set the zero flag to the specified bit */
				state->status_register &= ~CONDITION_CODE_ZERO;
				state->status_register |= CONDITION_CODE_ZERO * !(destination_value & (1ul << source_value));

				switch (instruction)
				{
					case INSTRUCTION_BTST_STATIC:
					case INSTRUCTION_BTST_DYNAMIC:
						break;

					case INSTRUCTION_BCHG_STATIC:
					case INSTRUCTION_BCHG_DYNAMIC:
						result_value = destination_value ^ (1ul << source_value);
						break;

					case INSTRUCTION_BCLR_STATIC:
					case INSTRUCTION_BCLR_DYNAMIC:
						result_value = destination_value & ~(1ul << source_value);
						break;

					case INSTRUCTION_BSET_STATIC:
					case INSTRUCTION_BSET_DYNAMIC:
						result_value = destination_value | (1ul << source_value);
						break;

					default:
						/* Shut up stupid compiler warnings */
						break;
				}

				break;


			case INSTRUCTION_MOVEP:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("MOVEP");
				break;

			case INSTRUCTION_MOVEA:
				result_value = operation_size == 2 ? SIGN_EXTEND(source_value, 15) : source_value;
				break;

			case INSTRUCTION_MOVE:
			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_LEA:
			case INSTRUCTION_TST:
				result_value = source_value;
				break;

			case INSTRUCTION_LINK:
				/* Push address register to stack */
				state->address_registers[7] -= 4;
				WriteLongWord(callbacks, state->address_registers[7], state->address_registers[opcode_primary_register]);

				/* Copy stack pointer to address register */
				state->address_registers[opcode_primary_register] = state->address_registers[7];

				/* Offset the stack pointer by the immediate value */
				state->address_registers[7] += UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(source_value, 0xFFFF);

				break;

			case INSTRUCTION_UNLK:
				state->address_registers[7] = state->address_registers[opcode_primary_register];
				state->address_registers[opcode_primary_register] = ReadLongWord(callbacks, state->address_registers[7]);
				state->address_registers[7] += 4;
				break;

			case INSTRUCTION_NEGX:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("NEGX");
				break;

			case INSTRUCTION_CLR:
				result_value = 0;
				break;

			case INSTRUCTION_NEG:
				result_value = -destination_value;
				break;

			case INSTRUCTION_NOT:
				result_value = ~destination_value;
				break;

			case INSTRUCTION_EXT:
				result_value = SIGN_EXTEND(destination_value, opcode & 0x0040 ? 15 : 7);
				break;

			case INSTRUCTION_NBCD:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("NBCD");
				break;

			case INSTRUCTION_SWAP:
				result_value = ((destination_value & 0x0000FFFF) << 16) | ((destination_value & 0xFFFF0000) >> 16);
				break;

			case INSTRUCTION_PEA:
				state->address_registers[7] -= 4;
				WriteLongWord(callbacks, state->address_registers[7], source_value);
				break;

			case INSTRUCTION_ILLEGAL:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("ILLEGAL");
				break;

			case INSTRUCTION_TAS:
				/* TODO - This instruction doesn't work properly on memory on the Mega Drive */
				state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);
				state->status_register |= CONDITION_CODE_NEGATIVE * !!(destination_value & 0x80);
				state->status_register |= CONDITION_CODE_ZERO * !!(destination_value == 0);

				result_value = destination_value | 0x80;
				break;

			case INSTRUCTION_TRAP:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("TRAP");
				break;

			case INSTRUCTION_MOVE_USP:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("MOVE USP");
				break;

			case INSTRUCTION_RESET:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("RESET");
				break;

			case INSTRUCTION_NOP:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("NOP");
				break;

			case INSTRUCTION_STOP:
				/* TODO */
				/*if (supervisor_mode)
					state->status_register = source_value;
				else
					TRAP();*/
				UNIMPLEMENTED_INSTRUCTION("STOP");

				break;

			case INSTRUCTION_RTE:
				state->status_register = ReadWord(callbacks, state->address_registers[7]);
				state->address_registers[7] += 2;
				state->program_counter = ReadLongWord(callbacks, state->address_registers[7]);
				state->address_registers[7] += 4;				
				break;

			case INSTRUCTION_RTS:
				state->program_counter = ReadLongWord(callbacks, state->address_registers[7]);
				state->address_registers[7] += 4;				
				break;

			case INSTRUCTION_TRAPV:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("TRAPV");
				break;

			case INSTRUCTION_RTR:
				state->status_register = (state->status_register & 0xFF00) | ReadByte(callbacks, state->address_registers[7]);
				state->address_registers[7] += 2;
				state->program_counter = ReadLongWord(callbacks, state->address_registers[7]);
				state->address_registers[7] += 4;				
				break;

			case INSTRUCTION_JSR:
				state->address_registers[7] -= 4;
				WriteLongWord(callbacks, state->address_registers[7], state->program_counter);
				state->program_counter = source_value;
				break;

			case INSTRUCTION_JMP:
				state->program_counter = source_value;
				break;

			case INSTRUCTION_MOVEM:
			{
				/* Hot damn is this a mess */
				unsigned long memory_address = DecodeMemoryAddressMode(state, callbacks, 0, opcode_primary_address_mode, opcode_primary_register);
				size_t i;
				unsigned short bitfield;

				signed char delta;
				unsigned long (*read_function)(const M68k_ReadWriteCallbacks *callbacks, unsigned long address);
				void (*write_function)(const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value);

				if (opcode & 0x0040)
				{
					delta = 4;
					read_function = ReadLongWord;
					write_function = WriteLongWord;
				}
				else
				{
					delta = 2;
					read_function = ReadWord;
					write_function = WriteWord;
				}

				if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
					delta = -delta;

				bitfield = source_value;

				/* First group of registers */
				for (i = 0; i < 8; ++i)
				{
					if (bitfield & 1)
					{
						if (opcode & 0x0400)
						{
							/* Memory to register */
							state->data_registers[i] = read_function(callbacks, memory_address);
						}
						else
						{
							/* Register to memory */
							if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
								write_function(callbacks, memory_address + delta, state->address_registers[7 - i]);
							else
								write_function(callbacks, memory_address, state->data_registers[i]);
						}

						memory_address += delta;
					}

					bitfield >>= 1;
				}

				/* Second group of registers */
				for (i = 0; i < 8; ++i)
				{
					if (bitfield & 1)
					{
						if (opcode & 0x0400)
						{
							/* Memory to register */
							state->address_registers[i] = read_function(callbacks, memory_address);
						}
						else
						{
							/* Register to memory */
							if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
								write_function(callbacks, memory_address + delta, state->data_registers[7 - i]);
							else
								write_function(callbacks, memory_address, state->address_registers[i]);
						}

						memory_address += delta;
					}

					bitfield >>= 1;
				}

				switch (opcode_primary_address_mode)
				{
					case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT:
					case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT:
						bitfield = source_value;

						for (i = 0; i < 16; ++i)
						{
							if (bitfield & 1)
								state->address_registers[opcode_primary_register] += delta;

							bitfield >>= 1;
						}
						
						break;

					default:
						break;
				}

				break;
			}

			case INSTRUCTION_CHK:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("CHK");
				break;

			case INSTRUCTION_ADDQ:
				result_value = destination_value + (((opcode_secondary_register - 1u) & 7u) + 1u); /* A little math trick to turn 0 into 8 */
				break;

			case INSTRUCTION_SUBQ:
				result_value = destination_value - (((opcode_secondary_register - 1u) & 7u) + 1u); /* A little math trick to turn 0 into 8 */
				break;

			case INSTRUCTION_SCC:
				result_value = IsOpcodeConditionTrue(state, opcode) ? 0xFF : 0;
				break;

			case INSTRUCTION_DBCC:
				if (!IsOpcodeConditionTrue(state, opcode))
				{
					unsigned short loop_counter = state->data_registers[opcode_primary_register] & 0xFFFF;

					if (loop_counter-- != 0)
					{
						state->program_counter -= 2;
						state->program_counter += UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(source_value, 0xFFFF);
					}

					state->data_registers[opcode_primary_register] &= ~0xFFFF;
					state->data_registers[opcode_primary_register] |= loop_counter;
				}

				break;

			case INSTRUCTION_BCC:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BRA:
				if (instruction == INSTRUCTION_BCC)
				{
					if (!IsOpcodeConditionTrue(state, opcode))
						break;
				}
				else if (instruction == INSTRUCTION_BSR)
				{
					state->address_registers[7] -= 4;
					WriteLongWord(callbacks, state->address_registers[7], state->program_counter);
				}

				if ((opcode & 0x00FF) != 0)
				{
					state->program_counter += UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(opcode, 0xFF);
				}
				else
				{
					state->program_counter -= 2;
					state->program_counter += UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(source_value, 0xFFFF);
				}

				break;

			case INSTRUCTION_MOVEQ:
				result_value = SIGN_EXTEND(opcode & 0x00FF, 7);
				break;

			case INSTRUCTION_DIVU:
			{
				const unsigned long quotient = destination_value / source_value;

				/* TODO
				if (source_value == 0)
					TRAP();*/

				state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO | CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

				/* Overflow detection */
				if (quotient > 0xFFFF)
				{
					state->status_register |= CONDITION_CODE_OVERFLOW;
				}
				else
				{
					const unsigned short remainder = destination_value % source_value;

					state->data_registers[opcode_secondary_register] = quotient | (remainder << 16);
				}

				state->status_register |= CONDITION_CODE_NEGATIVE * !!(state->data_registers[opcode_secondary_register] & 0x8000);
				state->status_register |= CONDITION_CODE_ZERO * (quotient == 0);

				break;
			}

			case INSTRUCTION_DIVS:
			{
				const short signed_source_value = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(source_value, 0xFFFF);
				const long signed_destination_value = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(destination_value, 0xFFFFFFFF);

				const long quotient = signed_destination_value / signed_source_value;

				state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO | CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

				/* TODO
				if (source_value == 0)
					TRAP();*/

				/* Overflow detection */
				if (quotient < -0x8000 || quotient > 0x7FFF)
				{
					state->status_register |= CONDITION_CODE_OVERFLOW;
				}
				else
				{
					const short remainder = signed_destination_value % signed_source_value;

					state->data_registers[opcode_secondary_register] = (SIGNED_NATIVE_TO_UNSIGNED_TWOS_COMPLEMENT(quotient) & 0xFFFF) | ((SIGNED_NATIVE_TO_UNSIGNED_TWOS_COMPLEMENT(remainder) & 0xFFFF) << 16);
				}

				state->status_register |= CONDITION_CODE_NEGATIVE * !!(state->data_registers[opcode_secondary_register] & 0x8000);
				state->status_register |= CONDITION_CODE_ZERO * (quotient == 0);

				break;
			}

			case INSTRUCTION_SBCD:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("SBCD");
				break;

			case INSTRUCTION_SUBX:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("SUBX");
				break;

			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMPA:
				if (!opcode_bit_8)
					source_value = SIGN_EXTEND(source_value, 15);

				result_value = destination_value - source_value;

				break;

			case INSTRUCTION_MULU:
			{
				const unsigned short multiplier = source_value & 0xFFFF;
				const unsigned short multiplicand = state->data_registers[opcode_secondary_register] & 0xFFFF;
				state->data_registers[opcode_secondary_register] = multiplicand * multiplier;
				break;
			}

			case INSTRUCTION_MULS:
			{
				const short multiplier = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(source_value & 0xFFFF, 0xFFFF);
				const short multiplicand = UNSIGNED_TWOS_COMPLEMENT_TO_SIGNED_NATIVE(state->data_registers[opcode_secondary_register] & 0xFFFF, 0xFFFF);
				state->data_registers[opcode_secondary_register] = SIGNED_NATIVE_TO_UNSIGNED_TWOS_COMPLEMENT(multiplicand * multiplier);
				break;
			}

			case INSTRUCTION_ABCD:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("ABCD");
				break;

			case INSTRUCTION_EXG:
			{
				unsigned long temp;

				switch (opcode & 0x00F8)
				{
					case 0x0040:
						temp = state->data_registers[opcode_secondary_register];
						state->data_registers[opcode_secondary_register] = state->data_registers[opcode_primary_register];
						state->data_registers[opcode_primary_register] = temp;
						break;
						
					case 0x0048:
						temp = state->address_registers[opcode_secondary_register];
						state->address_registers[opcode_secondary_register] = state->address_registers[opcode_primary_register];
						state->address_registers[opcode_primary_register] = temp;
						break;

					case 0x0088:
						temp = state->data_registers[opcode_secondary_register];
						state->data_registers[opcode_secondary_register] = state->address_registers[opcode_primary_register];
						state->address_registers[opcode_primary_register] = temp;
						break;
				}

				break;
			}

			case INSTRUCTION_ADDX:
				/* TODO */
				UNIMPLEMENTED_INSTRUCTION("ADDX");
				break;

			case INSTRUCTION_ADDA:
				if (!opcode_bit_8)
					source_value = SIGN_EXTEND(source_value, 15);

				result_value = destination_value + source_value;

				break;

			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
			{
				const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
				const unsigned long original_sign_bit = destination_value & sign_bit_bitmask;
				unsigned char i;
				unsigned char count;

				result_value = destination_value;

				switch (instruction)
				{
					case INSTRUCTION_ASD_MEMORY:
					case INSTRUCTION_LSD_MEMORY:
					case INSTRUCTION_ROXD_MEMORY:
					case INSTRUCTION_ROD_MEMORY:
						count = 1;
						break;

					case INSTRUCTION_ASD_REGISTER:
					case INSTRUCTION_LSD_REGISTER:
					case INSTRUCTION_ROXD_REGISTER:
					case INSTRUCTION_ROD_REGISTER:
						count = opcode & 0x0020 ? state->data_registers[opcode_secondary_register] % 64 : ((opcode_secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */
						break;

					default:
						/* Shut up dumbass compiler warnings */
						count = 0;
						break;
				}

				state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

				if (opcode_bit_8)
				{
					/* Left */
					for (i = 0; i < count; ++i)
					{
						state->status_register &= ~CONDITION_CODE_CARRY;
						state->status_register |= CONDITION_CODE_CARRY * !!(result_value & sign_bit_bitmask);

						if (instruction != INSTRUCTION_ROD_MEMORY && INSTRUCTION_ROD_REGISTER)
						{
							state->status_register &= ~CONDITION_CODE_EXTEND;
							state->status_register |= CONDITION_CODE_EXTEND * !!(state->status_register & CONDITION_CODE_CARRY);
						}

						switch (instruction)
						{
							case INSTRUCTION_ASD_MEMORY:
							case INSTRUCTION_ASD_REGISTER:
								state->status_register |= CONDITION_CODE_OVERFLOW * ((result_value & sign_bit_bitmask) != original_sign_bit);
								result_value <<= 1;
								break;

							case INSTRUCTION_LSD_MEMORY:
							case INSTRUCTION_LSD_REGISTER:
								result_value <<= 1;
								break;

							case INSTRUCTION_ROXD_MEMORY:
							case INSTRUCTION_ROXD_REGISTER:
								result_value <<= 1;
								result_value |= 1 * !!(state->status_register & CONDITION_CODE_EXTEND);
								break;

							case INSTRUCTION_ROD_MEMORY:
							case INSTRUCTION_ROD_REGISTER:
								result_value = (result_value << 1) | (1 * !!(result_value & sign_bit_bitmask));
								break;

							default:
								/* Shut up dumbass compiler warnings */
								break;
						}
					}
				}
				else
				{
					/* Right */
					for (i = 0; i < count; ++i)
					{
						state->status_register &= ~CONDITION_CODE_CARRY;
						state->status_register |= CONDITION_CODE_CARRY * !!(result_value & 1);

						if (instruction != INSTRUCTION_ROD_MEMORY && INSTRUCTION_ROD_REGISTER)
						{
							state->status_register &= ~CONDITION_CODE_EXTEND;
							state->status_register |= CONDITION_CODE_EXTEND * !!(state->status_register & CONDITION_CODE_CARRY);
						}


						switch (instruction)
						{
							case INSTRUCTION_ASD_MEMORY:
							case INSTRUCTION_ASD_REGISTER:
								result_value >>= 1;
								result_value |= original_sign_bit;
								break;

							case INSTRUCTION_LSD_MEMORY:
							case INSTRUCTION_LSD_REGISTER:
								result_value >>= 1;
								break;

							case INSTRUCTION_ROXD_MEMORY:
							case INSTRUCTION_ROXD_REGISTER:
								result_value >>= 1;
								result_value |= sign_bit_bitmask * !!(state->status_register & CONDITION_CODE_EXTEND);
								break;

							case INSTRUCTION_ROD_MEMORY:
							case INSTRUCTION_ROD_REGISTER:
								result_value = (result_value >> 1) | (sign_bit_bitmask * !!(result_value & sign_bit_bitmask));
								break;

							default:
								/* Shut up dumbass compiler warnings */
								break;
						}
					}
				}

				break;
			}

			case INSTRUCTION_UNKNOWN:
				/* Doesn't do anything */
				break;
		}

		/* Update the condition codes in the following order: */
		/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
		{
			const unsigned long msb_mask = 1ul << (operation_size * 8 - 1);
			const cc_bool sm = !!(source_value & msb_mask);
			const cc_bool dm = !!(destination_value & msb_mask);
			const cc_bool rm = !!(result_value & msb_mask);

			/* Update CARRY condition code */
			switch (instruction)
			{
				case INSTRUCTION_ABCD:
					/* TODO - "Decimal carry" */
					break;

				case INSTRUCTION_NBCD:
				case INSTRUCTION_SBCD:
					/* TODO - "Decimal borrow" */
					break;

				case INSTRUCTION_ADDQ:
					/* Condition codes are not affected when the destination is an address register */
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
						break;
					/* Fallthrough */
				case INSTRUCTION_ADD:
				case INSTRUCTION_ADDI:
				case INSTRUCTION_ADDX:
					state->status_register &= ~CONDITION_CODE_CARRY;
					state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));
					break;

				case INSTRUCTION_SUBQ:
					/* Condition codes are not affected when the destination is an address register */
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
						break;
					/* Fallthrough */
				case INSTRUCTION_CMP:
				case INSTRUCTION_CMPA:
				case INSTRUCTION_CMPI:
				case INSTRUCTION_CMPM:
				case INSTRUCTION_SUB:
				case INSTRUCTION_SUBI:
				case INSTRUCTION_SUBX:
					state->status_register &= ~CONDITION_CODE_CARRY;
					state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
					break;

				case INSTRUCTION_NEG:
				case INSTRUCTION_NEGX:
					state->status_register &= ~CONDITION_CODE_CARRY;
					state->status_register |= CONDITION_CODE_CARRY * (dm || rm);
					break;

				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
				case INSTRUCTION_ROXD_MEMORY:
					/* The condition code is set in the actual instruction code */
					break;

				case INSTRUCTION_AND:
				case INSTRUCTION_ANDI:
				case INSTRUCTION_EOR:
				case INSTRUCTION_EORI:
				case INSTRUCTION_MOVEQ:
				case INSTRUCTION_MOVE:
				case INSTRUCTION_OR:
				case INSTRUCTION_ORI:
				case INSTRUCTION_CLR:
				case INSTRUCTION_EXT:
				case INSTRUCTION_NOT:
				case INSTRUCTION_TAS:
				case INSTRUCTION_TST:
				case INSTRUCTION_DIVS:
				case INSTRUCTION_DIVU:
				case INSTRUCTION_MULS:
				case INSTRUCTION_MULU:
				case INSTRUCTION_SWAP:
					/* Always cleared */
					state->status_register &= ~CONDITION_CODE_CARRY;
					break;

				case INSTRUCTION_CHK:
					/* Undefined */
					break;

				case INSTRUCTION_ADDA:
				case INSTRUCTION_ANDI_TO_CCR:
				case INSTRUCTION_ANDI_TO_SR:
				case INSTRUCTION_BCC:
				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
				case INSTRUCTION_BRA:
				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
				case INSTRUCTION_BSR:
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
				case INSTRUCTION_DBCC:
				case INSTRUCTION_EORI_TO_CCR:
				case INSTRUCTION_EORI_TO_SR:
				case INSTRUCTION_EXG:
				case INSTRUCTION_JMP:
				case INSTRUCTION_JSR:
				case INSTRUCTION_ILLEGAL:
				case INSTRUCTION_LEA:
				case INSTRUCTION_LINK:
				case INSTRUCTION_MOVE_FROM_SR:
				case INSTRUCTION_MOVE_TO_CCR:
				case INSTRUCTION_MOVE_TO_SR:
				case INSTRUCTION_MOVE_USP:
				case INSTRUCTION_MOVEA:
				case INSTRUCTION_MOVEM:
				case INSTRUCTION_MOVEP:
				case INSTRUCTION_NOP:
				case INSTRUCTION_ORI_TO_CCR:
				case INSTRUCTION_ORI_TO_SR:
				case INSTRUCTION_PEA:
				case INSTRUCTION_RESET:
				case INSTRUCTION_RTE:
				case INSTRUCTION_RTR:
				case INSTRUCTION_RTS:
				case INSTRUCTION_SCC:
				case INSTRUCTION_STOP:
				case INSTRUCTION_SUBA:
				case INSTRUCTION_TRAP:
				case INSTRUCTION_TRAPV:
				case INSTRUCTION_UNLK:
				case INSTRUCTION_UNKNOWN:
					/* These instructions don't affect condition codes (unless they write to them directly) */
					break;
			}

			/* Update OVERFLOW condition code */
			switch (instruction)
			{
				case INSTRUCTION_ADDQ:
					/* Condition codes are not affected when the destination is an address register */
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
						break;
					/* Fallthrough */
				case INSTRUCTION_ADD:
				case INSTRUCTION_ADDI:
				case INSTRUCTION_ADDX:
					state->status_register &= ~CONDITION_CODE_OVERFLOW;
					state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));
					break;

				case INSTRUCTION_SUBQ:
					/* Condition codes are not affected when the destination is an address register */
					if (opcode_primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
						break;
					/* Fallthrough */
				case INSTRUCTION_CMP:
				case INSTRUCTION_CMPA:
				case INSTRUCTION_CMPI:
				case INSTRUCTION_CMPM:
				case INSTRUCTION_SUB:
				case INSTRUCTION_SUBI:
				case INSTRUCTION_SUBX:
					state->status_register &= ~CONDITION_CODE_OVERFLOW;
					state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
					break;

				case INSTRUCTION_NEG:
				case INSTRUCTION_NEGX:
					state->status_register &= ~CONDITION_CODE_OVERFLOW;
					state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);
					break;

				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_DIVS:
				case INSTRUCTION_DIVU:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_MULS:
				case INSTRUCTION_MULU:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
				case INSTRUCTION_ROXD_MEMORY:
					/* The condition code is set in the actual instruction code */
					break;

				case INSTRUCTION_AND:
				case INSTRUCTION_ANDI:
				case INSTRUCTION_CLR:
				case INSTRUCTION_EOR:
				case INSTRUCTION_EORI:
				case INSTRUCTION_EXT:
				case INSTRUCTION_MOVE:
				case INSTRUCTION_MOVEQ:
				case INSTRUCTION_NOT:
				case INSTRUCTION_OR:
				case INSTRUCTION_ORI:
				case INSTRUCTION_SWAP:
				case INSTRUCTION_TAS:
				case INSTRUCTION_TST:
					/* Always cleared */
					state->status_register &= ~CONDITION_CODE_OVERFLOW;
					break;

				case INSTRUCTION_ABCD:
				case INSTRUCTION_CHK:
				case INSTRUCTION_NBCD:
				case INSTRUCTION_SBCD:
					/* Undefined */
					break;

				case INSTRUCTION_ADDA:
				case INSTRUCTION_ANDI_TO_CCR:
				case INSTRUCTION_ANDI_TO_SR:
				case INSTRUCTION_BCC:
				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
				case INSTRUCTION_BRA:
				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
				case INSTRUCTION_BSR:
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
				case INSTRUCTION_DBCC:
				case INSTRUCTION_EORI_TO_CCR:
				case INSTRUCTION_EORI_TO_SR:
				case INSTRUCTION_EXG:
				case INSTRUCTION_JMP:
				case INSTRUCTION_JSR:
				case INSTRUCTION_ILLEGAL:
				case INSTRUCTION_LEA:
				case INSTRUCTION_LINK:
				case INSTRUCTION_MOVE_FROM_SR:
				case INSTRUCTION_MOVE_TO_CCR:
				case INSTRUCTION_MOVE_TO_SR:
				case INSTRUCTION_MOVE_USP:
				case INSTRUCTION_MOVEA:
				case INSTRUCTION_MOVEM:
				case INSTRUCTION_MOVEP:
				case INSTRUCTION_NOP:
				case INSTRUCTION_ORI_TO_CCR:
				case INSTRUCTION_ORI_TO_SR:
				case INSTRUCTION_PEA:
				case INSTRUCTION_RESET:
				case INSTRUCTION_RTE:
				case INSTRUCTION_RTR:
				case INSTRUCTION_RTS:
				case INSTRUCTION_SCC:
				case INSTRUCTION_STOP:
				case INSTRUCTION_SUBA:
				case INSTRUCTION_TRAP:
				case INSTRUCTION_TRAPV:
				case INSTRUCTION_UNLK:
				case INSTRUCTION_UNKNOWN:
					/* These instructions don't affect condition codes (unless they write to them directly) */
					break;
			}

			/* Update ZERO condition code */
			switch (instruction)
			{
				case INSTRUCTION_ABCD:
				case INSTRUCTION_ADDX:
				case INSTRUCTION_NBCD:
				case INSTRUCTION_NEGX:
				case INSTRUCTION_SBCD:
				case INSTRUCTION_SUBX:
					/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
					break;

				case INSTRUCTION_ADD:
				case INSTRUCTION_ADDI:
				case INSTRUCTION_ADDQ:
				case INSTRUCTION_AND:
				case INSTRUCTION_ANDI:
				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_CLR:
				case INSTRUCTION_CMP:
				case INSTRUCTION_CMPA:
				case INSTRUCTION_CMPI:
				case INSTRUCTION_CMPM:
				case INSTRUCTION_EOR:
				case INSTRUCTION_EORI:
				case INSTRUCTION_EXT:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_MOVE:
				case INSTRUCTION_MOVEQ:
				case INSTRUCTION_NEG:
				case INSTRUCTION_NOT:
				case INSTRUCTION_OR:
				case INSTRUCTION_ORI:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
				case INSTRUCTION_ROXD_MEMORY:
				case INSTRUCTION_SUB:
				case INSTRUCTION_SUBI:
				case INSTRUCTION_SUBQ:
				case INSTRUCTION_SWAP:
				case INSTRUCTION_TST:
					/* Standard behaviour: set if result is zero; clear otherwise */
					state->status_register &= ~CONDITION_CODE_ZERO;
					state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
					break;

				case INSTRUCTION_DIVS:
				case INSTRUCTION_DIVU:
				case INSTRUCTION_MULS:
				case INSTRUCTION_MULU:
				case INSTRUCTION_TAS:
					/* The condition code is set in the actual instruction code */
					break;

				case INSTRUCTION_CHK:
					/* Undefined */
					break;

				case INSTRUCTION_ADDA:
				case INSTRUCTION_ANDI_TO_CCR:
				case INSTRUCTION_ANDI_TO_SR:
				case INSTRUCTION_BCC:
				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
				case INSTRUCTION_BRA:
				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
				case INSTRUCTION_BSR:
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
				case INSTRUCTION_DBCC:
				case INSTRUCTION_EORI_TO_CCR:
				case INSTRUCTION_EORI_TO_SR:
				case INSTRUCTION_EXG:
				case INSTRUCTION_JMP:
				case INSTRUCTION_JSR:
				case INSTRUCTION_ILLEGAL:
				case INSTRUCTION_LEA:
				case INSTRUCTION_LINK:
				case INSTRUCTION_MOVE_FROM_SR:
				case INSTRUCTION_MOVE_TO_CCR:
				case INSTRUCTION_MOVE_TO_SR:
				case INSTRUCTION_MOVE_USP:
				case INSTRUCTION_MOVEA:
				case INSTRUCTION_MOVEM:
				case INSTRUCTION_MOVEP:
				case INSTRUCTION_NOP:
				case INSTRUCTION_ORI_TO_CCR:
				case INSTRUCTION_ORI_TO_SR:
				case INSTRUCTION_PEA:
				case INSTRUCTION_RESET:
				case INSTRUCTION_RTE:
				case INSTRUCTION_RTR:
				case INSTRUCTION_RTS:
				case INSTRUCTION_SCC:
				case INSTRUCTION_STOP:
				case INSTRUCTION_SUBA:
				case INSTRUCTION_TRAP:
				case INSTRUCTION_TRAPV:
				case INSTRUCTION_UNLK:
				case INSTRUCTION_UNKNOWN:
					/* These instructions don't affect condition codes (unless they write to them directly) */
					break;
			}

			/* Update NEGATIVE condition code */
			switch (instruction)
			{
				case INSTRUCTION_CHK:
				case INSTRUCTION_DIVS:
				case INSTRUCTION_DIVU:
				case INSTRUCTION_MULS:
				case INSTRUCTION_MULU:
				case INSTRUCTION_TAS:
					/* The condition code is set in the actual instruction code */
					break;

				case INSTRUCTION_ADD:
				case INSTRUCTION_ADDI:
				case INSTRUCTION_ADDQ:
				case INSTRUCTION_ADDX:
				case INSTRUCTION_AND:
				case INSTRUCTION_ANDI:
				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_CLR:
				case INSTRUCTION_CMP:
				case INSTRUCTION_CMPA:
				case INSTRUCTION_CMPI:
				case INSTRUCTION_CMPM:
				case INSTRUCTION_EOR:
				case INSTRUCTION_EORI:
				case INSTRUCTION_EXT:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_MOVE:
				case INSTRUCTION_MOVEQ:
				case INSTRUCTION_NEG:
				case INSTRUCTION_NEGX:
				case INSTRUCTION_NOT:
				case INSTRUCTION_OR:
				case INSTRUCTION_ORI:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
				case INSTRUCTION_ROXD_MEMORY:
				case INSTRUCTION_SUB:
				case INSTRUCTION_SUBI:
				case INSTRUCTION_SUBQ:
				case INSTRUCTION_SUBX:
				case INSTRUCTION_SWAP:
				case INSTRUCTION_TST:
					/* Standard behaviour: set if result value is negative; clear otherwise */
					state->status_register &= ~CONDITION_CODE_NEGATIVE;
					state->status_register |= CONDITION_CODE_NEGATIVE * !!(result_value & msb_mask);
					break;

				case INSTRUCTION_ABCD:
				case INSTRUCTION_NBCD:
				case INSTRUCTION_SBCD:
					/* Undefined */
					break;

				case INSTRUCTION_ADDA:
				case INSTRUCTION_ANDI_TO_CCR:
				case INSTRUCTION_ANDI_TO_SR:
				case INSTRUCTION_BCC:
				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
				case INSTRUCTION_BRA:
				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
				case INSTRUCTION_BSR:
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
				case INSTRUCTION_DBCC:
				case INSTRUCTION_EORI_TO_CCR:
				case INSTRUCTION_EORI_TO_SR:
				case INSTRUCTION_EXG:
				case INSTRUCTION_JMP:
				case INSTRUCTION_JSR:
				case INSTRUCTION_ILLEGAL:
				case INSTRUCTION_LEA:
				case INSTRUCTION_LINK:
				case INSTRUCTION_MOVE_FROM_SR:
				case INSTRUCTION_MOVE_TO_CCR:
				case INSTRUCTION_MOVE_TO_SR:
				case INSTRUCTION_MOVE_USP:
				case INSTRUCTION_MOVEA:
				case INSTRUCTION_MOVEM:
				case INSTRUCTION_MOVEP:
				case INSTRUCTION_NOP:
				case INSTRUCTION_ORI_TO_CCR:
				case INSTRUCTION_ORI_TO_SR:
				case INSTRUCTION_PEA:
				case INSTRUCTION_RESET:
				case INSTRUCTION_RTE:
				case INSTRUCTION_RTR:
				case INSTRUCTION_RTS:
				case INSTRUCTION_SCC:
				case INSTRUCTION_STOP:
				case INSTRUCTION_SUBA:
				case INSTRUCTION_TRAP:
				case INSTRUCTION_TRAPV:
				case INSTRUCTION_UNLK:
				case INSTRUCTION_UNKNOWN:
					/* These instructions don't affect condition codes (unless they write to them directly) */
					break;
			}

			/* Update EXTEND condition code */
			switch (instruction)
			{
				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_DIVS:
				case INSTRUCTION_DIVU:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_MULS:
				case INSTRUCTION_MULU:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
				case INSTRUCTION_ROXD_MEMORY:
				case INSTRUCTION_TAS:
					/* The condition code is set in the actual instruction code */
					break;

				case INSTRUCTION_ABCD:
				case INSTRUCTION_ADD:
				case INSTRUCTION_ADDI:
				case INSTRUCTION_ADDQ:
				case INSTRUCTION_ADDX:
				case INSTRUCTION_NBCD:
				case INSTRUCTION_NEG:
				case INSTRUCTION_NEGX:
				case INSTRUCTION_SBCD:
				case INSTRUCTION_SUB:
				case INSTRUCTION_SUBI:
				case INSTRUCTION_SUBQ:
				case INSTRUCTION_SUBX:
					/* Standard behaviour: set to CARRY */
					state->status_register &= ~CONDITION_CODE_EXTEND;
					state->status_register |= CONDITION_CODE_EXTEND * !!(state->status_register & CONDITION_CODE_CARRY);
					break;

				case INSTRUCTION_AND:
				case INSTRUCTION_ANDI:
				case INSTRUCTION_CLR:
				case INSTRUCTION_CHK:
				case INSTRUCTION_CMP:
				case INSTRUCTION_CMPA:
				case INSTRUCTION_CMPI:
				case INSTRUCTION_CMPM:
				case INSTRUCTION_EOR:
				case INSTRUCTION_EORI:
				case INSTRUCTION_EXT:
				case INSTRUCTION_MOVE:
				case INSTRUCTION_MOVEQ:
				case INSTRUCTION_NOT:
				case INSTRUCTION_OR:
				case INSTRUCTION_ORI:
				case INSTRUCTION_SWAP:
				case INSTRUCTION_TST:
					/* Unaffected */
					break;

				case INSTRUCTION_ADDA:
				case INSTRUCTION_ANDI_TO_CCR:
				case INSTRUCTION_ANDI_TO_SR:
				case INSTRUCTION_BCC:
				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
				case INSTRUCTION_BRA:
				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
				case INSTRUCTION_BSR:
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
				case INSTRUCTION_DBCC:
				case INSTRUCTION_EORI_TO_CCR:
				case INSTRUCTION_EORI_TO_SR:
				case INSTRUCTION_EXG:
				case INSTRUCTION_JMP:
				case INSTRUCTION_JSR:
				case INSTRUCTION_ILLEGAL:
				case INSTRUCTION_LEA:
				case INSTRUCTION_LINK:
				case INSTRUCTION_MOVE_FROM_SR:
				case INSTRUCTION_MOVE_TO_CCR:
				case INSTRUCTION_MOVE_TO_SR:
				case INSTRUCTION_MOVE_USP:
				case INSTRUCTION_MOVEA:
				case INSTRUCTION_MOVEM:
				case INSTRUCTION_MOVEP:
				case INSTRUCTION_NOP:
				case INSTRUCTION_ORI_TO_CCR:
				case INSTRUCTION_ORI_TO_SR:
				case INSTRUCTION_PEA:
				case INSTRUCTION_RESET:
				case INSTRUCTION_RTE:
				case INSTRUCTION_RTR:
				case INSTRUCTION_RTS:
				case INSTRUCTION_SCC:
				case INSTRUCTION_STOP:
				case INSTRUCTION_SUBA:
				case INSTRUCTION_TRAP:
				case INSTRUCTION_TRAPV:
				case INSTRUCTION_UNLK:
				case INSTRUCTION_UNKNOWN:
					/* These instructions don't affect condition codes (unless they write to them directly) */
					break;
			}
		}

		/* Write output to destination */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_MOVE_TO_CCR:
				/* Write to condition code register */
				state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);
				break;

			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_MOVE_TO_SR:
				/* Write to status register */
				state->status_register = result_value;
				break;

			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_EORI:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
			case INSTRUCTION_SUBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE:
			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_CLR:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_EXT:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_SWAP:
			case INSTRUCTION_TAS:
			case INSTRUCTION_LEA:
			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
			case INSTRUCTION_SCC:
			case INSTRUCTION_MOVEQ:
			case INSTRUCTION_SBCD:
			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_EOR:
			case INSTRUCTION_ABCD:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
			case INSTRUCTION_ADDX:
			case INSTRUCTION_ASD_MEMORY:
			case INSTRUCTION_LSD_MEMORY:
			case INSTRUCTION_ROXD_MEMORY:
			case INSTRUCTION_ROD_MEMORY:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
				/* Write to destination */
				SetValueUsingDecodedAddressMode(callbacks, &destination_decoded_address_mode, result_value);
				break;

			case INSTRUCTION_ADDA:
			case INSTRUCTION_BCC:
			case INSTRUCTION_BRA:
			case INSTRUCTION_BSR:
			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_CHK:
			case INSTRUCTION_CMP:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_CMPM:
			case INSTRUCTION_DBCC:
			case INSTRUCTION_DIVS:
			case INSTRUCTION_DIVU:
			case INSTRUCTION_EXG:
			case INSTRUCTION_ILLEGAL:
			case INSTRUCTION_JMP:
			case INSTRUCTION_JSR:
			case INSTRUCTION_LINK:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_MOVEM:
			case INSTRUCTION_MOVEP:
			case INSTRUCTION_MULS:
			case INSTRUCTION_MULU:
			case INSTRUCTION_NOP:
			case INSTRUCTION_PEA:
			case INSTRUCTION_RESET:
			case INSTRUCTION_RTE:
			case INSTRUCTION_RTR:
			case INSTRUCTION_RTS:
			case INSTRUCTION_STOP:
			case INSTRUCTION_SUBA:
			case INSTRUCTION_TRAP:
			case INSTRUCTION_TRAPV:
			case INSTRUCTION_TST:
			case INSTRUCTION_UNLK:
			case INSTRUCTION_UNKNOWN:
				/* Doesn't write anything */
				break;
		}

	#ifdef DEBUG_STUFF
		{
			const char* const instruction_strings[] = {
				"INSTRUCTION_ABCD",
				"INSTRUCTION_ADD",
				"INSTRUCTION_ADDA",
				"INSTRUCTION_ADDI",
				"INSTRUCTION_ADDQ",
				"INSTRUCTION_ADDX",
				"INSTRUCTION_AND",
				"INSTRUCTION_ANDI",
				"INSTRUCTION_ANDI_TO_CCR",
				"INSTRUCTION_ANDI_TO_SR",
				"INSTRUCTION_ASD_MEMORY",
				"INSTRUCTION_ASD_REGISTER",
				"INSTRUCTION_BCC",
				"INSTRUCTION_BCHG_DYNAMIC",
				"INSTRUCTION_BCHG_STATIC",
				"INSTRUCTION_BCLR_DYNAMIC",
				"INSTRUCTION_BCLR_STATIC",
				"INSTRUCTION_BRA",
				"INSTRUCTION_BSET_DYNAMIC",
				"INSTRUCTION_BSET_STATIC",
				"INSTRUCTION_BSR",
				"INSTRUCTION_BTST_DYNAMIC",
				"INSTRUCTION_BTST_STATIC",
				"INSTRUCTION_CHK",
				"INSTRUCTION_CLR",
				"INSTRUCTION_CMP",
				"INSTRUCTION_CMPA",
				"INSTRUCTION_CMPI",
				"INSTRUCTION_CMPM",
				"INSTRUCTION_DBCC",
				"INSTRUCTION_DIVS",
				"INSTRUCTION_DIVU",
				"INSTRUCTION_EOR",
				"INSTRUCTION_EORI",
				"INSTRUCTION_EORI_TO_CCR",
				"INSTRUCTION_EORI_TO_SR",
				"INSTRUCTION_EXG",
				"INSTRUCTION_EXT",
				"INSTRUCTION_ILLEGAL",
				"INSTRUCTION_JMP",
				"INSTRUCTION_JSR",
				"INSTRUCTION_LEA",
				"INSTRUCTION_LINK",
				"INSTRUCTION_LSD_MEMORY",
				"INSTRUCTION_LSD_REGISTER",
				"INSTRUCTION_MOVE",
				"INSTRUCTION_MOVE_FROM_SR",
				"INSTRUCTION_MOVE_TO_CCR",
				"INSTRUCTION_MOVE_TO_SR",
				"INSTRUCTION_MOVE_USP",
				"INSTRUCTION_MOVEA",
				"INSTRUCTION_MOVEM",
				"INSTRUCTION_MOVEP",
				"INSTRUCTION_MOVEQ",
				"INSTRUCTION_MULS",
				"INSTRUCTION_MULU",
				"INSTRUCTION_NBCD",
				"INSTRUCTION_NEG",
				"INSTRUCTION_NEGX",
				"INSTRUCTION_NOP",
				"INSTRUCTION_NOT",
				"INSTRUCTION_OR",
				"INSTRUCTION_ORI",
				"INSTRUCTION_ORI_TO_CCR",
				"INSTRUCTION_ORI_TO_SR",
				"INSTRUCTION_PEA",
				"INSTRUCTION_RESET",
				"INSTRUCTION_ROD_MEMORY",
				"INSTRUCTION_ROD_REGISTER",
				"INSTRUCTION_ROXD_MEMORY",
				"INSTRUCTION_ROXD_REGISTER",
				"INSTRUCTION_RTE",
				"INSTRUCTION_RTR",
				"INSTRUCTION_RTS",
				"INSTRUCTION_SBCD",
				"INSTRUCTION_SCC",
				"INSTRUCTION_STOP",
				"INSTRUCTION_SUB",
				"INSTRUCTION_SUBA",
				"INSTRUCTION_SUBI",
				"INSTRUCTION_SUBQ",
				"INSTRUCTION_SUBX",
				"INSTRUCTION_SWAP",
				"INSTRUCTION_TAS",
				"INSTRUCTION_TRAP",
				"INSTRUCTION_TRAPV",
				"INSTRUCTION_TST",
				"INSTRUCTION_UNLK",

				"INSTRUCTION_UNKNOWN"
			};

			fprintf(stderr, "0x%.8lX - %s\n", state->program_counter, instruction_strings[instruction]);
			state->program_counter |= 0; /* Something to latch a breakpoint onto */
		}
	#endif
	}
}
