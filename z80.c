/*
Useful:
https://floooh.github.io/2021/12/06/z80-instruction-timing.html
*/

/* TODO: R and I registers. */

#include "z80.h"

#include <assert.h>
#include <stddef.h>

#include "clowncommon.h"

#include "error.h"

/* TODO: Use a bit index here instead of a bitmask. Apply this change to the 68k emulator too. */
#define SIGN_EXTEND(value, bitmask) (((value) & ((bitmask) >> 1u)) - ((value) & (((bitmask) >> 1u) + 1u)))

#define UNIMPLEMENTED_INSTRUCTION(instruction) PrintError("Unimplemented instruction " instruction " used at 0x%X", z80->state->program_counter)

typedef enum InstructionMode
{
	INSTRUCTION_MODE_NORMAL,
	INSTRUCTION_MODE_BITS,
	INSTRUCTION_MODE_MISC
} InstructionMode;

enum
{
	FLAG_BIT_CARRY = 0,
	FLAG_BIT_ADD_SUBTRACT = 1,
	FLAG_BIT_PARITY_OVERFLOW = 2,
	FLAG_BIT_HALF_CARRY = 4,
	FLAG_BIT_ZERO = 6,
	FLAG_BIT_SIGN = 7,
	FLAG_MASK_CARRY = 1 << FLAG_BIT_CARRY,
	FLAG_MASK_ADD_SUBTRACT = 1 << FLAG_BIT_ADD_SUBTRACT,
	FLAG_MASK_PARITY_OVERFLOW = 1 << FLAG_BIT_PARITY_OVERFLOW,
	FLAG_MASK_HALF_CARRY = 1 << FLAG_BIT_HALF_CARRY,
	FLAG_MASK_ZERO = 1 << FLAG_BIT_ZERO,
	FLAG_MASK_SIGN = 1 << FLAG_BIT_SIGN
};

typedef struct Instruction
{
	const InstructionMetadata *metadata;
	unsigned int literal;
	unsigned int address;
	cc_bool double_prefix_mode;
} Instruction;

static cc_bool EvaluateCondition(unsigned char flags, Z80_Condition condition)
{
	switch (condition)
	{
		case Z80_CONDITION_NOT_ZERO:
			return (flags & FLAG_MASK_ZERO) == 0;

		case Z80_CONDITION_ZERO:
			return (flags & FLAG_MASK_ZERO) != 0;

		case Z80_CONDITION_NOT_CARRY:
			return (flags & FLAG_MASK_CARRY) == 0;

		case Z80_CONDITION_CARRY:
			return (flags & FLAG_MASK_CARRY) != 0;

		case Z80_CONDITION_PARITY_OVERFLOW:
			return (flags & FLAG_MASK_PARITY_OVERFLOW) == 0;

		case Z80_CONDITION_PARITY_EQUALITY:
			return (flags & FLAG_MASK_PARITY_OVERFLOW) != 0;

		case Z80_CONDITION_PLUS:
			return (flags & FLAG_MASK_SIGN) == 0;

		case Z80_CONDITION_MINUS:
			return (flags & FLAG_MASK_SIGN) != 0;

		default:
			/* Should never occur. */
			assert(0);
			return cc_false;
	}
}

static unsigned int MemoryRead(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks, unsigned int address)
{
	/* Memory accesses take 3 cycles. */
	z80->state->cycles += 3;

	return callbacks->read(callbacks->user_data, address);
}

static void MemoryWrite(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks, unsigned int address, unsigned int data)
{
	/* Memory accesses take 3 cycles. */
	z80->state->cycles += 3;

	callbacks->write(callbacks->user_data, address, data);
}

static unsigned int InstructionMemoryRead(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks)
{
	const unsigned int data = MemoryRead(z80, callbacks, z80->state->program_counter);

	++z80->state->program_counter;
	z80->state->program_counter &= 0xFFFF;

	return data;
}

static unsigned int OpcodeFetch(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks)
{
	/* Opcode fetches take an extra cycle. */
	++z80->state->cycles;

	++z80->state->r;
	z80->state->r &= 0xFF;

	return InstructionMemoryRead(z80, callbacks);
}

/* TODO: Should the bytes be written in reverse order? */
static unsigned int MemoryRead16Bit(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks, unsigned int address)
{
	unsigned int value;
	value = MemoryRead(z80, callbacks, address + 0);
	value |= MemoryRead(z80, callbacks, address + 1) << 8;
	return value;
}

static void MemoryWrite16Bit(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks, unsigned int address, unsigned int value)
{
	MemoryWrite(z80, callbacks, address + 0, value & 0xFF);
	MemoryWrite(z80, callbacks, address + 1, value >> 8);
}

static unsigned int ReadOperand(const Z80 *z80, const Instruction *instruction, Z80_Operand operand, const Z80_ReadAndWriteCallbacks *callbacks)
{
	unsigned int value;

	switch (operand)
	{
		default:
		case Z80_OPERAND_NONE:
			/* Should never happen. */
			assert(0);
			/* Fallthrough */
		case Z80_OPERAND_A:
			value = z80->state->a;
			break;

		case Z80_OPERAND_B:
			value = z80->state->b;
			break;

		case Z80_OPERAND_C:
			value = z80->state->c;
			break;

		case Z80_OPERAND_D:
			value = z80->state->d;
			break;

		case Z80_OPERAND_E:
			value = z80->state->e;
			break;

		case Z80_OPERAND_H:
			value = z80->state->h;
			break;

		case Z80_OPERAND_L:
			value = z80->state->l;
			break;

		case Z80_OPERAND_IXH:
			value = z80->state->ixh;
			break;

		case Z80_OPERAND_IXL:
			value = z80->state->ixl;
			break;

		case Z80_OPERAND_IYH:
			value = z80->state->iyh;
			break;

		case Z80_OPERAND_IYL:
			value = z80->state->iyl;
			break;

		case Z80_OPERAND_AF:
			value = ((unsigned int)z80->state->a << 8) | z80->state->f;
			break;

		case Z80_OPERAND_BC:
			value = ((unsigned int)z80->state->b << 8) | z80->state->c;
			break;

		case Z80_OPERAND_DE:
			value = ((unsigned int)z80->state->d << 8) | z80->state->e;
			break;

		case Z80_OPERAND_HL:
			value = ((unsigned int)z80->state->h << 8) | z80->state->l;
			break;

		case Z80_OPERAND_IX:
			value = ((unsigned int)z80->state->ixh << 8) | z80->state->ixl;
			break;

		case Z80_OPERAND_IY:
			value = ((unsigned int)z80->state->iyh << 8) | z80->state->iyl;
			break;

		case Z80_OPERAND_PC:
			value = z80->state->program_counter;
			break;

		case Z80_OPERAND_SP:
			value = z80->state->stack_pointer;
			break;

		case Z80_OPERAND_LITERAL_8BIT:
		case Z80_OPERAND_LITERAL_16BIT:
			value = instruction->literal;
			break;

		case Z80_OPERAND_BC_INDIRECT:
		case Z80_OPERAND_DE_INDIRECT:
		case Z80_OPERAND_HL_INDIRECT:
		case Z80_OPERAND_IX_INDIRECT:
		case Z80_OPERAND_IY_INDIRECT:
		case Z80_OPERAND_ADDRESS:
			value = MemoryRead(z80, callbacks, instruction->address);

			if (instruction->metadata->indirect_16bit)
				value = MemoryRead(z80, callbacks, instruction->address + 1) << 8;

			break;
	}

	return value;
}

static void WriteOperand(const Z80 *z80, const Instruction *instruction, Z80_Operand operand, unsigned int value, const Z80_ReadAndWriteCallbacks *callbacks)
{
	switch (operand)
	{
		default:
		case Z80_OPERAND_NONE:
		case Z80_OPERAND_LITERAL_8BIT:
		case Z80_OPERAND_LITERAL_16BIT:
			/* Should never happen. */
			assert(0);
			break;

		case Z80_OPERAND_A:
			z80->state->a = value;
			break;

		case Z80_OPERAND_B:
			z80->state->b = value;
			break;

		case Z80_OPERAND_C:
			z80->state->c = value;
			break;

		case Z80_OPERAND_D:
			z80->state->d = value;
			break;

		case Z80_OPERAND_E:
			z80->state->e = value;
			break;

		case Z80_OPERAND_H:
			z80->state->h = value;
			break;

		case Z80_OPERAND_L:
			z80->state->l = value;
			break;

		case Z80_OPERAND_IXH:
			z80->state->ixh = value;
			break;

		case Z80_OPERAND_IXL:
			z80->state->ixl = value;
			break;

		case Z80_OPERAND_IYH:
			z80->state->iyh = value;
			break;

		case Z80_OPERAND_IYL:
			z80->state->iyl = value;
			break;

		case Z80_OPERAND_AF:
			z80->state->a = value >> 8;
			z80->state->f = value & 0xFF;
			break;

		case Z80_OPERAND_BC:
			z80->state->b = value >> 8;
			z80->state->c = value & 0xFF;
			break;

		case Z80_OPERAND_DE:
			z80->state->d = value >> 8;
			z80->state->e = value & 0xFF;
			break;

		case Z80_OPERAND_HL:
			z80->state->h = value >> 8;
			z80->state->l = value & 0xFF;
			break;

		case Z80_OPERAND_IX:
			z80->state->ixh = value >> 8;
			z80->state->ixl = value & 0xFF;
			break;

		case Z80_OPERAND_IY:
			z80->state->iyh = value >> 8;
			z80->state->iyl = value & 0xFF;
			break;

		case Z80_OPERAND_PC:
			z80->state->program_counter = value;
			break;

		case Z80_OPERAND_SP:
			z80->state->stack_pointer = value;
			break;

		case Z80_OPERAND_BC_INDIRECT:
		case Z80_OPERAND_DE_INDIRECT:
		case Z80_OPERAND_HL_INDIRECT:
		case Z80_OPERAND_IX_INDIRECT:
		case Z80_OPERAND_IY_INDIRECT:
		case Z80_OPERAND_ADDRESS:
			if (instruction->metadata->indirect_16bit)
				MemoryWrite16Bit(z80, callbacks, instruction->address, value);
			else
				MemoryWrite(z80, callbacks, instruction->address, value);

			break;
	}
}

static void DecodeInstructionMetadata(InstructionMetadata *metadata, InstructionMode instruction_mode, Z80_RegisterMode register_mode, unsigned char opcode)
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

	unsigned int i;

	metadata->has_displacement = cc_false;

	metadata->operands[0] = Z80_OPERAND_NONE;
	metadata->operands[1] = Z80_OPERAND_NONE;
	metadata->read_destination = cc_false;
	metadata->write_destination = cc_false;
	metadata->indirect_16bit = cc_false;

	switch (instruction_mode)
	{
		case INSTRUCTION_MODE_NORMAL:
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
									metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
									break;

								case 3:
									metadata->opcode = Z80_OPCODE_JR_UNCONDITIONAL;
									metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
									break;

								case 4:
								case 5:
								case 6:
								case 7:
									metadata->opcode = Z80_OPCODE_JR_CONDITIONAL;
									metadata->operands[0] = Z80_OPERAND_LITERAL_8BIT;
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
							static const Z80_Opcode opcodes[8] = {Z80_OPCODE_RLCA, Z80_OPCODE_RRCA, Z80_OPCODE_RLA, Z80_OPCODE_RRA, Z80_OPCODE_DAA, Z80_OPCODE_CPL, Z80_OPCODE_SCF, Z80_OPCODE_CCF};
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
					metadata->operands[1] = Z80_OPERAND_A;
					metadata->read_destination = cc_true;
					metadata->write_destination = metadata->opcode != Z80_OPCODE_CP;
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

									if (register_mode != Z80_REGISTER_MODE_HL)
										metadata->has_displacement = cc_true;

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
									metadata->operands[1] = Z80_OPERAND_HL;
									metadata->read_destination = cc_true;
									metadata->write_destination = cc_true;
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
							metadata->operands[1] = Z80_OPERAND_A;
							metadata->read_destination = cc_true;
							metadata->write_destination = metadata->opcode != Z80_OPCODE_CP;
							break;

						case 7:
							metadata->opcode = Z80_OPCODE_RST;
							metadata->embedded_literal = y * 8;
							break;
					}

					break;
			}

			break;

		case INSTRUCTION_MODE_BITS:
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
					metadata->embedded_literal = 1u << y;
					metadata->read_destination = cc_true;
					break;

				case 2:
					metadata->opcode = Z80_OPCODE_RES;
					metadata->operands[1] = registers[z];
					metadata->embedded_literal = ~(1u << y);
					metadata->read_destination = cc_true;
					metadata->write_destination = cc_true;
					break;

				case 3:
					metadata->opcode = Z80_OPCODE_SET;
					metadata->operands[1] = registers[z];
					metadata->embedded_literal = 1u << y;
					metadata->read_destination = cc_true;
					metadata->write_destination = cc_true;
					break;
			}

			break;

		case INSTRUCTION_MODE_MISC:
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
							metadata->operands[1] = Z80_OPERAND_HL;
							metadata->read_destination = cc_true;
							metadata->write_destination = cc_true;

							break;

						case 3:
							metadata->opcode = Z80_OPCODE_LD_16BIT;
							metadata->write_destination = cc_true;
							metadata->indirect_16bit = cc_true;

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
							metadata->operands[0] = Z80_OPERAND_A;
							metadata->operands[1] = Z80_OPERAND_A;
							metadata->write_destination = cc_true;
							break;

						case 5:
							if (y != 1)
								metadata->opcode = Z80_OPCODE_RETN;
							else
								metadata->opcode = Z80_OPCODE_RETI;

							break;

						case 6:
						{
							static const unsigned int interrupt_modes[4] = {0, 0, 1, 2};

							metadata->opcode = Z80_OPCODE_IM;
							metadata->embedded_literal = interrupt_modes[y & 3];

							break;
						}

						case 7:
						{
							static const Z80_Opcode assorted_opcodes[8] = {Z80_OPCODE_LD_I_A, Z80_OPCODE_LD_R_A, Z80_OPCODE_LD_A_I, Z80_OPCODE_LD_A_R, Z80_OPCODE_RRD, Z80_OPCODE_RLD, Z80_OPCODE_NOP, Z80_OPCODE_NOP};
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

	/* Convert HL to IX and IY if needed. */
	for (i = 0; i < 2; ++i)
	{
		if (metadata->operands[i ^ 1] != Z80_OPERAND_HL_INDIRECT
		 && metadata->operands[i ^ 1] != Z80_OPERAND_IX_INDIRECT
		 && metadata->operands[i ^ 1] != Z80_OPERAND_IY_INDIRECT)
		{
			switch (metadata->operands[i])
			{
				default:
					break;

				case Z80_OPERAND_H:
					switch (register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							break;

						case Z80_REGISTER_MODE_IX:
							metadata->operands[i] = Z80_OPERAND_IXH;
							break;

						case Z80_REGISTER_MODE_IY:
							metadata->operands[i] = Z80_OPERAND_IYH;
							break;
					}

					break;

				case Z80_OPERAND_L:
					switch (register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							break;

						case Z80_REGISTER_MODE_IX:
							metadata->operands[i] = Z80_OPERAND_IXL;
							break;

						case Z80_REGISTER_MODE_IY:
							metadata->operands[i] = Z80_OPERAND_IYL;
							break;
					}

					break;

				case Z80_OPERAND_HL:
					switch (register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							break;

						case Z80_REGISTER_MODE_IX:
							metadata->operands[i] = Z80_OPERAND_IX;
							break;

						case Z80_REGISTER_MODE_IY:
							metadata->operands[i] = Z80_OPERAND_IY;
							break;
					}

					break;

				case Z80_OPERAND_HL_INDIRECT:
					switch (register_mode)
					{
						case Z80_REGISTER_MODE_HL:
							break;

						case Z80_REGISTER_MODE_IX:
							metadata->operands[i] = Z80_OPERAND_IX_INDIRECT;
							metadata->has_displacement = cc_true;
							break;

						case Z80_REGISTER_MODE_IY:
							metadata->operands[i] = Z80_OPERAND_IY_INDIRECT;
							metadata->has_displacement = cc_true;
							break;
					}

					break;
			}
		}
	}
}

static void DecodeInstruction(const Z80 *z80, Instruction *instruction, const Z80_ReadAndWriteCallbacks *callbacks)
{
	unsigned int opcode;
	int displacement;
	unsigned int i;

	opcode = OpcodeFetch(z80, callbacks);

	instruction->metadata = &z80->constant->instruction_metadata_lookup_normal[z80->state->register_mode][opcode];

	/* Obtain displacement byte if one exists. */
	if (instruction->metadata->has_displacement)
	{
		displacement = InstructionMemoryRead(z80, callbacks);
		displacement = SIGN_EXTEND(displacement, 0xFF);

		/* The displacement byte adds 5 cycles on top of the 3 required to read it. */
		z80->state->cycles += 5;
	}

	instruction->double_prefix_mode = cc_false;

	/* Handle prefix instructions. */
	switch (instruction->metadata->opcode)
	{
		default:
			/* Nothing to do here. */
			break;

		case Z80_OPCODE_CB_PREFIX:
			if (z80->state->register_mode == Z80_REGISTER_MODE_HL)
			{
				/* Normal mode. */
				opcode = OpcodeFetch(z80, callbacks);
			}
			else
			{
				/* Double-prefix mode. */
				instruction->double_prefix_mode = cc_true;

				/* Yes, double-prefix mode fetches the opcode with a regular memory read. */
				opcode = InstructionMemoryRead(z80, callbacks);

				/* Reading the opcode is overlaid with the 5 displacement cycles, so the above memory read doesn't cost 3 cycles. */
				z80->state->cycles -= 3;
			}

			instruction->metadata = &z80->constant->instruction_metadata_lookup_bits[opcode];

			break;

		case Z80_OPCODE_ED_PREFIX:
			opcode = OpcodeFetch(z80, callbacks);
			instruction->metadata = &z80->constant->instruction_metadata_lookup_misc[opcode];
			break;
	}

	/* Obtain literal data. */
	switch (instruction->metadata->operands[0])
	{
		default:
			/* Nothing to do here. */
			break;

		case Z80_OPERAND_LITERAL_8BIT:
			instruction->literal = InstructionMemoryRead(z80, callbacks);

			if (instruction->metadata->has_displacement)
			{
				/* Reading the literal is overlaid with the 5 displacement cycles, so the above memory read doesn't cost 3 cycles. */
				z80->state->cycles -= 3;
			}

			break;

		case Z80_OPERAND_LITERAL_16BIT:
			instruction->literal = InstructionMemoryRead(z80, callbacks);
			instruction->literal |= InstructionMemoryRead(z80, callbacks) << 8;
			break;
	}

	/* Pre-calculate the address of indirect memory operands. */
	for (i = 0; i < 2; ++i)
	{
		switch (instruction->metadata->operands[i])
		{
			default:
				break;

			case Z80_OPERAND_BC_INDIRECT:
				instruction->address = ((unsigned int)z80->state->b << 8) | z80->state->c;
				break;

			case Z80_OPERAND_DE_INDIRECT:
				instruction->address = ((unsigned int)z80->state->d << 8) | z80->state->e;
				break;

			case Z80_OPERAND_HL_INDIRECT:
				instruction->address = ((unsigned int)z80->state->h << 8) | z80->state->l;
				break;

			case Z80_OPERAND_IX_INDIRECT:
				instruction->address = (((unsigned int)z80->state->ixh << 8) | z80->state->ixl) + displacement;
				break;

			case Z80_OPERAND_IY_INDIRECT:
				instruction->address = (((unsigned int)z80->state->iyh << 8) | z80->state->iyl) + displacement;
				break;

			case Z80_OPERAND_ADDRESS:
				instruction->address = InstructionMemoryRead(z80, callbacks);
				instruction->address |= InstructionMemoryRead(z80, callbacks) << 8;
				break;
		}
	}
}

#define SWAP(a, b) \
	swap_holder = a; \
	a = b; \
	b = swap_holder

#define CONDITION_CARRY_BASE(variable, bit) z80->state->f |= (variable >> (bit - FLAG_BIT_CARRY)) & FLAG_MASK_CARRY
#define CONDITION_HALF_CARRY_BASE(bit) z80->state->f |= ((source_value ^ destination_value ^ result_value) >> (bit - FLAG_BIT_HALF_CARRY)) & FLAG_MASK_HALF_CARRY
#define CONDITION_OVERFLOW_BASE(bit) z80->state->f |= ((~(source_value ^ destination_value) & (source_value ^ result_value)) >> (bit - FLAG_BIT_PARITY_OVERFLOW)) & FLAG_MASK_PARITY_OVERFLOW

#define CONDITION_HALF_CARRY_16BIT CONDITION_HALF_CARRY_BASE(12)
#define CONDITION_OVERFLOW_16BIT CONDITION_OVERFLOW_BASE(15)
#define CONDITION_CARRY_16BIT CONDITION_CARRY_BASE(result_value_long, 16)

#define CONDITION_SIGN z80->state->f |= (result_value >> (7 - FLAG_BIT_SIGN)) & FLAG_MASK_SIGN
#define CONDITION_ZERO z80->state->f |= result_value == 0 ? FLAG_MASK_ZERO : 0
#define CONDITION_HALF_CARRY CONDITION_HALF_CARRY_BASE(4)
#define CONDITION_OVERFLOW CONDITION_OVERFLOW_BASE(7)
#define CONDITION_PARITY z80->state->f |= z80->constant->parity_lookup[result_value]
#define CONDITION_CARRY CONDITION_CARRY_BASE(result_value, 8)

static void ExecuteInstruction(const Z80 *z80, const Instruction *instruction, const Z80_ReadAndWriteCallbacks *callbacks)
{
	unsigned int source_value;
	unsigned int destination_value;
	unsigned int result_value;
	unsigned char swap_holder;

	z80->state->register_mode = Z80_REGISTER_MODE_HL;

	if (instruction->double_prefix_mode)
	{
		/* Handle double-prefix instructions. */
		destination_value = ReadOperand(z80, instruction, z80->state->register_mode == Z80_REGISTER_MODE_IX ? Z80_OPERAND_IX_INDIRECT : Z80_OPERAND_IY_INDIRECT, callbacks);
	}
	else
	{
		if (instruction->metadata->operands[0] != Z80_OPERAND_NONE)
			source_value = ReadOperand(z80, instruction, instruction->metadata->operands[0], callbacks);

		if (instruction->metadata->read_destination)
			destination_value = ReadOperand(z80, instruction, instruction->metadata->operands[1], callbacks);
	}

	switch (instruction->metadata->opcode)
	{
		case Z80_OPCODE_NOP:
			/* Does nothing, naturally. */
			break;

		case Z80_OPCODE_EX_AF_AF:
			SWAP(z80->state->a, z80->state->a_);
			SWAP(z80->state->f, z80->state->f_);
			break;

		case Z80_OPCODE_DJNZ:
			/* This instruction takes an extra cycle. */
			z80->state->cycles += 1;

			--z80->state->b;
			z80->state->b &= 0xFF;

			if (z80->state->b != 0)
			{
				z80->state->program_counter += SIGN_EXTEND(source_value, 0xFF);

				/* Branching takes 5 cycles. */
				z80->state->cycles += 5;
			}

			break;

		case Z80_OPCODE_JR_CONDITIONAL:
			if (!EvaluateCondition(z80->state->f, instruction->metadata->condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_JR_UNCONDITIONAL:
			z80->state->program_counter += SIGN_EXTEND(source_value, 0xFF);

			/* Branching takes 5 cycles. */
			z80->state->cycles += 5;

			break;

		case Z80_OPCODE_LD_16BIT:
			result_value = source_value;
			break;

		case Z80_OPCODE_ADD_HL:
		{
			/* Needs to be 'unsigned long' so that it can hold the carry bit. */
			const unsigned long result_value_long = (unsigned long)source_value + (unsigned long)destination_value;
			result_value = result_value_long & 0xFFFF;

			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;

			CONDITION_CARRY_16BIT;
			CONDITION_HALF_CARRY_16BIT;

			/* This instruction requires an extra 7 cycles. */
			z80->state->cycles += 7;

			break;
		}

		case Z80_OPCODE_LD_8BIT:
			result_value = source_value;
			break;

		case Z80_OPCODE_INC_16BIT:
			result_value = (destination_value + 1) & 0xFFFF;

			/* This instruction requires an extra 2 cycles. */
			z80->state->cycles += 2;

			break;

		case Z80_OPCODE_DEC_16BIT:
			result_value = (destination_value - 1) & 0xFFFF;

			/* This instruction requires an extra 2 cycles. */
			z80->state->cycles += 2;

			break;

		case Z80_OPCODE_INC_8BIT:
			source_value = 1;
			result_value = (destination_value + source_value) & 0xFF;

			z80->state->f &= FLAG_MASK_CARRY;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;

		case Z80_OPCODE_DEC_8BIT:
			source_value = -1;
			result_value = (destination_value + source_value) & 0xFF;

			z80->state->f &= FLAG_MASK_CARRY;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			z80->state->f |= FLAG_MASK_ADD_SUBTRACT;
			/* TODO: I thought this needed to be inverted? */
			/*z80->state->f ^= FLAG_MASK_HALF_CARRY;*/

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;


			break;

		case Z80_OPCODE_RLCA:
		{
			const cc_bool carry = (z80->state->a & 0x80) != 0;

			z80->state->a <<= 1;
			z80->state->a &= 0xFF;
			z80->state->a |= carry ? 0x01 : 0;

			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RRCA:
		{
			const cc_bool carry = (z80->state->a & 0x01) != 0;

			z80->state->a >>= 1;
			z80->state->a |= carry ? 0x80 : 0;

			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RLA:
		{
			const cc_bool carry = (z80->state->a & 0x80) != 0;

			z80->state->a <<= 1;
			z80->state->a &= 0xFF;
			z80->state->a |= (z80->state->f &= FLAG_MASK_CARRY) != 0 ? 1 : 0;

			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_RRA:
		{
			const cc_bool carry = (z80->state->a & 0x01) != 0;

			z80->state->a >>= 1;
			z80->state->a |= (z80->state->f &= FLAG_MASK_CARRY) != 0 ? 0x80 : 0;

			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			break;
		}

		case Z80_OPCODE_DAA:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("DAA");
			break;

		case Z80_OPCODE_CPL:
			z80->state->a = ~z80->state->a;
			z80->state->a &= 0xFF;

			z80->state->f |= FLAG_MASK_HALF_CARRY | FLAG_MASK_ADD_SUBTRACT;

			break;

		case Z80_OPCODE_SCF:
			z80->state->f &= FLAG_MASK_SIGN | FLAG_MASK_ZERO | FLAG_MASK_PARITY_OVERFLOW;
			z80->state->f |= FLAG_MASK_CARRY;
			break;

		case Z80_OPCODE_CCF:
			z80->state->f &= ~(FLAG_MASK_ADD_SUBTRACT | FLAG_MASK_HALF_CARRY);

			z80->state->f |= (z80->state->f & FLAG_MASK_CARRY) != 0 ? FLAG_MASK_HALF_CARRY : 0;
			z80->state->f ^= FLAG_MASK_CARRY;

			break;

		case Z80_OPCODE_HALT:
			/* TODO */
			UNIMPLEMENTED_INSTRUCTION("HALT");
			break;

		case Z80_OPCODE_ADD_A:
			result_value = destination_value + source_value;

			z80->state->f = 0;
			CONDITION_CARRY;

			result_value &= 0xFF;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			break;

		case Z80_OPCODE_ADC_A:
			result_value = destination_value + source_value + ((z80->state->f & FLAG_MASK_CARRY) != 0 ? 1 : 0);

			z80->state->f = 0;
			CONDITION_CARRY;

			result_value &= 0xFF;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			break;

		case Z80_OPCODE_NEG:
			destination_value = 0;
			/* Fallthrough */
		case Z80_OPCODE_CP:
		case Z80_OPCODE_SUB:
			source_value = ~source_value;

			result_value = destination_value + source_value + 1;

			z80->state->f = 0;
			CONDITION_CARRY;

			result_value &= 0xFF;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			/* TODO: I thought these needed to be inverted? */
			/*z80->state->f ^= FLAG_MASK_HALF_CARRY | FLAG_MASK_CARRY;*/
			z80->state->f |= FLAG_MASK_ADD_SUBTRACT;

			break;

		case Z80_OPCODE_SBC_A:
			source_value = ~source_value;

			result_value = destination_value + source_value + ((z80->state->f & FLAG_MASK_CARRY) != 0 ? 0 : 1);

			z80->state->f = 0;
			CONDITION_CARRY;

			result_value &= 0xFF;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY;
			CONDITION_OVERFLOW;

			/* TODO: I thought these needed to be inverted? */
			/*z80->state->f ^= FLAG_MASK_HALF_CARRY | FLAG_MASK_CARRY;*/
			z80->state->f |= FLAG_MASK_ADD_SUBTRACT;

			break;

		case Z80_OPCODE_AND:
			result_value = destination_value & source_value;

			z80->state->f = 0;
			CONDITION_SIGN;
			CONDITION_ZERO;
			z80->state->f |= FLAG_MASK_HALF_CARRY;
			CONDITION_PARITY;
			break;

		case Z80_OPCODE_XOR:
			result_value = destination_value ^ source_value;

			z80->state->f = 0;
			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_PARITY;
			break;

		case Z80_OPCODE_OR:
			result_value = destination_value | source_value;

			z80->state->f = 0;
			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_PARITY;
			break;

		case Z80_OPCODE_POP:
			result_value = MemoryRead16Bit(z80, callbacks, z80->state->stack_pointer);
			z80->state->stack_pointer += 2;
			z80->state->stack_pointer &= 0xFFFF;
			break;

		case Z80_OPCODE_RET_CONDITIONAL:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			if (!EvaluateCondition(z80->state->f, instruction->metadata->condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_RET_UNCONDITIONAL:
			z80->state->program_counter = MemoryRead16Bit(z80, callbacks, z80->state->stack_pointer);
			z80->state->stack_pointer += 2;
			z80->state->stack_pointer &= 0xFFFF;
			break;

		case Z80_OPCODE_EXX:
			SWAP(z80->state->b, z80->state->b_);
			SWAP(z80->state->c, z80->state->c_);
			SWAP(z80->state->d, z80->state->d_);
			SWAP(z80->state->e, z80->state->e_);
			SWAP(z80->state->h, z80->state->h_);
			SWAP(z80->state->l, z80->state->l_);
			break;

		case Z80_OPCODE_LD_SP_HL:
			/* This instruction requires 2 cycles. */
			z80->state->cycles += 2;

			z80->state->stack_pointer = source_value;
			break;

		case Z80_OPCODE_JP_CONDITIONAL:
			if (!EvaluateCondition(z80->state->f, instruction->metadata->condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_JP_UNCONDITIONAL:
		case Z80_OPCODE_JP_HL:
			z80->state->program_counter = source_value;
			break;

		case Z80_OPCODE_CB_PREFIX:
		case Z80_OPCODE_ED_PREFIX:
			/* Should never occur: these are handled by `DecodeInstruction`. */
			assert(0);
			break;

		case Z80_OPCODE_DD_PREFIX:
			z80->state->register_mode = Z80_REGISTER_MODE_IX;
			break;

		case Z80_OPCODE_FD_PREFIX:
			z80->state->register_mode = Z80_REGISTER_MODE_IX;
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
			/* This instruction requires 3 extra cycles. */
			z80->state->cycles += 3;

			result_value = MemoryRead16Bit(z80, callbacks, z80->state->stack_pointer);
			MemoryWrite16Bit(z80, callbacks, z80->state->stack_pointer, destination_value);
			break;

		case Z80_OPCODE_EX_DE_HL:
			SWAP(z80->state->d, z80->state->h);
			SWAP(z80->state->e, z80->state->l);
			break;

		case Z80_OPCODE_DI:
			z80->state->interrupts_enabled = cc_false;
			break;

		case Z80_OPCODE_EI:
			z80->state->interrupts_enabled = cc_true;
			break;

		case Z80_OPCODE_PUSH:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, source_value >> 8);

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, source_value & 0xFF);
			break;

		case Z80_OPCODE_CALL_CONDITIONAL:
			if (!EvaluateCondition(z80->state->f, instruction->metadata->condition))
				break;
			/* Fallthrough */
		case Z80_OPCODE_CALL_UNCONDITIONAL:
			/* This instruction takes an extra cycle. */
			z80->state->cycles += 1;

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, z80->state->program_counter >> 8);

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, z80->state->program_counter & 0xFF);

			z80->state->program_counter = instruction->literal;
			break;

		case Z80_OPCODE_RST:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, z80->state->program_counter >> 8);

			--z80->state->stack_pointer;
			z80->state->stack_pointer &= 0xFFFF;
			MemoryWrite(z80, callbacks, z80->state->stack_pointer, z80->state->program_counter & 0xFF);

			z80->state->program_counter = instruction->metadata->embedded_literal;
			break;

		case Z80_OPCODE_RLC:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			result_value = (destination_value << 1) & 0xFF;
			result_value |= carry ? 0x01 : 0;

			z80->state->f &= ~(FLAG_MASK_CARRY | FLAG_MASK_HALF_CARRY | FLAG_MASK_ADD_SUBTRACT);
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_RRC:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			result_value = destination_value >> 1;
			result_value |= carry ? 0x80 : 0;

			z80->state->f &= ~(FLAG_MASK_CARRY | FLAG_MASK_HALF_CARRY | FLAG_MASK_ADD_SUBTRACT);
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_RL:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			result_value = (destination_value << 1) & 0xFF;
			result_value |= (z80->state->f &= FLAG_MASK_CARRY) != 0 ? 0x01 : 0;

			z80->state->f &= ~(FLAG_MASK_CARRY | FLAG_MASK_HALF_CARRY | FLAG_MASK_ADD_SUBTRACT);
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_RR:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			result_value = destination_value >> 1;
			result_value |= (z80->state->f &= FLAG_MASK_CARRY) != 0 ? 0x80 : 0;

			z80->state->f &= ~(FLAG_MASK_CARRY | FLAG_MASK_HALF_CARRY | FLAG_MASK_ADD_SUBTRACT);
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_SLA:
		case Z80_OPCODE_SLL:
		{
			const cc_bool carry = (destination_value & 0x80) != 0;

			result_value = (destination_value << 1) & 0xFF;

			z80->state->f = 0;
			z80->state->f |= (result_value & 0x80) != 0 ? FLAG_MASK_SIGN : 0;
			z80->state->f |= result_value == 0 ? FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_SRA:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			result_value = (destination_value >> 1) | (destination_value & 0x80);

			z80->state->f = 0;
			z80->state->f |= (result_value & 0x80) != 0 ? FLAG_MASK_SIGN : 0;
			z80->state->f |= result_value == 0 ? FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_SRL:
		{
			const cc_bool carry = (destination_value & 0x01) != 0;

			result_value = destination_value >> 1;

			z80->state->f = 0;
			z80->state->f |= (result_value & 0x80) != 0 ? FLAG_MASK_SIGN : 0;
			z80->state->f |= result_value == 0 ? FLAG_MASK_ZERO : 0;
			/* TODO: Parity. */
			z80->state->f |= carry ? FLAG_MASK_CARRY : 0;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;
		}

		case Z80_OPCODE_BIT:
			z80->state->f &= FLAG_MASK_CARRY;
			z80->state->f |= ((destination_value & instruction->metadata->embedded_literal) == 0) ? FLAG_MASK_ZERO : 0;
			z80->state->f |= FLAG_MASK_HALF_CARRY;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;

		case Z80_OPCODE_RES:
			result_value = destination_value & instruction->metadata->embedded_literal;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

			break;

		case Z80_OPCODE_SET:
			result_value = destination_value | instruction->metadata->embedded_literal;

			/* The memory-accessing version takes an extra cycle. */
			z80->state->cycles += instruction->metadata->operands[1] == Z80_OPERAND_HL_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IX_INDIRECT
			                   || instruction->metadata->operands[1] == Z80_OPERAND_IY_INDIRECT;

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
		{
			/* Needs to be 'unsigned long' so that it can hold the carry bit. */
			unsigned long result_value_long;

			source_value = ~source_value;
			result_value_long = (unsigned long)source_value + (unsigned long)destination_value + ((z80->state->f & FLAG_MASK_CARRY) != 0 ? 0 : 1);
			result_value = result_value_long & 0xFFFF;

			z80->state->f = 0;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY_16BIT;
			CONDITION_OVERFLOW_16BIT;
			CONDITION_CARRY_16BIT;

			z80->state->f |= FLAG_MASK_ADD_SUBTRACT;

			/* This instruction requires an extra 7 cycles. */
			z80->state->cycles += 7;

			break;
		}

		case Z80_OPCODE_ADC_HL:
		{
			/* Needs to be 'unsigned long' so that it can hold the carry bit. */
			const unsigned long result_value_long = (unsigned long)source_value + (unsigned long)destination_value + ((z80->state->f & FLAG_MASK_CARRY) != 0 ? 1 : 0);
			result_value = result_value_long & 0xFFFF;

			z80->state->f = 0;

			CONDITION_SIGN;
			CONDITION_ZERO;
			CONDITION_HALF_CARRY_16BIT;
			CONDITION_OVERFLOW_16BIT;
			CONDITION_CARRY_16BIT;

			/* This instruction requires an extra 7 cycles. */
			z80->state->cycles += 7;

			break;
		}

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
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			z80->state->i = z80->state->a;

			break;

		case Z80_OPCODE_LD_R_A:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			z80->state->r = z80->state->a;

			break;

		case Z80_OPCODE_LD_A_I:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			z80->state->a = z80->state->i;

			break;

		case Z80_OPCODE_LD_A_R:
			/* This instruction requires an extra cycle. */
			z80->state->cycles += 1;

			z80->state->a = z80->state->r;

			break;

		case Z80_OPCODE_RRD:
			/* This instruction requires an extra 4 cycles. */
			z80->state->cycles += 4;

			UNIMPLEMENTED_INSTRUCTION("RRD");
			break;

		case Z80_OPCODE_RLD:
			/* This instruction requires an extra 4 cycles. */
			z80->state->cycles += 4;

			UNIMPLEMENTED_INSTRUCTION("RLD");
			break;

		case Z80_OPCODE_LDI:
		{
			const unsigned int de = ((unsigned int)z80->state->d << 8) | z80->state->e;
			const unsigned int hl = ((unsigned int)z80->state->h << 8) | z80->state->l;

			MemoryWrite(z80, callbacks, de, MemoryRead(z80, callbacks, hl));

			/* Increment 'hl'. */
			++z80->state->l;
			z80->state->l &= 0xFF;

			if (z80->state->l == 0)
			{
				++z80->state->h;
				z80->state->h &= 0xFF;
			}

			/* Increment 'de'. */
			++z80->state->e;
			z80->state->e &= 0xFF;

			if (z80->state->e == 0)
			{
				++z80->state->d;
				z80->state->d &= 0xFF;
			}

			/* Decrement 'bc'. */
			--z80->state->c;
			z80->state->c &= 0xFF;

			if (z80->state->c == 0xFF)
			{
				--z80->state->b;
				z80->state->b &= 0xFF;
			}

			z80->state->f &= FLAG_MASK_CARRY | FLAG_MASK_ZERO | FLAG_MASK_SIGN;
			z80->state->f |= (z80->state->b | z80->state->c) != 0 ? FLAG_MASK_PARITY_OVERFLOW : 0;

			/* This instruction requires an extra 2 cycles. */
			z80->state->cycles += 2;

			break;
		}

		case Z80_OPCODE_LDD:
		{
			const unsigned int de = ((unsigned int)z80->state->d << 8) | z80->state->e;
			const unsigned int hl = ((unsigned int)z80->state->h << 8) | z80->state->l;

			MemoryWrite(z80, callbacks, de, MemoryRead(z80, callbacks, hl));

			/* Decrement 'hl'. */
			--z80->state->l;
			z80->state->l &= 0xFF;

			if (z80->state->l == 0)
			{
				--z80->state->h;
				z80->state->h &= 0xFF;
			}

			/* Decrement 'de'. */
			--z80->state->e;
			z80->state->e &= 0xFF;

			if (z80->state->e == 0)
			{
				--z80->state->d;
				z80->state->d &= 0xFF;
			}

			/* Decrement 'bc'. */
			--z80->state->c;
			z80->state->c &= 0xFF;

			if (z80->state->c == 0xFF)
			{
				--z80->state->b;
				z80->state->b &= 0xFF;
			}

			z80->state->f &= FLAG_MASK_CARRY | FLAG_MASK_ZERO | FLAG_MASK_SIGN;
			z80->state->f |= (z80->state->b | z80->state->c) != 0 ? FLAG_MASK_PARITY_OVERFLOW : 0;

			/* This instruction requires an extra 2 cycles. */
			z80->state->cycles += 2;

			break;
		}

		case Z80_OPCODE_LDIR:
		{
			unsigned int bc;
			unsigned int de;
			unsigned int hl;

			bc = ((unsigned int)z80->state->b << 8) | z80->state->c;
			de = ((unsigned int)z80->state->d << 8) | z80->state->e;
			hl = ((unsigned int)z80->state->h << 8) | z80->state->l;

			for (;;)
			{
				MemoryWrite(z80, callbacks, de, MemoryRead(z80, callbacks, hl));

				++hl;
				hl &= 0xFFFF;

				++de;
				de &= 0xFFFF;

				--bc;
				bc &= 0xFFFF;

				/* An extra 2 cycles are needed here. */
				z80->state->cycles += 2;

				if (bc == 0)
					break;

				/* An extra 5 cycles are needed here. */
				z80->state->cycles += 5;
			}

			z80->state->b = 0;
			z80->state->c = 0;
			z80->state->d = de >> 8;
			z80->state->e = de & 0xFF;
			z80->state->h = hl >> 8;
			z80->state->l = hl & 0xFF;

			z80->state->f &= FLAG_MASK_CARRY | FLAG_MASK_ZERO | FLAG_MASK_SIGN;

			break;
		}

		case Z80_OPCODE_LDDR:
		{
			unsigned int bc;
			unsigned int de;
			unsigned int hl;

			bc = ((unsigned int)z80->state->b << 8) | z80->state->c;
			de = ((unsigned int)z80->state->d << 8) | z80->state->e;
			hl = ((unsigned int)z80->state->h << 8) | z80->state->l;

			for (;;)
			{
				MemoryWrite(z80, callbacks, de, MemoryRead(z80, callbacks, hl));

				--hl;
				hl &= 0xFFFF;

				--de;
				de &= 0xFFFF;

				--bc;
				bc &= 0xFFFF;

				/* An extra 2 cycles are needed here. */
				z80->state->cycles += 2;

				if (bc == 0)
					break;

				/* An extra 5 cycles are needed here. */
				z80->state->cycles += 5;
			}

			z80->state->b = 0;
			z80->state->c = 0;
			z80->state->d = de >> 8;
			z80->state->e = de & 0xFF;
			z80->state->h = hl >> 8;
			z80->state->l = hl & 0xFF;

			z80->state->f &= FLAG_MASK_CARRY | FLAG_MASK_ZERO | FLAG_MASK_SIGN;

			break;
		}

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

	if (instruction->metadata->write_destination)
	{
		WriteOperand(z80, instruction, instruction->metadata->operands[1], result_value, callbacks);

		/* Handle double-prefix instructions. */
		/* Don't do a redundant write in double-prefix mode when operating on '(HL)'. */
		if (instruction->double_prefix_mode && instruction->metadata->operands[1] != Z80_OPERAND_HL_INDIRECT)
			WriteOperand(z80, instruction, z80->state->register_mode == Z80_REGISTER_MODE_IX ? Z80_OPERAND_IX_INDIRECT : Z80_OPERAND_IY_INDIRECT, result_value, callbacks);
	}
}

void Z80_Constant_Initialise(Z80_Constant *constant)
{
	unsigned int i;

	/* Pre-compute instruction metadata, to speed up opcode decoding. */
	for (i = 0; i < 0x100; ++i)
	{
		DecodeInstructionMetadata(&constant->instruction_metadata_lookup_normal[Z80_REGISTER_MODE_HL][i], INSTRUCTION_MODE_NORMAL, Z80_REGISTER_MODE_HL, i);
		DecodeInstructionMetadata(&constant->instruction_metadata_lookup_normal[Z80_REGISTER_MODE_IX][i], INSTRUCTION_MODE_NORMAL, Z80_REGISTER_MODE_IX, i);
		DecodeInstructionMetadata(&constant->instruction_metadata_lookup_normal[Z80_REGISTER_MODE_IY][i], INSTRUCTION_MODE_NORMAL, Z80_REGISTER_MODE_IY, i);

		/* 'Z80_REGISTER_MODE_HL' is used here so that the destination operand write only has to check for HL instead of IX and IY. */
		DecodeInstructionMetadata(&constant->instruction_metadata_lookup_bits[i], INSTRUCTION_MODE_BITS, Z80_REGISTER_MODE_HL, i);

		DecodeInstructionMetadata(&constant->instruction_metadata_lookup_misc[i], INSTRUCTION_MODE_MISC, Z80_REGISTER_MODE_HL, i);
	}

	for (i = 0; i < 0x100; ++i)
	{
		/* http://graphics.stanford.edu/~seander/bithacks.html#ParityMultiply */
		/* I have absolutely no idea how this works. */
		unsigned int v;
		v = i;
		v ^= v >> 1;
		v ^= v >> 2;
		v = (v & 0x11) * 0x11;
		constant->parity_lookup[i] = (v & 0x10) == 0 ? FLAG_MASK_PARITY_OVERFLOW : 0;
	}
}

void Z80_State_Initialise(Z80_State *state)
{
	/* A disgusting hack. */
	const Z80 z80 = {NULL, state};

	Z80_Reset(&z80);

	/* Update on the next cycle. */
	state->cycles = 1;
}

void Z80_Reset(const Z80 *z80)
{
	/* Revert back to a prefix-free mode. */
	z80->state->register_mode = Z80_REGISTER_MODE_HL;

	z80->state->program_counter = 0;

	/* The official Z80 manual claims that this happens ('z80cpu_um.pdf' page 18). */
	z80->state->interrupts_enabled = cc_false;
}

void Z80_Interrupt(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks)
{
	/* TODO: The other interrupt modes. */
	/* TODO: Interrupts should not be able to occur directly after a prefix instruction. */
	if (z80->state->interrupts_enabled)
	{
		z80->state->interrupts_enabled = cc_false;

		/* TODO: Interrupt duration. */
		--z80->state->stack_pointer;
		z80->state->stack_pointer &= 0xFFFF;
		callbacks->write(callbacks->user_data, z80->state->stack_pointer, z80->state->program_counter >> 8);

		--z80->state->stack_pointer;
		z80->state->stack_pointer &= 0xFFFF;
		callbacks->write(callbacks->user_data, z80->state->stack_pointer, z80->state->program_counter & 0xFF);

		z80->state->program_counter = 0x38;
	}
}

void Z80_DoCycle(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks)
{
	if (--z80->state->cycles == 0)
	{
		Instruction instruction;

		DecodeInstruction(z80, &instruction, callbacks);

		ExecuteInstruction(z80, &instruction, callbacks);
	}
}
