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
#include <setjmp.h>
#include <stddef.h>

#include "clowncommon.h"

#include "error.h"
#include "m68k/instruction-actions.h"
#include "m68k/instruction.h"
#include "m68k/opcode.h"

/*#define DEBUG_STUFF*/

#ifdef DEBUG_STUFF
#include <stdio.h>
#endif

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
	DECODED_ADDRESS_MODE_TYPE_MEMORY,
	DECODED_ADDRESS_MODE_TYPE_STATUS_REGISTER,
	DECODED_ADDRESS_MODE_TYPE_CONDITION_CODE_REGISTER
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

typedef struct Stuff
{
	M68k_State *state;
	const M68k_ReadWriteCallbacks *callbacks;
	struct
	{
		unsigned long program_counter;
		unsigned short status_register;
		jmp_buf context;
	} exception;
} Stuff;

/* Exception forward-declarations. */

static void Group1Or2Exception(Stuff *stuff, size_t vector_offset);
static void Group0Exception(Stuff *stuff, size_t vector_offset, unsigned long access_address, cc_bool is_a_read);

/* Memory reads */

static unsigned long ReadByte(const Stuff *stuff, unsigned long address)
{
	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;
	const cc_bool odd = (address & 1) != 0;

	return callbacks->read_callback(callbacks->user_data, address & 0xFFFFFE, (cc_bool)!odd, odd) >> (odd ? 0 : 8);
}

static unsigned long ReadWord(Stuff *stuff, unsigned long address)
{
	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;

	if ((address & 1) != 0)
	{
		Group0Exception(stuff, 3, address, cc_true);
		longjmp(stuff->exception.context, 1);
	}

	return callbacks->read_callback(callbacks->user_data, address & 0xFFFFFE, cc_true, cc_true);
}

static unsigned long ReadLongWord(Stuff *stuff, unsigned long address)
{
	unsigned long value;

	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;

	if ((address & 1) != 0)
	{
		Group0Exception(stuff, 3, address, cc_true);
		longjmp(stuff->exception.context, 1);
	}

	value = 0;
	value |= (unsigned long)callbacks->read_callback(callbacks->user_data, (address + 0) & 0xFFFFFE, cc_true, cc_true) << 16;
	value |= (unsigned long)callbacks->read_callback(callbacks->user_data, (address + 2) & 0xFFFFFE, cc_true, cc_true) <<  0;

	return value;
}

/* Memory writes */

static void WriteByte(const Stuff *stuff, unsigned long address, unsigned long value)
{
	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;
	const cc_bool odd = (address & 1) != 0;

	callbacks->write_callback(callbacks->user_data, address & 0xFFFFFE, (cc_bool)!odd, odd, value << (odd ? 0 : 8));
}

static void WriteWord(Stuff *stuff, unsigned long address, unsigned long value)
{
	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;

	if ((address & 1) != 0)
	{
		Group0Exception(stuff, 3, address, cc_false);
		longjmp(stuff->exception.context, 1);
	}

	callbacks->write_callback(callbacks->user_data, address & 0xFFFFFE, cc_true, cc_true, value);
}

static void WriteLongWord(Stuff *stuff, unsigned long address, unsigned long value)
{
	const M68k_ReadWriteCallbacks* const callbacks = stuff->callbacks;

	if ((address & 1) != 0)
	{
		Group0Exception(stuff, 3, address, cc_false);
		longjmp(stuff->exception.context, 1);
	}

	callbacks->write_callback(callbacks->user_data, (address + 0) & 0xFFFFFE, cc_true, cc_true, (value >> 16) & 0xFFFF);
	callbacks->write_callback(callbacks->user_data, (address + 2) & 0xFFFFFE, cc_true, cc_true, (value >>  0) & 0xFFFF);
}

/* Exceptions */

static void Group1Or2Exception(Stuff *stuff, size_t vector_offset)
{
	M68k_State* const state = stuff->state;

	state->address_registers[7] -= 4;
	WriteLongWord(stuff, state->address_registers[7], stuff->exception.program_counter);
	state->address_registers[7] -= 2;
	WriteWord(stuff, state->address_registers[7], stuff->exception.status_register);

	state->status_register &= 0x00FF;
	/* Set supervisor bit */
	state->status_register |= 0x2000;

	state->program_counter = ReadLongWord(stuff, vector_offset * 4);
}

static void Group0Exception(Stuff *stuff, size_t vector_offset, unsigned long access_address, cc_bool is_a_read)
{
	M68k_State* const state = stuff->state;

	/* If a data or address error occurs during group 0 exception processing, then the CPU halts. */
	if ((state->address_registers[7] & 1) != 0)
	{
		state->halted = cc_true;
	}
	else
	{
		Group1Or2Exception(stuff, vector_offset);

		/* Push extra information to the stack. */
		state->address_registers[7] -= 2;
		WriteWord(stuff, state->address_registers[7], state->instruction_register);
		state->address_registers[7] -= 4;
		WriteLongWord(stuff, state->address_registers[7], access_address);
		state->address_registers[7] -= 2;
		/* TODO - Function code and 'Instruction/Not' bit. */
		WriteWord(stuff, state->address_registers[7], is_a_read << 4);
	}
}

/* Misc. utility */

static unsigned long DecodeMemoryAddressMode(Stuff *stuff, const Operand *decoded_operand)
{
	M68k_State* const state = stuff->state;

	unsigned long address;

	if (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_SHORT)
	{
		/* Absolute short */
		const unsigned int short_address = ReadWord(stuff, state->program_counter);

		address = CC_SIGN_EXTEND_ULONG(15, short_address);
		state->program_counter += 2;
	}
	else if (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_ABSOLUTE_LONG)
	{
		/* Absolute long */
		address = ReadLongWord(stuff, state->program_counter);
		state->program_counter += 4;
	}
	else if (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE)
	{
		/* Immediate value */
		if (decoded_operand->operation_size_in_bytes == 1)
		{
			/* A byte-sized immediate value occupies two bytes of space */
			address = state->program_counter + 1;
			state->program_counter += 2;
		}
		else
		{
			address = state->program_counter;
			state->program_counter += decoded_operand->operation_size_in_bytes;
		}
	}
	else if (decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT)
	{
		/* Address register indirect */
		address = state->address_registers[decoded_operand->address_mode_register];
	}
	else if (decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
	{
		/* Address register indirect with predecrement */

		/* The stack pointer moves two bytes instead of one byte, for alignment purposes */
		const unsigned int increment_decrement_size = (decoded_operand->address_mode_register == 7 && decoded_operand->operation_size_in_bytes == 1) ? 2 : decoded_operand->operation_size_in_bytes;

		state->address_registers[decoded_operand->address_mode_register] -= increment_decrement_size;
		address = state->address_registers[decoded_operand->address_mode_register];
	}
	else if (decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT)
	{
		/* Address register indirect with postincrement */

		/* The stack pointer moves two bytes instead of one byte, for alignment purposes */
		const unsigned int increment_decrement_size = (decoded_operand->address_mode_register == 7 && decoded_operand->operation_size_in_bytes == 1) ? 2 : decoded_operand->operation_size_in_bytes;

		address = state->address_registers[decoded_operand->address_mode_register];
		state->address_registers[decoded_operand->address_mode_register] += increment_decrement_size;
	}
	else
	{
		if (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && (decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_DISPLACEMENT || decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX))
		{
			/* Program counter used as base address */
			address = state->program_counter;
		}
		else
		{
			/* Address register used as base address */
			address = state->address_registers[decoded_operand->address_mode_register];
		}

		if (decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT || (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_DISPLACEMENT))
		{
			/* Add displacement */
			const unsigned int displacement = ReadWord(stuff, state->program_counter);

			address += CC_SIGN_EXTEND_ULONG(15, displacement);
			state->program_counter += 2;
		}
		else if (decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX || (decoded_operand->address_mode == ADDRESS_MODE_SPECIAL && decoded_operand->address_mode_register == ADDRESS_MODE_REGISTER_SPECIAL_PROGRAM_COUNTER_WITH_INDEX))
		{
			/* Add index register and index literal */
			const unsigned int extension_word = ReadWord(stuff, state->program_counter);
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

static void DecodeAddressMode(Stuff *stuff, DecodedAddressMode *decoded_address_mode, const Operand *decoded_operand)
{
	M68k_State* const state = stuff->state;

	switch (decoded_operand->address_mode)
	{
		case ADDRESS_MODE_DATA_REGISTER:
		case ADDRESS_MODE_ADDRESS_REGISTER:
			/* Register */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_REGISTER;
			decoded_address_mode->data.reg.address = &(decoded_operand->address_mode == ADDRESS_MODE_ADDRESS_REGISTER ? state->address_registers : state->data_registers)[decoded_operand->address_mode_register];
			decoded_address_mode->data.reg.operation_size_bitmask = (0xFFFFFFFF >> (32 - decoded_operand->operation_size_in_bytes * 8));
			break;

		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT:
		case ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_INDEX:
		case ADDRESS_MODE_SPECIAL:
			/* Memory access */
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_MEMORY;
			decoded_address_mode->data.memory.address = DecodeMemoryAddressMode(stuff, decoded_operand);
			decoded_address_mode->data.memory.operation_size_in_bytes = (unsigned char)decoded_operand->operation_size_in_bytes;
			break;

		case ADDRESS_MODE_STATUS_REGISTER:
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_STATUS_REGISTER;
			break;

		case ADDRESS_MODE_CONDITION_CODE_REGISTER:
			decoded_address_mode->type = DECODED_ADDRESS_MODE_TYPE_CONDITION_CODE_REGISTER;
			break;
	}
}

static unsigned long GetValueUsingDecodedAddressMode(Stuff *stuff, DecodedAddressMode *decoded_address_mode)
{
	unsigned long value = 0;

	M68k_State* const state = stuff->state;

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
				case 0:
					value = address;
					break;

				case 1:
					value = ReadByte(stuff, address);
					break;

				case 2:
					value = ReadWord(stuff, address);
					break;

				case 4:
					value = ReadLongWord(stuff, address);
					break;
			}

			break;
		}

		case DECODED_ADDRESS_MODE_TYPE_STATUS_REGISTER:
			value = state->status_register;
			break;

		case DECODED_ADDRESS_MODE_TYPE_CONDITION_CODE_REGISTER:
			value = state->status_register & 0xFF;
			break;
	}

	return value;
}

static void SetValueUsingDecodedAddressMode(Stuff *stuff, DecodedAddressMode *decoded_address_mode, unsigned long value)
{
	M68k_State* const state = stuff->state;

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
				case 0:
					/* This should never happen. */
					assert(cc_false);
					break;

				case 1:
					WriteByte(stuff, address, value);
					break;

				case 2:
					WriteWord(stuff, address, value);
					break;

				case 4:
					WriteLongWord(stuff, address, value);
					break;
			}

			break;
		}

		case DECODED_ADDRESS_MODE_TYPE_STATUS_REGISTER:
			state->status_register = value;
			break;

		case DECODED_ADDRESS_MODE_TYPE_CONDITION_CODE_REGISTER:
			state->status_register = (value & 0xFF) | (state->status_register & 0xFF00);
			break;
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


/* API */

void M68k_Reset(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks)
{
	Stuff stuff;

	stuff.state = state;
	stuff.callbacks = callbacks;
	stuff.exception.program_counter = state->program_counter;
	stuff.exception.status_register = state->status_register;

	if (!setjmp(stuff.exception.context))
	{
		state->halted = cc_false;

		state->address_registers[7] = ReadLongWord(&stuff, 0);
		state->program_counter = ReadLongWord(&stuff, 4);

		state->status_register &= 0x00FF;
		/* Set supervisor bit */
		state->status_register |= 0x2000;
		/* Set interrupt mask set to 7 */
		state->status_register |= 0x0700;
	}
}

void M68k_Interrupt(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned int level)
{
	Stuff stuff;

	stuff.state = state;
	stuff.callbacks = callbacks;
	stuff.exception.program_counter = state->program_counter;
	stuff.exception.status_register = state->status_register;

	if (!setjmp(stuff.exception.context))
	{
		assert(level >= 1 && level <= 7);

		if (level == 7 || level > (((unsigned int)state->status_register >> 8) & 7))
		{
			Group1Or2Exception(&stuff, 24 + level);

			/* Set interrupt mask set to current level */
			state->status_register |= level << 8;
		}
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
		/* Wait for current instruction to finish. */
		--state->cycles_left_in_instruction;
	}
	else
	{
		/* Initialise closure and exception stuff. */
		Stuff stuff;

		stuff.state = state;
		stuff.callbacks = callbacks;
		stuff.exception.program_counter = state->program_counter;
		stuff.exception.status_register = state->status_register;

		if (!setjmp(stuff.exception.context))
		{
			/* Process new instruction */
			Opcode opcode;
			DecodedAddressMode source_decoded_address_mode, destination_decoded_address_mode;
			unsigned long source_value, destination_value, result_value;
			DecodedOpcode decoded_opcode;
			unsigned long msb_mask;
			unsigned int sm, dm, rm;

			source_value = destination_value = result_value = 0;

			opcode.raw = ReadWord(&stuff, state->program_counter);

			opcode.bits_6_and_7 = (opcode.raw >> 6) & 3;
			opcode.bit_8 = (opcode.raw & 0x100) != 0;

			opcode.primary_register = (opcode.raw >> 0) & 7;
			opcode.primary_address_mode = (AddressMode)((opcode.raw >> 3) & 7);
			opcode.secondary_address_mode = (AddressMode)((opcode.raw >> 6) & 7);
			opcode.secondary_register = (opcode.raw >> 9) & 7;

			state->instruction_register = opcode.raw;
			state->program_counter += 2;

			/* Figure out which instruction this is */
			DecodeOpcode(&decoded_opcode, &opcode);

			#define operation_size decoded_opcode.size /* TODO: Get rid of this ugly thing. */

			switch (decoded_opcode.instruction)
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
					"INSTRUCTION_BCC_SHORT",
					"INSTRUCTION_BCC_WORD",
					"INSTRUCTION_BCHG_DYNAMIC",
					"INSTRUCTION_BCHG_STATIC",
					"INSTRUCTION_BCLR_DYNAMIC",
					"INSTRUCTION_BCLR_STATIC",
					"INSTRUCTION_BRA_SHORT",
					"INSTRUCTION_BRA_WORD",
					"INSTRUCTION_BSET_DYNAMIC",
					"INSTRUCTION_BSET_STATIC",
					"INSTRUCTION_BSR_SHORT",
					"INSTRUCTION_BSR_WORD",
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
}
