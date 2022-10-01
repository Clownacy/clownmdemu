/*
TODO:

68k memory access order:
https://gendev.spritesmind.net/forum/viewtopic.php?p=36896#p36896

DIVU/DIVS timing:
https://gendev.spritesmind.net/forum/viewtopic.php?p=35569#p35569

Z80 reset stuff:
https://gendev.spritesmind.net/forum/viewtopic.php?p=36118#p36118
*/

#include "m68k.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "error.h"
#include "m68k/instructions.h"

/*#define DEBUG_STUFF*/

#ifdef DEBUG_STUFF
#include <stdio.h>
#endif

#define UNIMPLEMENTED_INSTRUCTION(instruction) PrintError("Unimplemented instruction " instruction " used at 0x%lX", state->program_counter)

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

enum
{
	CONDITION_CODE_CARRY    = 1 << 0,
	CONDITION_CODE_OVERFLOW = 1 << 1,
	CONDITION_CODE_ZERO     = 1 << 2,
	CONDITION_CODE_NEGATIVE = 1 << 3,
	CONDITION_CODE_EXTEND   = 1 << 4
};

typedef	enum DecodedAddressModeType
{
	DECODED_ADDRESS_MODE_TYPE_REGISTER,
	DECODED_ADDRESS_MODE_TYPE_MEMORY
} DecodedAddressModeType;

typedef struct DecodedAddressMode
{
	DecodedAddressModeType type;
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

typedef struct Opcode
{
	unsigned int raw;

	unsigned int primary_register;
	unsigned int secondary_register;
	unsigned int bits_6_and_7;

	AddressMode primary_address_mode;
	AddressMode secondary_address_mode;

	cc_bool bit_8;
} Opcode;

/* Exception forward-declarations. */

static void Group1Or2Exception(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, size_t vector_offset);
static void Group0Exception(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, size_t vector_offset, unsigned long access_address, cc_bool is_a_read);

/* Memory reads */

static unsigned long ReadByte(const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	const cc_bool odd = (address & 1) != 0;

	return callbacks->read_callback(callbacks->user_data, address & 0xFFFFFE, (cc_bool)!odd, odd) >> (odd ? 0 : 8);
}

static unsigned long ReadWord(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	if ((address & 1) != 0)
		Group0Exception(state, callbacks, 3, address, cc_true);

	return callbacks->read_callback(callbacks->user_data, address & 0xFFFFFE, cc_true, cc_true);
}

static unsigned long ReadLongWord(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned long address)
{
	unsigned long value;

	if ((address & 1) != 0)
		Group0Exception(state, callbacks, 3, address, cc_true);

	value = 0;
	value |= (unsigned long)callbacks->read_callback(callbacks->user_data, (address + 0) & 0xFFFFFE, cc_true, cc_true) << 16;
	value |= (unsigned long)callbacks->read_callback(callbacks->user_data, (address + 2) & 0xFFFFFE, cc_true, cc_true) <<  0;

	return value;
}

/* Memory writes */

static void WriteByte(const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	const cc_bool odd = (address & 1) != 0;

	callbacks->write_callback(callbacks->user_data, address & 0xFFFFFE, (cc_bool)!odd, odd, value << (odd ? 0 : 8));
}

static void WriteWord(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	if ((address & 1) != 0)
		Group0Exception(state, callbacks, 3, address, cc_false);

	callbacks->write_callback(callbacks->user_data, address & 0xFFFFFE, cc_true, cc_true, value);
}

static void WriteLongWord(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value)
{
	if ((address & 1) != 0)
		Group0Exception(state, callbacks, 3, address, cc_false);

	callbacks->write_callback(callbacks->user_data, (address + 0) & 0xFFFFFE, cc_true, cc_true, (value >> 16) & 0xFFFF);
	callbacks->write_callback(callbacks->user_data, (address + 2) & 0xFFFFFE, cc_true, cc_true, (value >>  0) & 0xFFFF);
}

/* Exceptions */

static void Group1Or2Exception(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, size_t vector_offset)
{
	state->address_registers[7] -= 4;
	WriteLongWord(state, callbacks, state->address_registers[7], state->program_counter);
	state->address_registers[7] -= 2;
	WriteWord(state, callbacks, state->address_registers[7], state->status_register);

	state->status_register &= 0x00FF;
	/* Set supervisor bit */
	state->status_register |= 0x2000;

	state->program_counter = ReadLongWord(state, callbacks, vector_offset * 4);
}

static void Group0Exception(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, size_t vector_offset, unsigned long access_address, cc_bool is_a_read)
{
	/* If a data or address error occurs during group 0 exception processing, then the CPU halts. */
	if ((state->address_registers[7] & 1) != 0)
	{
		state->halted = cc_true;
	}
	else
	{
		Group1Or2Exception(state, callbacks, vector_offset);

		/* Push extra information to the stack. */
		state->address_registers[7] -= 2;
		WriteWord(state, callbacks, state->address_registers[7], state->instruction_register);
		state->address_registers[7] -= 4;
		WriteLongWord(state, callbacks, state->address_registers[7], access_address);
		state->address_registers[7] -= 2;
		/* TODO - Function code and 'Instruction/Not' bit. */
		WriteWord(state, callbacks, state->address_registers[7], is_a_read << 4);
	}
}

/* Misc. utility */

static unsigned long DecodeMemoryAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned int operation_size_in_bytes, AddressMode address_mode, unsigned int reg)
{
	/* The stack pointer moves two bytes instead of one byte, for alignment purposes */
	const unsigned int increment_decrement_size = (reg == 7 && operation_size_in_bytes == 1) ? 2 : operation_size_in_bytes;

	unsigned long address;

	if (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_SHORT)
	{
		/* Absolute short */
		const unsigned int short_address = ReadWord(state, callbacks, state->program_counter);

		address = CC_SIGN_EXTEND_ULONG(15, short_address);
		state->program_counter += 2;
	}
	else if (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_LONG)
	{
		/* Absolute long */
		address = ReadLongWord(state, callbacks, state->program_counter);
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
			const unsigned int displacement = ReadWord(state, callbacks, state->program_counter);

			address += CC_SIGN_EXTEND_ULONG(15, displacement);
			state->program_counter += 2;
		}
		else if (address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX || (address_mode == ADDRESS_MODE_SPECIAL && reg == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX))
		{
			/* Add index register and index literal */
			const unsigned int extension_word = ReadWord(state, callbacks, state->program_counter);
			const cc_bool is_address_register = (extension_word & 0x8000) != 0;
			const unsigned int displacement_reg = (extension_word >> 12) & 7;
			const cc_bool is_longword = (extension_word & 0x0800) != 0;
			const unsigned long displacement_literal_value = CC_SIGN_EXTEND_ULONG(7, extension_word);
			/* TODO - Is an address register ever used here on the 68k? */
			const unsigned long displacement_reg_value = CC_SIGN_EXTEND_ULONG(is_longword ? 31 : 15, (is_address_register ? state->address_registers : state->data_registers)[displacement_reg]);

			address += displacement_reg_value;
			address += displacement_literal_value;
			state->program_counter += 2;
		}
	}

	return address;
}

static void DecodeAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode, unsigned int operation_size_in_bytes, AddressMode address_mode, unsigned int reg)
{
	switch (address_mode)
	{
		case ADDRESS_MODE_DATA_REGISTER:
		case ADDRESS_MODE_ADDRESS_REGISTER:
			/* Register */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_REGISTER;
			decoded_address_mode->data.reg.address = &(address_mode == ADDRESS_MODE_ADDRESS_REGISTER ? state->address_registers : state->data_registers)[reg];
			decoded_address_mode->data.reg.operation_size_bitmask = (0xFFFFFFFF >> (32 - operation_size_in_bytes * 8));
			break;

		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX:
		case ADDRESS_MODE_SPECIAL:
			/* Memory access */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_MEMORY;
			decoded_address_mode->data.memory.address = DecodeMemoryAddressMode(state, callbacks, operation_size_in_bytes, address_mode, reg);
			decoded_address_mode->data.memory.operation_size_in_bytes = (unsigned char)operation_size_in_bytes;
			break;
	}
}

static unsigned long GetValueUsingDecodedAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode)
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
					value = ReadWord(state, callbacks, address);
					break;

				case 4:
					value = ReadLongWord(state, callbacks, address);
					break;
			}

			break;
		}
	}

	return value;
}

static void SetValueUsingDecodedAddressMode(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, DecodedAddressMode *decoded_address_mode, unsigned long value)
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
					WriteWord(state, callbacks, address, value);
					break;

				case 4:
					WriteLongWord(state, callbacks, address, value);
					break;
			}

			break;
		}
	}
}

static cc_bool IsOpcodeConditionTrue(M68k_State *state, unsigned int opcode)
{
	const cc_bool carry = (state->status_register & CONDITION_CODE_CARRY) != 0;
	const cc_bool overflow = (state->status_register & CONDITION_CODE_OVERFLOW) != 0;
	const cc_bool zero = (state->status_register & CONDITION_CODE_ZERO) != 0;
	const cc_bool negative = (state->status_register & CONDITION_CODE_NEGATIVE) != 0;

	switch ((opcode >> 8) & 0xF)
	{
		case 0:
			/* True */
			return cc_true;

		case 1:
			/* False */
			return cc_false;

		case 2:
			/* Higher */
			return !carry && !zero;

		case 3:
			/* Lower or same */
			return carry || zero;

		case 4:
			/* Carry clear */
			return !carry;

		case 5:
			/* Carry set */
			return carry;

		case 6:
			/* Not equal */
			return !zero;

		case 7:
			/* Equal */
			return zero;

		case 8:
			/* Overflow clear */
			return !overflow;

		case 9:
			/* Overflow set */
			return overflow;

		case 10:
			/* Plus */
			return !negative;

		case 11:
			/* Minus */
			return negative;

		case 12:
			/* Greater or equal */
			return (negative && overflow) || (!negative && !overflow);

		case 13:
			/* Less than */
			return (negative && !overflow) || (!negative && overflow);

		case 14:
			/* Greater than */
			return (negative && overflow && !zero) || (!negative && !overflow && !zero);

		case 15:
			/* Less or equal */
			return zero || (negative && !overflow) || (!negative && overflow);

		default:
			return cc_false;
	}
}

static Instruction DecodeOpcode(const Opcode *opcode)
{
	Instruction instruction;

	switch ((opcode->raw >> 12) & 0xF)
	{
		case 0x0:
			if (opcode->bit_8)
			{
				if (opcode->primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
				{
					instruction = INSTRUCTION_MOVEP;
				}
				else
				{
					switch (opcode->bits_6_and_7)
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
				switch (opcode->secondary_register)
				{
					case 0:
						if (opcode->primary_address_mode == ADDRESS_MODE_SPECIAL && opcode->primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
						{
							switch (opcode->bits_6_and_7)
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
						if (opcode->primary_address_mode == ADDRESS_MODE_SPECIAL && opcode->primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
						{
							switch (opcode->bits_6_and_7)
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
						switch (opcode->bits_6_and_7)
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
						if (opcode->primary_address_mode == ADDRESS_MODE_SPECIAL && opcode->primary_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
						{
							switch (opcode->bits_6_and_7)
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

		case 0x1:
		case 0x2:
		case 0x3:
			if ((opcode->raw & 0x01C0) == 0x0040)
				instruction = INSTRUCTION_MOVEA;
			else
				instruction = INSTRUCTION_MOVE;

			break;

		case 0x4:
			if (opcode->bit_8)
			{
				switch (opcode->bits_6_and_7)
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
			else if ((opcode->raw & 0x0800) == 0)
			{
				if (opcode->bits_6_and_7 == 3)
				{
					switch (opcode->secondary_register)
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
					switch (opcode->secondary_register)
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
			else if ((opcode->raw & 0x0200) == 0)
			{
				if ((opcode->raw & 0x01B8) == 0x0080)
					instruction = INSTRUCTION_EXT;
				else if ((opcode->raw & 0x01C0) == 0x0000)
					instruction = INSTRUCTION_NBCD;
				else if ((opcode->raw & 0x01F8) == 0x0040)
					instruction = INSTRUCTION_SWAP;
				else if ((opcode->raw & 0x01C0) == 0x0040)
					instruction = INSTRUCTION_PEA;
				else if ((opcode->raw & 0x0B80) == 0x0880)
					instruction = INSTRUCTION_MOVEM;
			}
			else if (opcode->raw == 0x4AFA || opcode->raw == 0x4AFB || opcode->raw == 0x4AFC)
			{
				instruction = INSTRUCTION_ILLEGAL;
			}
			else if ((opcode->raw & 0x0FC0) == 0x0AC0)
			{
				instruction = INSTRUCTION_TAS;
			}
			else if ((opcode->raw & 0x0F00) == 0x0A00)
			{
				instruction = INSTRUCTION_TST;
			}
			else if ((opcode->raw & 0x0FF0) == 0x0E40)
			{
				instruction = INSTRUCTION_TRAP;
			}
			else if ((opcode->raw & 0x0FF8) == 0x0E50)
			{
				instruction = INSTRUCTION_LINK;
			}
			else if ((opcode->raw & 0x0FF8) == 0x0E58)
			{
				instruction = INSTRUCTION_UNLK;
			}
			else if ((opcode->raw & 0x0FF0) == 0x0E60)
			{
				instruction = INSTRUCTION_MOVE_USP;
			}
			else if ((opcode->raw & 0x0FF8) == 0x0E70)
			{
				switch (opcode->primary_register)
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
			else if ((opcode->raw & 0x0FC0) == 0x0E80)
			{
				instruction = INSTRUCTION_JSR;
			}
			else if ((opcode->raw & 0x0FC0) == 0x0EC0)
			{
				instruction = INSTRUCTION_JMP;
			}

			break;

		case 0x5:
			if (opcode->bits_6_and_7 == 3)
			{
				if (opcode->primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
					instruction = INSTRUCTION_DBCC;
				else
					instruction = INSTRUCTION_SCC;
			}
			else
			{
				if (opcode->primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
					instruction = opcode->bit_8 ? INSTRUCTION_SUBAQ : INSTRUCTION_ADDAQ;
				else
					instruction = opcode->bit_8 ? INSTRUCTION_SUBQ : INSTRUCTION_ADDQ;
			}

			break;

		case 0x6:
			if (opcode->secondary_register == 0)
			{
				if (opcode->bit_8)
					instruction = INSTRUCTION_BSR;
				else
					instruction = INSTRUCTION_BRA;
			}
			else
			{
				instruction = INSTRUCTION_BCC;
			}

			break;

		case 0x7:
			instruction = INSTRUCTION_MOVEQ;
			break;

		case 0x8:
			if (opcode->bits_6_and_7 == 3)
			{
				if (opcode->bit_8)
					instruction = INSTRUCTION_DIVS;
				else
					instruction = INSTRUCTION_DIVU;
			}
			else
			{
				if ((opcode->raw & 0x0170) == 0x0100)
					instruction = INSTRUCTION_SBCD;
				else
					instruction = INSTRUCTION_OR;
			}

			break;

		case 0x9:
			if (opcode->bits_6_and_7 == 3)
				instruction = INSTRUCTION_SUBA;
			else if ((opcode->raw & 0x0170) == 0x0100)
				instruction = INSTRUCTION_SUBX;
			else
				instruction = INSTRUCTION_SUB;

			break;

		case 0xA:
			instruction = INSTRUCTION_UNIMPLEMENTED_1;
			break;

		case 0xB:
			if (opcode->bits_6_and_7 == 3)
				instruction = INSTRUCTION_CMPA;
			else if (!opcode->bit_8)
				instruction = INSTRUCTION_CMP;
			else if (opcode->primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER)
				instruction = INSTRUCTION_CMPM;
			else
				instruction = INSTRUCTION_EOR;

			break;

		case 0xC:
			if (opcode->bits_6_and_7 == 3)
			{
				if (opcode->bit_8)
					instruction = INSTRUCTION_MULS;
				else
					instruction = INSTRUCTION_MULU;
			}
			else if ((opcode->raw & 0x0130) == 0x0100)
			{
				if (opcode->bits_6_and_7 == 0)
					instruction = INSTRUCTION_ABCD;
				else
					instruction = INSTRUCTION_EXG;
			}
			else
			{
				instruction = INSTRUCTION_AND;
			}

			break;

		case 0xD:
			if (opcode->bits_6_and_7 == 3)
				instruction = INSTRUCTION_ADDA;
			else if ((opcode->raw & 0x0170) == 0x0100)
				instruction = INSTRUCTION_ADDX;
			else
				instruction = INSTRUCTION_ADD;

			break;

		case 0xE:
			if (opcode->bits_6_and_7 == 3)
			{
				switch (opcode->secondary_register)
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
				switch (opcode->raw & 0x0018)
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

		case 0xF:
			instruction = INSTRUCTION_UNIMPLEMENTED_2;
			break;
	}

	return instruction;
}

/* API */

void M68k_Reset(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks)
{
	state->halted = cc_false;

	state->address_registers[7] = ReadLongWord(state, callbacks, 0);
	state->program_counter = ReadLongWord(state, callbacks, 4);

	state->status_register &= 0x00FF;
	/* Set supervisor bit */
	state->status_register |= 0x2000;
	/* Set interrupt mask set to 7 */
	state->status_register |= 0x0700;
}

void M68k_Interrupt(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned int level)
{
	assert(level >= 1 && level <= 7);

	if (level == 7 || level > (((unsigned int)state->status_register >> 8) & 7))
	{
		Group1Or2Exception(state, callbacks, 24 + level);

		/* Set interrupt mask set to current level */
		state->status_register |= level << 8;
	}
}

void M68k_DoCycle(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks)
{
	if (state->halted)
	{
		/* Nope, we're not doing anything. */
	}
	else if (state->cycles_left_in_instruction != 0)
	{
		/* Wait for current instruction to finish */
		--state->cycles_left_in_instruction;
	}
	else
	{
		/* Process new instruction */
		Opcode opcode;
		unsigned int operation_size;
		DecodedAddressMode source_decoded_address_mode, destination_decoded_address_mode;
		unsigned long source_value, destination_value, result_value;
		Instruction instruction;
		unsigned long msb_mask;
		cc_bool sm, dm, rm;

		operation_size = 1; /* Set to 1 by default to prevent an invalid shift later on */
		source_value = destination_value = result_value = 0;
		instruction = INSTRUCTION_ILLEGAL;

		opcode.raw = ReadWord(state, callbacks, state->program_counter);

		opcode.bits_6_and_7 = (opcode.raw >> 6) & 3;
		opcode.bit_8 = (opcode.raw & 0x100) != 0;

		opcode.primary_register = (opcode.raw >> 0) & 7;
		opcode.primary_address_mode = (AddressMode)((opcode.raw >> 3) & 7);
		opcode.secondary_address_mode = (AddressMode)((opcode.raw >> 6) & 7);
		opcode.secondary_register = (opcode.raw >> 9) & 7;

		state->instruction_register = opcode.raw;
		state->program_counter += 2;

		/* Figure out which instruction this is */
		instruction = DecodeOpcode(&opcode);

		switch (instruction)
		{
			#include "m68k/gen.c"
		}

	#ifdef DEBUG_STUFF
		{
			static const char* const instruction_strings[] = {
				"INSTRUCTION_ABCD",
				"INSTRUCTION_ADD",
				"INSTRUCTION_ADDA",
				"INSTRUCTION_ADDAQ",
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
				"INSTRUCTION_SUBAQ",
				"INSTRUCTION_SUBI",
				"INSTRUCTION_SUBQ",
				"INSTRUCTION_SUBX",
				"INSTRUCTION_SWAP",
				"INSTRUCTION_TAS",
				"INSTRUCTION_TRAP",
				"INSTRUCTION_TRAPV",
				"INSTRUCTION_TST",
				"INSTRUCTION_UNLK",

				"INSTRUCTION_UNIMPLEMENTED_1",
				"INSTRUCTION_UNIMPLEMENTED_2"
			};

			fprintf(stderr, "0x%.8lX - %s\n", state->program_counter, instruction_strings[instruction]);
			state->program_counter |= 0; /* Something to latch a breakpoint onto */
		}
	#endif
	}
}
