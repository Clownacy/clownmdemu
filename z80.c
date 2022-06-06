#include "z80.h"

#include <assert.h>

#include "clowncommon.h"

#include "error.h"

#define SIGN_EXTEND(value, bitmask) (((value) & ((bitmask) >> 1u)) - ((value) & (((bitmask) >> 1u) + 1u)))

#define UNIMPLEMENTED_INSTRUCTION(instruction) PrintError("Unimplemented instruction " instruction " used at 0x%X", state->program_counter)

typedef struct DecodeInstructionCallbackData
{
	Z80_State *state;
	const Z80_ReadCallback *read_callback;
} DecodeInstructionCallbackData;

static unsigned int DecodeInstructionCallback(void *user_data)
{
	const DecodeInstructionCallbackData* const callback_data = user_data;

	const unsigned int value = callback_data->read_callback->callback(callback_data->read_callback->user_data, callback_data->state->program_counter);

	++callback_data->state->program_counter;
	callback_data->state->program_counter &= 0xFFFF;

	return value;
}

static cc_bool EvaluateCondition(unsigned char flags, Z80_Condition condition)
{
	switch (condition)
	{
		case Z80_CONDITION_NOT_ZERO:
			return (flags & Z80_FLAG_MASK_ZERO) == 0;

		case Z80_CONDITION_ZERO:
			return (flags & Z80_FLAG_MASK_ZERO) != 0;

		case Z80_CONDITION_NOT_CARRY:
			return (flags & Z80_FLAG_MASK_CARRY) == 0;

		case Z80_CONDITION_CARRY:
			return (flags & Z80_FLAG_MASK_CARRY) != 0;

		case Z80_CONDITION_PARITY_OVERFLOW:
			return (flags & Z80_FLAG_MASK_PARITY_OVERFLOW) == 0;

		case Z80_CONDITION_PARITY_EQUALITY:
			return (flags & Z80_FLAG_MASK_PARITY_OVERFLOW) != 0;

		case Z80_CONDITION_PLUS:
			return (flags & Z80_FLAG_MASK_SIGN) == 0;

		case Z80_CONDITION_MINUS:
			return (flags & Z80_FLAG_MASK_SIGN) != 0;

		default:
			/* Should never occur. */
			assert(0);
			return cc_false;
	}
}
#if 0
static unsigned int ALU_Add(Z80_State *state, unsigned int op1, unsigned int op2, unsigned int carry)
{
	unsigned int result;

	result = op1 + op2 + carry;

	/* Clear carry, half-carry, and overflow flags. */
	state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_PARITY_OVERFLOW);

	/* Set carry flag. */
	state->f |= (result & (1 << 8)) >> (8 - Z80_FLAG_BIT_CARRY);

	/* Set half-carry flag. */
	state->f |= ((op1 ^ op2 ^ result) & (1 << 4)) >> (4 - Z80_FLAG_BIT_HALF_CARRY);

	/* Set overflow flag. */
	state->f |= (~(op1 ^ op2) & (op1 ^ result) & (1 << 7)) >> (7 - Z80_FLAG_BIT_PARITY_OVERFLOW);

	return result & 0xFF;
}

static unsigned int ALU_AND(Z80_State *state, unsigned int op1, unsigned int op2)
{
	unsigned int result;

	result = op1 & op2;

	/* Clear carry, half-carry, and overflow flags. */
	state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_PARITY_OVERFLOW);

	/* Set carry flag. */
	state->f |= (result & (1 << 8)) >> (8 - Z80_FLAG_BIT_CARRY);

	/* Set half-carry flag. */
	state->f |= ((op1 ^ op2 ^ result) & (1 << 4)) >> (4 - Z80_FLAG_BIT_HALF_CARRY);

	/* Set overflow flag. */
	state->f |= (~(op1 ^ op2) & (op1 ^ result) & (1 << 7)) >> (7 - Z80_FLAG_BIT_PARITY_OVERFLOW);

	return result;
}
#endif
/* TODO: Eliminate this. */
static unsigned int Read16Bit(const Z80_ReadCallback *read_callback, unsigned int address)
{
	unsigned int value;
	value = read_callback->callback(read_callback->user_data, address + 0);
	value |= read_callback->callback(read_callback->user_data, address + 1) << 8;
	return value;
}

/* TODO: Eliminate this. */
static void Write16Bit(const Z80_WriteCallback *write_callback, unsigned int address, unsigned int value)
{
	write_callback->callback(write_callback->user_data, address + 0, value & 0xFF);
	write_callback->callback(write_callback->user_data, address + 1, value >> 8);
}

static unsigned int ReadOperand(Z80_State *state, const Z80_Instruction *instruction, unsigned int operand, const Z80_ReadCallback *read_callback)
{
	unsigned int value;

	switch (instruction->metadata.operands[operand])
	{
		case Z80_OPERAND_NONE:
			value = 0;
			break;

		case Z80_OPERAND_A:
			value = state->a;
			break;

		case Z80_OPERAND_B:
			value = state->b;
			break;

		case Z80_OPERAND_C:
			value = state->c;
			break;

		case Z80_OPERAND_D:
			value = state->d;
			break;

		case Z80_OPERAND_E:
			value = state->e;
			break;

		case Z80_OPERAND_H:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					value = state->h;
					break;

				case Z80_REGISTER_MODE_IX:
					value = state->ixh;
					break;

				case Z80_REGISTER_MODE_IY:
					value = state->iyh;
					break;
			}

			break;

		case Z80_OPERAND_L:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					value = state->l;
					break;

				case Z80_REGISTER_MODE_IX:
					value = state->ixl;
					break;

				case Z80_REGISTER_MODE_IY:
					value = state->iyl;
					break;
			}

			break;

		case Z80_OPERAND_AF:
			value = (state->a << 8) | state->f;
			break;

		case Z80_OPERAND_BC:
			value = (state->b << 8) | state->c;
			break;

		case Z80_OPERAND_DE:
			value = (state->d << 8) | state->e;
			break;

		case Z80_OPERAND_HL:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					value = (state->h << 8) | state->l;
					break;

				case Z80_REGISTER_MODE_IX:
					value = (state->ixh << 8) | state->iyl;
					break;

				case Z80_REGISTER_MODE_IY:
					value = (state->iyh << 8) | state->iyl;
					break;
			}

			break;

		case Z80_OPERAND_PC:
			value = state->program_counter;
			break;

		case Z80_OPERAND_SP:
			value = state->stack_pointer;
			break;

		case Z80_OPERAND_LITERAL_8BIT:
		case Z80_OPERAND_LITERAL_16BIT:
			value = instruction->literal;
			break;

		case Z80_OPERAND_BC_INDIRECT:
		case Z80_OPERAND_DE_INDIRECT:
		case Z80_OPERAND_HL_INDIRECT:
		case Z80_OPERAND_ADDRESS:
		{
			unsigned int address;

			switch (instruction->metadata.operands[operand])
			{
				default:
					/* Should never occur. */
					assert(0);
					address = 0;
					break;

				case Z80_OPERAND_BC_INDIRECT:
					address = (state->b << 8) | state->c;
					break;

				case Z80_OPERAND_DE_INDIRECT:
					address = (state->d << 8) | state->e;
					break;

				case Z80_OPERAND_HL_INDIRECT:
					switch (state->register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							address = (state->h << 8) | state->l;
							break;

						case Z80_REGISTER_MODE_IX:
							address = ((state->ixh << 8) | state->ixl) + instruction->displacement;
							break;

						case Z80_REGISTER_MODE_IY:
							address = ((state->iyh << 8) | state->iyl) + instruction->displacement;
							break;
					}

					break;

				case Z80_OPERAND_ADDRESS:
					address = instruction->literal;
					break;
			}

			if (instruction->metadata.indirect_16bit)
				value = Read16Bit(read_callback, address);
			else
				value = read_callback->callback(read_callback->user_data, address);

			break;
		}
	}

	return value;
}

static void WriteOperand(Z80_State *state, const Z80_Instruction *instruction, unsigned int operand, unsigned int value, const Z80_WriteCallback *write_callback)
{
	switch (instruction->metadata.operands[operand])
	{
		case Z80_OPERAND_NONE:
			break;

		case Z80_OPERAND_A:
			state->a = value;
			break;

		case Z80_OPERAND_B:
			state->b = value;
			break;

		case Z80_OPERAND_C:
			state->c = value;
			break;

		case Z80_OPERAND_D:
			state->d = value;
			break;

		case Z80_OPERAND_E:
			state->e = value;
			break;

		case Z80_OPERAND_H:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					state->h = value;
					break;

				case Z80_REGISTER_MODE_IX:
					state->ixh = value;
					break;

				case Z80_REGISTER_MODE_IY:
					state->iyh = value;
					break;
			}

			break;

		case Z80_OPERAND_L:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					state->l = value;
					break;

				case Z80_REGISTER_MODE_IX:
					state->ixl = value;
					break;

				case Z80_REGISTER_MODE_IY:
					state->iyl = value;
					break;
			}

			break;

		case Z80_OPERAND_AF:
			state->a = value >> 8;
			state->f = value & 0xFF;
			break;

		case Z80_OPERAND_BC:
			state->b = value >> 8;
			state->c = value & 0xFF;
			break;

		case Z80_OPERAND_DE:
			state->d = value >> 8;
			state->e = value & 0xFF;
			break;

		case Z80_OPERAND_HL:
			switch (state->register_mode)
			{
				case Z80_REGISTER_MODE_HL:
					state->h = value >> 8;
					state->l = value & 0xFF;
					break;

				case Z80_REGISTER_MODE_IX:
					state->ixh = value >> 8;
					state->ixl = value & 0xFF;
					break;

				case Z80_REGISTER_MODE_IY:
					state->iyh = value >> 8;
					state->iyl = value & 0xFF;
					break;
			}

			break;

		case Z80_OPERAND_PC:
			state->program_counter = value;
			break;

		case Z80_OPERAND_SP:
			state->stack_pointer = value;
			break;

		case Z80_OPERAND_BC_INDIRECT:
		case Z80_OPERAND_DE_INDIRECT:
		case Z80_OPERAND_HL_INDIRECT:
		case Z80_OPERAND_ADDRESS:
		{
			unsigned int address;

			switch (instruction->metadata.operands[operand])
			{
				default:
					/* Should never occur. */
					assert(0);
					address = 0;
					break;

				case Z80_OPERAND_BC_INDIRECT:
					address = (state->b << 8) | state->c;
					break;

				case Z80_OPERAND_DE_INDIRECT:
					address = (state->d << 8) | state->e;
					break;

				case Z80_OPERAND_HL_INDIRECT:
					switch (state->register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							address = (state->h << 8) | state->l;
							break;

						case Z80_REGISTER_MODE_IX:
							address = ((state->ixh << 8) | state->ixl) + instruction->displacement;
							break;

						case Z80_REGISTER_MODE_IY:
							address = ((state->iyh << 8) | state->iyl) + instruction->displacement;
							break;
					}

					break;

				case Z80_OPERAND_ADDRESS:
					address = instruction->literal;
					break;
			}

			if (instruction->metadata.indirect_16bit)
				Write16Bit(write_callback, address, value);
			else
				write_callback->callback(write_callback->user_data, address, value);

			break;
		}
	}
}

void Z80_State_Initialise(Z80_State *state)
{
	Z80_Reset(state);
}

void Z80_Reset(Z80_State *state)
{
	/* Revert back to a prefix-free mode. */
	state->register_mode = Z80_REGISTER_MODE_HL;
	state->instruction_mode = Z80_INSTRUCTION_MODE_NORMAL;

	state->program_counter = 0;

	/* The official Z80 manual claims that this happens ('z80cpu_um.pdf' page 18). */
	state->interrupts_enabled = cc_false;
}

void Z80_Interrupt(Z80_State *state, const Z80_ReadAndWriteCallbacks *callbacks)
{
	/* TODO: The other interrupt modes. */
	if (state->interrupts_enabled)
	{
		state->interrupts_enabled = cc_false;

		--state->stack_pointer;
		state->stack_pointer &= 0xFFFF;
		callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter >> 8);

		--state->stack_pointer;
		state->stack_pointer &= 0xFFFF;
		callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter & 0xFF);

		state->program_counter = 0x38;
	}
}

void Z80_DecodeInstructionMetadata(Z80_InstructionMetadata *metadata, Z80_InstructionMode instruction_mode, unsigned char opcode)
{
	static const Z80_Operand registers[8] = {Z80_OPERAND_B, Z80_OPERAND_C, Z80_OPERAND_D, Z80_OPERAND_E, Z80_OPERAND_H, Z80_OPERAND_L, Z80_OPERAND_HL_INDIRECT, Z80_OPERAND_A};
	static const Z80_Operand register_pairs_1[4] = {Z80_OPERAND_BC, Z80_OPERAND_DE, Z80_OPERAND_HL, Z80_OPERAND_SP};
	static const Z80_Operand register_pairs_2[4] = {Z80_OPERAND_BC, Z80_OPERAND_DE, Z80_OPERAND_HL, Z80_OPERAND_AF};
	static const Z80_Opcode arithmetic_logic_opcodes[8] = {Z80_OPCODE_ADD_A, Z80_OPCODE_ADC_A, Z80_OPCODE_SUB, Z80_OPCODE_SBC_A, Z80_OPCODE_AND, Z80_OPCODE_XOR, Z80_OPCODE_OR, Z80_OPCODE_CP};
	static const Z80_Opcode rotate_shift_opcodes[8] = {Z80_OPCODE_RLC, Z80_OPCODE_RRC, Z80_OPCODE_RL, Z80_OPCODE_RR, Z80_OPCODE_SLA, Z80_OPCODE_SRA, Z80_OPCODE_SLL, Z80_OPCODE_SRL};
	static const Z80_Opcode block_opcodes[4][4] = {
		{Z80_OPCODE_LDI,  Z80_OPCODE_LDD,  Z80_OPCODE_LDIR, Z80_OPCODE_LDDR},
		{Z80_OPCODE_CPI,  Z80_OPCODE_CPD,  Z80_OPCODE_CPIR, Z80_OPCODE_CPDR},
		{Z80_OPCODE_INI,  Z80_OPCODE_IND,  Z80_OPCODE_INIR, Z80_OPCODE_INDR},
		{Z80_OPCODE_OUTI, Z80_OPCODE_OUTD, Z80_OPCODE_OTIR, Z80_OPCODE_OTDR}
	};

	const unsigned int x = (opcode >> 6) & 3;
	const unsigned int y = (opcode >> 3) & 7;
	const unsigned int z = (opcode >> 0) & 7;
	const unsigned int p = (y >> 1) & 3;
	const cc_bool q = (y & 1) != 0;

	metadata->has_displacement = cc_false;

	metadata->operands[0] = Z80_OPERAND_NONE;
	metadata->operands[1] = Z80_OPERAND_NONE;
	metadata->read_destination = cc_false;
	metadata->write_destination = cc_false;
	metadata->indirect_16bit = cc_false;

	switch (instruction_mode)
	{
		case Z80_INSTRUCTION_MODE_NORMAL:
			switch (x)
			{
				case 0:
					switch (z)
					{
						case 0:
							switch (y)
							{
								case 0:
									metadata->opcode = Z80_OPCODE_NOP;
									break;

								case 1:
									metadata->opcode = Z80_OPCODE_EX_AF_AF;
									break;

								case 2:
									metadata->opcode = Z80_OPCODE_DJNZ;
									metadata->has_displacement = cc_true;
									break;

								case 3:
									metadata->opcode = Z80_OPCODE_JR_UNCONDITIONAL;
									metadata->has_displacement = cc_true;
									break;

								case 4:
								case 5:
								case 6:
								case 7:
									metadata->opcode = Z80_OPCODE_JR_CONDITIONAL;
									metadata->has_displacement = cc_true;
									metadata->condition = y - 4;
									break;
							}

							break;

						case 1:
							if (!q)
							{
								metadata->opcode = Z80_OPCODE_LD_16BIT;
								metadata->operands[0] = Z80_OPERAND_LITERAL_16BIT;
								metadata->operands[1] = register_pairs_1[p];
								metadata->write_destination = cc_true;
								metadata->indirect_16bit = cc_true;
							}
							else
							{
								metadata->opcode = Z80_OPCODE_ADD_HL;
								metadata->operands[0] = register_pairs_1[p];
								metadata->operands[1] = Z80_OPERAND_HL;
								metadata->read_destination = cc_true;
								metadata->write_destination = cc_true;
							}

							break;

						case 2:
						{
							static const Z80_Operand operands[4] = {Z80_OPERAND_BC_INDIRECT, Z80_OPERAND_DE_INDIRECT, Z80_OPERAND_ADDRESS, Z80_OPERAND_ADDRESS};

							const Z80_Operand operand_a = p == 2 ? Z80_OPERAND_HL : Z80_OPERAND_A;
							const Z80_Operand operand_b = operands[p];

							metadata->opcode = p == 2 ? Z80_OPCODE_LD_16BIT : Z80_OPCODE_LD_8BIT;
							metadata->write_destination = cc_true;
							metadata->indirect_16bit = p == 2;

							if (!q)
							{
								metadata->operands[0] = operand_a;
								metadata->operands[1] = operand_b;
							}
							else
							{
								metadata->operands[0] = operand_b;
								metadata->operands[1] = operand_a;
							}

							break;
						}

						case 3:
							if (!q)
								metadata->opcode = Z80_OPCODE_INC_16BIT;
							else
								metadata->opcode = Z80_OPCODE_DEC_16BIT;

							metadata->operands[1] = register_pairs_1[p];
							metadata->read_destination = cc_true;
							metadata->write_destination = cc_true;
							break;

						case 4:
							metadata->opcode = Z80_OPCODE_INC_8BIT;
							metadata->operands[1] = registers[y];
							metadata->read_destination = cc_true;
							metadata->write_destination = cc_true;
							break;

						case 5:
							metadata->opcode = Z80_OPCODE_DEC_8BIT;
							metadata->operands[1] = registers[y];
							metadata->read_destination = cc_true;
							metadata->write_destination = cc_true;
							break;

						case 6:
							metadata->opcode = Z80_OPCODE_LD_8BIT;
							metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
							metadata->operands[1] = registers[y];
							metadata->write_destination = cc_true;
							break;

						case 7:
						{
							static const opcodes[8] = {Z80_OPCODE_RLCA, Z80_OPCODE_RRCA, Z80_OPCODE_RLA, Z80_OPCODE_RRA, Z80_OPCODE_DAA, Z80_OPCODE_CPL, Z80_OPCODE_SCF, Z80_OPCODE_CCF};
							metadata->opcode = opcodes[y];
							break;
						}
					}

					break;

				case 1:
					if (z == 6 && y == 6)
					{
						metadata->opcode = Z80_OPCODE_HALT;
					}
					else
					{
						metadata->opcode = Z80_OPCODE_LD_8BIT;
						metadata->operands[0] = registers[z];
						metadata->operands[1] = registers[y];
						metadata->write_destination = cc_true;
					}

					break;

				case 2:
					metadata->opcode = arithmetic_logic_opcodes[y];
					metadata->operands[0] = registers[z];
					break;

				case 3:
					switch (z)
					{
						case 0:
							metadata->opcode = Z80_OPCODE_RET_CONDITIONAL;
							metadata->condition = y;
							break;

						case 1:
							if (!q)
							{
								metadata->opcode = Z80_OPCODE_POP;
								metadata->operands[1] = register_pairs_2[p];
								metadata->write_destination = cc_true;
							}
							else
							{
								switch (p)
								{
									case 0:
										metadata->opcode = Z80_OPCODE_RET_UNCONDITIONAL;
										break;

									case 1:
										metadata->opcode = Z80_OPCODE_EXX;
										break;

									case 2:
										metadata->opcode = Z80_OPCODE_JP_HL;
										metadata->operands[0] = Z80_OPERAND_HL;
										break;

									case 3:
										metadata->opcode = Z80_OPCODE_LD_SP_HL;
										metadata->operands[0] = Z80_OPERAND_HL;
										break;
								}
							}

							break;

						case 2:
							metadata->opcode = Z80_OPCODE_JP_CONDITIONAL;
							metadata->condition = y;
							metadata->operands[0] = Z80_OPERAND_LITERAL_16BIT;
							break;

						case 3:
							switch (y)
							{
								case 0:
									metadata->opcode = Z80_OPCODE_JP_UNCONDITIONAL;
									metadata->operands[0] = Z80_OPERAND_LITERAL_16BIT;
									break;

								case 1:
									metadata->opcode = Z80_OPCODE_CB_PREFIX;
									break;

								case 2:
									metadata->opcode = Z80_OPCODE_OUT;
									metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
									break;

								case 3:
									metadata->opcode = Z80_OPCODE_IN;
									metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
									break;

								case 4:
									metadata->opcode = Z80_OPCODE_EX_SP_HL;
									break;

								case 5:
									metadata->opcode = Z80_OPCODE_EX_DE_HL;
									break;

								case 6:
									metadata->opcode = Z80_OPCODE_DI;
									break;

								case 7:
									metadata->opcode = Z80_OPCODE_EI;
									break;
							}
					
							break;

						case 4:
							metadata->opcode = Z80_OPCODE_CALL_CONDITIONAL;
							metadata->condition = y;
							metadata->operands[0] = Z80_OPERAND_LITERAL_16BIT;
							break;

						case 5:
							if (!q)
							{
								metadata->opcode = Z80_OPCODE_PUSH;
								metadata->operands[0] = register_pairs_2[p];
							}
							else
							{
								switch (p)
								{
									case 0:
										metadata->opcode = Z80_OPCODE_CALL_UNCONDITIONAL;
										metadata->operands[0] = Z80_OPERAND_LITERAL_16BIT;
										break;

									case 1:
										metadata->opcode = Z80_OPCODE_DD_PREFIX;
										break;

									case 2:
										metadata->opcode = Z80_OPCODE_ED_PREFIX;
										break;

									case 3:
										metadata->opcode = Z80_OPCODE_FD_PREFIX;
										break;
								}
							}

							break;

						case 6:
							metadata->opcode = arithmetic_logic_opcodes[y];
							metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
							break;

						case 7:
							metadata->opcode = Z80_OPCODE_RST;
							metadata->embedded_literal = y * 8;
							break;
					}

					break;
			}

			break;

		case Z80_INSTRUCTION_MODE_BITS:
			switch (x)
			{
				case 0:
					metadata->opcode = rotate_shift_opcodes[y];
					metadata->operands[1] = registers[z];
					metadata->read_destination = cc_true;
					metadata->write_destination = cc_true;
					break;

				case 1:
					metadata->opcode = Z80_OPCODE_BIT;
					metadata->operands[1] = registers[z];
					metadata->embedded_literal = y;
					metadata->read_destination = cc_true;
					break;

				case 2:
					metadata->opcode = Z80_OPCODE_RES;
					metadata->operands[1] = registers[z];
					metadata->embedded_literal = y;
					metadata->read_destination = cc_true;
					metadata->write_destination = cc_true;
					break;

				case 3:
					metadata->opcode = Z80_OPCODE_SET;
					metadata->operands[1] = registers[z];
					metadata->embedded_literal = y;
					metadata->read_destination = cc_true;
					metadata->write_destination = cc_true;
					break;
			}

			break;

		case Z80_INSTRUCTION_MODE_MISC:
			switch (x)
			{
				case 0:
				case 3:
					/* Invalid instruction. */
					metadata->opcode = Z80_OPCODE_NOP;
					break;

				case 1:
					switch (z)
					{
						case 0:
							if (y != 6)
								metadata->opcode = Z80_OPCODE_IN_REGISTER;
							else
								metadata->opcode = Z80_OPCODE_IN_NO_REGISTER;

							break;

						case 1:
							if (y != 6)
								metadata->opcode = Z80_OPCODE_OUT_REGISTER;
							else
								metadata->opcode = Z80_OPCODE_OUT_NO_REGISTER;

							break;

						case 2:
							if (!q)
								metadata->opcode = Z80_OPCODE_SBC_HL;
							else
								metadata->opcode = Z80_OPCODE_ADC_HL;

							metadata->operands[0] = register_pairs_1[p];

							break;

						case 3:
							metadata->opcode = Z80_OPCODE_LD_16BIT;
							metadata->write_destination = cc_true;

							if (!q)
							{
								metadata->operands[0] = register_pairs_1[p];
								metadata->operands[1] = Z80_OPERAND_ADDRESS;
							}
							else
							{
								metadata->operands[0] = Z80_OPERAND_ADDRESS;
								metadata->operands[1] = register_pairs_1[p];
							}

							break;

						case 4:
							metadata->opcode = Z80_OPCODE_NEG;
							break;

						case 5:
							if (y != 1)
								metadata->opcode = Z80_OPCODE_RETN;
							else
								metadata->opcode = Z80_OPCODE_RETI;

							break;

						case 6:
						{
							static const interrupt_modes[4] = {0, 0, 1, 2};

							metadata->opcode = Z80_OPCODE_IM;
							metadata->embedded_literal = interrupt_modes[y & 3];

							break;
						}

						case 7:
						{
							static const assorted_opcodes[8] = {Z80_OPCODE_LD_I_A, Z80_OPCODE_LD_R_A, Z80_OPCODE_LD_A_I, Z80_OPCODE_LD_A_R, Z80_OPCODE_RRD, Z80_OPCODE_RLD, Z80_OPCODE_NOP, Z80_OPCODE_NOP};
							metadata->opcode = assorted_opcodes[y];
							break;
						}
					}

					break;

				case 2:
					if (z <= 3 && y >= 4)
						metadata->opcode = block_opcodes[z][y - 4];
					else
						/* Invalid instruction. */
						metadata->opcode = Z80_OPCODE_NOP;

					break;
			}
			break;
	}
}

void Z80_DecodeInstruction(Z80_Instruction *instruction, Z80_InstructionMode instruction_mode, Z80_RegisterMode register_mode, const Z80_ReadInstructionCallback *read_callback)
{
	Z80_DecodeInstructionMetadata(&instruction->metadata, instruction_mode, read_callback->callback(read_callback->user_data));

	if (instruction->metadata.has_displacement || (register_mode != Z80_REGISTER_MODE_HL && (instruction->metadata.operands[0] == Z80_OPERAND_HL_INDIRECT || instruction->metadata.operands[1] == Z80_OPERAND_HL_INDIRECT)))
	{
		const unsigned int displacement = read_callback->callback(read_callback->user_data);
		instruction->displacement = SIGN_EXTEND(displacement, 0xFF);
	}

	if (instruction->metadata.operands[0] == Z80_OPERAND_LITERAL_8BIT)
	{
		instruction->literal = read_callback->callback(read_callback->user_data);
	}
	else if (instruction->metadata.operands[0] == Z80_OPERAND_LITERAL_16BIT || instruction->metadata.operands[0] == Z80_OPERAND_ADDRESS || instruction->metadata.operands[1] == Z80_OPERAND_ADDRESS)
	{
		instruction->literal = read_callback->callback(read_callback->user_data);
		instruction->literal |= read_callback->callback(read_callback->user_data) << 8;
	}
}

#define SWAP(a, b) \
	swap_holder = a; \
	a = b; \
	b = swap_holder

void Z80_ExecuteInstruction(Z80_State *state, const Z80_Instruction *instruction, const Z80_ReadAndWriteCallbacks *callbacks)
{
	const unsigned int source_value = ReadOperand(state, instruction, 0, &callbacks->read);

	unsigned int destination_value;
	unsigned char swap_holder;

	if (instruction->metadata.read_destination)
		destination_value = ReadOperand(state, instruction, 1, &callbacks->read);

	switch (instruction->metadata.opcode)
	{
		case Z80_OPCODE_NOP:
			/* Does nothing, naturally. */
			state->cycles = 4;
			break;

		case Z80_OPCODE_EX_AF_AF:
			SWAP(state->a, state->a_);
			SWAP(state->f, state->f_);

			state->cycles = 4;
			break;

		case Z80_OPCODE_DJNZ:
			if (--state->b != 0)
			{
				state->cycles = 13;
				state->program_counter += instruction->displacement;
			}
			else
			{
				state->cycles = 8;
			}

			break;

		case Z80_OPCODE_JR_CONDITIONAL:
			state->cycles = 7;
			if (!EvaluateCondition(state->f, instruction->metadata.condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_JR_UNCONDITIONAL:
			state->cycles = 12;
			state->program_counter += instruction->displacement;
			break;

		case Z80_OPCODE_LD_16BIT:
			destination_value = source_value;
			break;

		case Z80_OPCODE_ADD_HL:
			destination_value += source_value;
			destination_value &= 0xFFFF;
			break;

		case Z80_OPCODE_LD_8BIT:
			destination_value = source_value;
			break;

		case Z80_OPCODE_INC_16BIT:
			++destination_value;
			destination_value &= 0xFFFF;
			break;

		case Z80_OPCODE_DEC_16BIT:
			--destination_value;
			destination_value &= 0xFFFF;
			break;

		case Z80_OPCODE_INC_8BIT:
			++destination_value;
			destination_value &= 0xFF;
			break;

		case Z80_OPCODE_DEC_8BIT:
			--destination_value;
			destination_value &= 0xFF;
			break;

		case Z80_OPCODE_RLCA:
		{
			const cc_bool carry = (state->a & 0x80) != 0;

			state->a <<= 1;
			state->a &= 0xFF;
			state->a |= carry ? 0x01 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RRCA:
		{
			const cc_bool carry = (state->a & 0x01) != 0;

			state->a >>= 1;
			state->a |= carry ? 0x80 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RLA:
		{
			const cc_bool carry = (state->a & 0x80) != 0;

			state->a <<= 1;
			state->a &= 0xFF;
			state->a |= (state->f &= Z80_FLAG_MASK_CARRY) != 0 ? 1 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RRA:
		{
			const cc_bool carry = (state->a & 0x01) != 0;

			state->a >>= 1;
			state->a |= (state->f &= Z80_FLAG_MASK_CARRY) != 0 ? 0x80 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_DAA:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("DAA");
			break;

		case Z80_OPCODE_CPL:
			state->a = ~state->a;
			state->a &= 0xFF;

			state->f |= Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT;

			break;

		case Z80_OPCODE_SCF:
			state->f &= ~(Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= Z80_FLAG_MASK_CARRY;
			break;

		case Z80_OPCODE_CCF:
			state->f &= ~(Z80_FLAG_MASK_ADD_SUBTRACT | Z80_FLAG_MASK_HALF_CARRY);

			state->f |= (state->f & Z80_FLAG_MASK_CARRY) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;
			state->f ^= Z80_FLAG_MASK_CARRY;

			break;

		case Z80_OPCODE_HALT:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("HALT");
			break;

		case Z80_OPCODE_ADD_A:
			state->f = 0;

			/* Set half-carry flag. */
			state->f |= ((state->a & 0xF) + (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;

			/* Perform addition. */
			state->a += source_value;

			/* Set remaining flags. */
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (state->a & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((state->a & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;

			state->a &= 0xFF;

			break;

		case Z80_OPCODE_ADC_A:
			state->f = 0;

			/* Set half-carry flag. */
			/* TODO: Pretty sure this is incorrect - shouldn't it add the carry as well? */
			state->f |= ((state->a & 0xF) + (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;

			/* Perform addition. */
			state->a += source_value;
			state->a += (state->f & Z80_FLAG_MASK_CARRY) != 0;

			/* Set remaining flags. */
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (state->a & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((state->a & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;

			state->a &= 0xFF;

			break;

		case Z80_OPCODE_SUB:
			state->f = 0;

			/* Set half-carry flag. */
			state->f |= ((state->a & 0xF) - (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;

			/* Perform subtraction. */
			state->a -= source_value;

			/* Set remaining flags. */
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (state->a & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((state->a & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;
			state->f |= Z80_FLAG_MASK_ADD_SUBTRACT;

			state->a &= 0xFF;

			break;

		case Z80_OPCODE_SBC_A:
			state->f = 0;

			/* Set half-carry flag. */
			state->f |= ((state->a & 0xF) - (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;

			/* Perform subtraction. */
			state->a -= source_value;
			state->a -= (state->f & Z80_FLAG_MASK_CARRY) != 0;

			/* Set remaining flags. */
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (state->a & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((state->a & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;
			state->f |= Z80_FLAG_MASK_ADD_SUBTRACT;

			state->a &= 0xFF;

			break;

		case Z80_OPCODE_AND:
			state->a &= source_value;

			state->f = 0;
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= Z80_FLAG_MASK_HALF_CARRY;
			/* TODO: Parity. */
			break;

		case Z80_OPCODE_XOR:
			state->a ^= source_value;

			state->f = 0;
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			break;

		case Z80_OPCODE_OR:
			state->a |= source_value;

			state->f = 0;
			state->f |= (state->a & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->a == 0 ? Z80_FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			break;

		case Z80_OPCODE_CP:
		{
			const unsigned int result = state->a - source_value;

			state->f = 0;

			/* Set half-carry flag. */
			state->f |= ((state->a & 0xF) - (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;

			/* Set remaining flags. */
			state->f |= (result & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= result == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (result & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((result & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;
			state->f |= Z80_FLAG_MASK_ADD_SUBTRACT;

			break;
		}

		case Z80_OPCODE_POP:
			destination_value = Read16Bit(&callbacks->read, state->stack_pointer);
			state->stack_pointer += 2;
			state->stack_pointer &= 0xFFFF;
			break;

		case Z80_OPCODE_RET_CONDITIONAL:
			if (!EvaluateCondition(state->f, instruction->metadata.condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_RET_UNCONDITIONAL:
			state->program_counter = Read16Bit(&callbacks->read, state->stack_pointer);
			state->stack_pointer += 2;
			state->stack_pointer &= 0xFFFF;
			break;

		case Z80_OPCODE_EXX:
			SWAP(state->b, state->b_);
			SWAP(state->c, state->c_);
			SWAP(state->d, state->d_);
			SWAP(state->e, state->e_);
			SWAP(state->h, state->h_);
			SWAP(state->l, state->l_);
			break;

		case Z80_OPCODE_LD_SP_HL:
			state->stack_pointer = source_value;
			break;

		case Z80_OPCODE_JP_CONDITIONAL:
			if (!EvaluateCondition(state->f, instruction->metadata.condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_JP_UNCONDITIONAL:
		case Z80_OPCODE_JP_HL:
			state->program_counter = source_value;
			break;

		case Z80_OPCODE_CB_PREFIX:
		case Z80_OPCODE_DD_PREFIX:
		case Z80_OPCODE_ED_PREFIX:
		case Z80_OPCODE_FD_PREFIX:
			/* Nothing to do here: this is handled in `Z80_DoCycle`. */
			break;

		case Z80_OPCODE_OUT:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("OUT");
			break;

		case Z80_OPCODE_IN:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("IN");
			break;

		case Z80_OPCODE_EX_SP_HL:
		{
			const unsigned int old_sp = state->stack_pointer;

			state->stack_pointer = (state->h << 8) | state->l;
			state->h = old_sp >> 8;
			state->l = old_sp & 0xFF;

			break;
		}

		case Z80_OPCODE_EX_DE_HL:
		{
			unsigned char old;

			old = state->h;
			state->h = state->d;
			state->d = old;

			old = state->l;
			state->l = state->e;
			state->e = old;

			break;
		}

		case Z80_OPCODE_DI:
			state->interrupts_enabled = cc_false;
			break;

		case Z80_OPCODE_EI:
			state->interrupts_enabled = cc_true;
			break;

		case Z80_OPCODE_PUSH:
			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, source_value >> 8);

			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, source_value & 0xFF);
			break;

		case Z80_OPCODE_CALL_CONDITIONAL:
			if (!EvaluateCondition(state->f, instruction->metadata.condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_CALL_UNCONDITIONAL:
			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter >> 8);

			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter & 0xFF);

			state->program_counter = instruction->literal;
			break;

		case Z80_OPCODE_RST:
			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter >> 8);

			--state->stack_pointer;
			state->stack_pointer &= 0xFFFF;
			callbacks->write.callback(callbacks->write.user_data, state->stack_pointer, state->program_counter & 0xFF);

			state->program_counter = instruction->metadata.embedded_literal;
			break;

		case Z80_OPCODE_RLC:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			destination_value <<= 1;
			destination_value &= 0xFF;
			destination_value |= carry ? 0x01 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RRC:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			destination_value >>= 1;
			destination_value |= carry ? 0x80 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RL:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			destination_value <<= 1;
			destination_value &= 0xFF;
			destination_value |= (state->f &= Z80_FLAG_MASK_CARRY) != 0 ? 0x01 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RR:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			destination_value >>= 1;
			destination_value |= (state->f &= Z80_FLAG_MASK_CARRY) != 0 ? 0x80 : 0;

			state->f &= ~(Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_HALF_CARRY | Z80_FLAG_MASK_ADD_SUBTRACT);
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_SLA:
		case Z80_OPCODE_SLL:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			destination_value <<= 1;
			destination_value &= 0xFF;

			state->f = 0;
			state->f |= (destination_value & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= destination_value == 0 ? Z80_FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_SRA:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			destination_value = (destination_value >> 1) | (destination_value & 0x80);

			state->f = 0;
			state->f |= (destination_value & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= destination_value == 0 ? Z80_FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_SRL:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			destination_value >>= 1;

			state->f = 0;
			state->f |= (destination_value & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= destination_value == 0 ? Z80_FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			state->f |= carry ? Z80_FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_BIT:
			state->f &= Z80_FLAG_MASK_CARRY;
			state->f |= ((destination_value & (1 << instruction->metadata.embedded_literal)) == 0) ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= Z80_FLAG_MASK_HALF_CARRY;
			break;

		case Z80_OPCODE_RES:
			destination_value &= ~(1 << instruction->metadata.embedded_literal);
			break;

		case Z80_OPCODE_SET:
			destination_value |= 1 << instruction->metadata.embedded_literal;
			break;

		case Z80_OPCODE_IN_REGISTER:
			UNIMPLEMENTED_INSTRUCTION("IN (register)");
			break;

		case Z80_OPCODE_IN_NO_REGISTER:
			UNIMPLEMENTED_INSTRUCTION("IN (no register)");
			break;

		case Z80_OPCODE_OUT_REGISTER:
			UNIMPLEMENTED_INSTRUCTION("OUT (register)");
			break;

		case Z80_OPCODE_OUT_NO_REGISTER:
			UNIMPLEMENTED_INSTRUCTION("OUT (no register)");
			break;

		case Z80_OPCODE_SBC_HL:
			UNIMPLEMENTED_INSTRUCTION("SBC HL");
			break;

		case Z80_OPCODE_ADC_HL:
		{
			state->l += source_value & 0xFF;

			state->f = 0;

			/* Set half-carry flag. */
			/* TODO */
			/*state->f |= ((state->a & 0xF) + (source_value & 0xF) & 0x10) != 0 ? Z80_FLAG_MASK_HALF_CARRY : 0;*/

			/* Perform addition. */
			state->h += source_value >> 8;
			state->h += state->l >> 8; /* TODO: Broken. */
			state->h += (state->f & Z80_FLAG_MASK_CARRY) != 0;

			/* Set remaining flags. */
			state->f |= (state->h & 0x80) != 0 ? Z80_FLAG_MASK_SIGN : 0;
			state->f |= state->h == 0 ? Z80_FLAG_MASK_ZERO : 0;
			state->f |= (state->h & 0x100) != 0 ? Z80_FLAG_MASK_CARRY : 0;
			state->f |= ((state->h & 0x80) != 0) != ((state->f & Z80_FLAG_MASK_CARRY) != 0) ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;

			state->h &= 0xFF;
			state->l &= 0xFF;

			break;
		}

		case Z80_OPCODE_NEG:
			UNIMPLEMENTED_INSTRUCTION("NEG");
			break;

		case Z80_OPCODE_RETN:
			UNIMPLEMENTED_INSTRUCTION("RETN");
			break;

		case Z80_OPCODE_RETI:
			UNIMPLEMENTED_INSTRUCTION("RETI");
			break;

		case Z80_OPCODE_IM:
			UNIMPLEMENTED_INSTRUCTION("IM");
			break;

		case Z80_OPCODE_LD_I_A:
			UNIMPLEMENTED_INSTRUCTION("LD I,A");
			break;

		case Z80_OPCODE_LD_R_A:
			UNIMPLEMENTED_INSTRUCTION("LD R,A");
			break;

		case Z80_OPCODE_LD_A_I:
			UNIMPLEMENTED_INSTRUCTION("LD A,I");
			break;

		case Z80_OPCODE_LD_A_R:
			UNIMPLEMENTED_INSTRUCTION("LD A,R");
			break;

		case Z80_OPCODE_RRD:
			UNIMPLEMENTED_INSTRUCTION("RRD");
			break;

		case Z80_OPCODE_RLD:
			UNIMPLEMENTED_INSTRUCTION("RLD");
			break;

		case Z80_OPCODE_LDI:
		{
			const unsigned int de = (state->d << 8) | state->e;
			const unsigned int hl = (state->h << 8) | state->l;

			callbacks->write.callback(callbacks->write.user_data, de, callbacks->read.callback(callbacks->read.user_data, hl));

			/* Increment 'hl'. */
			++state->l;
			state->h += state->l >> 8; /* TODO: Broken. */
			state->h &= 0xFF;
			state->l &= 0xFF;

			/* Increment 'de'. */
			++state->e;
			state->d += state->e >> 8; /* TODO: Broken. */
			state->d &= 0xFF;
			state->e &= 0xFF;

			/* Decrement 'bc'. */
			--state->c;
			state->b += state->c >> 8; /* TODO: Broken. */
			state->b &= 0xFF;
			state->c &= 0xFF;

			state->f &= Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_ZERO | Z80_FLAG_MASK_SIGN;
			state->f |= (state->b | state->c) != 0 ? Z80_FLAG_MASK_PARITY_OVERFLOW : 0;

			break;
		}

		case Z80_OPCODE_LDD:
			UNIMPLEMENTED_INSTRUCTION("LDD");
			break;

		case Z80_OPCODE_LDIR:
		{
			unsigned int bc;
			unsigned int de;
			unsigned int hl;

			bc = (state->b << 8) | state->c;
			de = (state->d << 8) | state->e;
			hl = (state->h << 8) | state->l;

			do
			{
				callbacks->write.callback(callbacks->write.user_data, de, callbacks->read.callback(callbacks->read.user_data, hl));

				++hl;
				hl &= 0xFFFF;

				++de;
				de &= 0xFFFF;

				--bc;
				bc &= 0xFFFF;

			} while (bc != 0);

			state->b = 0;
			state->c = 0;
			state->d = de >> 8;
			state->e = de & 0xFF;
			state->h = hl >> 8;
			state->l = hl & 0xFF;

			state->f &= Z80_FLAG_MASK_CARRY | Z80_FLAG_MASK_ZERO | Z80_FLAG_MASK_SIGN;

			break;
		}

		case Z80_OPCODE_LDDR:
			UNIMPLEMENTED_INSTRUCTION("LDDR");
			break;

		case Z80_OPCODE_CPI:
			UNIMPLEMENTED_INSTRUCTION("CPI");
			break;

		case Z80_OPCODE_CPD:
			UNIMPLEMENTED_INSTRUCTION("CPD");
			break;

		case Z80_OPCODE_CPIR:
			UNIMPLEMENTED_INSTRUCTION("CPIR");
			break;

		case Z80_OPCODE_CPDR:
			UNIMPLEMENTED_INSTRUCTION("CPDR");
			break;

		case Z80_OPCODE_INI:
			UNIMPLEMENTED_INSTRUCTION("INI");
			break;

		case Z80_OPCODE_IND:
			UNIMPLEMENTED_INSTRUCTION("IND");
			break;

		case Z80_OPCODE_INIR:
			UNIMPLEMENTED_INSTRUCTION("INIR");
			break;

		case Z80_OPCODE_INDR:
			UNIMPLEMENTED_INSTRUCTION("INDR");
			break;

		case Z80_OPCODE_OUTI:
			UNIMPLEMENTED_INSTRUCTION("OTDI");
			break;

		case Z80_OPCODE_OUTD:
			UNIMPLEMENTED_INSTRUCTION("OUTD");
			break;

		case Z80_OPCODE_OTIR:
			UNIMPLEMENTED_INSTRUCTION("OTIR");
			break;

		case Z80_OPCODE_OTDR:
			UNIMPLEMENTED_INSTRUCTION("OTDR");
			break;
	}

	if (instruction->metadata.write_destination)
		WriteOperand(state, instruction, 1, destination_value, &callbacks->write);
}

void Z80_DoCycle(Z80_State *state, const Z80_ReadAndWriteCallbacks *callbacks)
{
	const DecodeInstructionCallbackData callback_data = {state, &callbacks->read};
	const Z80_ReadInstructionCallback read_instruction_callback = {DecodeInstructionCallback, (void*)&callback_data};

	Z80_Instruction instruction;

	Z80_DecodeInstruction(&instruction, state->instruction_mode, state->register_mode, &read_instruction_callback);

	Z80_ExecuteInstruction(state, &instruction, callbacks);

	switch (instruction.metadata.opcode)
	{
		default:
			state->register_mode = Z80_REGISTER_MODE_HL;
			state->instruction_mode = Z80_INSTRUCTION_MODE_NORMAL;
			break;

		case Z80_OPCODE_CB_PREFIX:
			state->instruction_mode = Z80_INSTRUCTION_MODE_BITS;
			break;

		case Z80_OPCODE_ED_PREFIX:
			state->instruction_mode = Z80_INSTRUCTION_MODE_MISC;
			break;

		case Z80_OPCODE_DD_PREFIX:
			state->register_mode = Z80_REGISTER_MODE_IX;
			break;

		case Z80_OPCODE_FD_PREFIX:
			state->register_mode = Z80_REGISTER_MODE_IY;
			break;
	}
}
