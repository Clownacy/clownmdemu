#include "z80.h"

#include "clowncommon.h"

#define SIGN_EXTEND(value, bitmask) (((value) & ((bitmask) >> 1u)) - ((value) & (((bitmask) >> 1u) + 1u)))

typedef struct DecodeInstructionCallbackData
{
	Z80_State *state;
	const Z80_ReadCallback *read_callback;
} DecodeInstructionCallbackData;

static unsigned int DecodeInstructionCallback(void *user_data)
{
	const DecodeInstructionCallbackData* const callback_data = user_data;

	return callback_data->read_callback->callback(callback_data->read_callback->user_data, callback_data->state->program_counter++);
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
			value = state->h;
			break;

		case Z80_OPERAND_L:
			value = state->l;
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
			value = (state->h << 8) | state->l;
			break;

		case Z80_OPERAND_PC:
			value = state->program_counter;
			break;

		case Z80_OPERAND_SP:
			value = state->stack_pointer;
			break;

		case Z80_OPERAND_BC_INDIRECT:
		case Z80_OPERAND_DE_INDIRECT:
		case Z80_OPERAND_HL_INDIRECT:
		case Z80_OPERAND_ADDRESS:
		{
			unsigned int address;

			switch (instruction->metadata.opcode)
			{
				default:
					/* Should never occur. */
					address = 0;
					break;

				case Z80_OPERAND_BC_INDIRECT:
					address = (state->b << 8) | state->c;
					break;

				case Z80_OPERAND_DE_INDIRECT:
					address = (state->h << 8) | state->e;
					break;

				case Z80_OPERAND_HL_INDIRECT:
					address = (state->h << 8) | state->l;
					break;

				case Z80_OPERAND_ADDRESS:
					address = instruction->literal;
					break;
			}

			value = read_callback->callback(read_callback->user_data, address + 0);
			value |= read_callback->callback(read_callback->user_data, address + 1) << 8;

			break;
		}
	}

	return value;
}

static unsigned int WriteOperand(Z80_State *state, const Z80_Instruction *instruction, unsigned int operand, unsigned int value, const Z80_WriteCallback *write_callback)
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
			state->h = value;
			break;

		case Z80_OPERAND_L:
			state->l = value;
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
			state->h = value >> 8;
			state->l = value & 0xFF;
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

			switch (instruction->metadata.opcode)
			{
				default:
					/* Should never occur. */
					address = 0;
					break;

				case Z80_OPERAND_BC_INDIRECT:
					address = (state->b << 8) | state->c;
					break;

				case Z80_OPERAND_DE_INDIRECT:
					address = (state->h << 8) | state->e;
					break;

				case Z80_OPERAND_HL_INDIRECT:
					address = (state->h << 8) | state->l;
					break;

				case Z80_OPERAND_ADDRESS:
					address = instruction->literal;
					break;
			}

			write_callback->callback(write_callback->user_data, address + 0, value & 0xFF);
			write_callback->callback(write_callback->user_data, address + 1, value >> 8);

			break;
		}
	}

	return value;
}

void Z80_State_Initialise(Z80_State *state)
{
	state->register_mode = Z80_REGISTER_MODE_HL;
	state->instruction_mode = Z80_INSTRUCTION_MODE_NORMAL;
}

void Z80_Reset(Z80_State *state)
{
	state->program_counter = 0;
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
							static const Z80_Operand operands[4] = {Z80_OPERAND_BC, Z80_OPERAND_DE, Z80_OPERAND_ADDRESS, Z80_OPERAND_ADDRESS};

							const Z80_Operand operand_a = p == 2 ? Z80_OPERAND_HL_INDIRECT : Z80_OPERAND_A;
							const Z80_Operand operand_b = operands[p];

							metadata->opcode = p == 2 ? Z80_OPCODE_LD_16BIT : Z80_OPCODE_LD_8BIT;
							metadata->write_destination = cc_true;

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
										break;

									case 3:
										metadata->opcode = Z80_OPCODE_LD_SP_HL;
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
					metadata->write_destination = cc_true;
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

void Z80_DecodeInstruction(Z80_Instruction *instruction, Z80_InstructionMode instruction_mode, const Z80_ReadInstructionCallback *read_callback)
{
	Z80_DecodeInstructionMetadata(&instruction->metadata, read_callback->callback(read_callback->user_data), instruction_mode);

	if (instruction->metadata.has_displacement)
		instruction->displacement = read_callback->callback(read_callback->user_data);

	switch (instruction->metadata.operands[0])
	{
		default:
			break;

		case Z80_OPERAND_LITERAL_8BIT:
			instruction->literal = read_callback->callback(read_callback->user_data);
			break;

		case Z80_OPERAND_LITERAL_16BIT:
			instruction->literal = read_callback->callback(read_callback->user_data);
			instruction->literal |= read_callback->callback(read_callback->user_data) << 8;
			break;
	}
}

void Z80_ExecuteInstruction(Z80_State *state, const Z80_Instruction *instruction, const Z80_ReadAndWriteCallbacks *callbacks)
{
	const unsigned int source_value = ReadOperand(state, instruction, 0, &callbacks->read);

	unsigned int destination_value;

	if (instruction->metadata.read_destination)
		destination_value = ReadOperand(state, instruction, 1, &callbacks->read);

	switch (instruction->metadata.opcode)
	{

	}

	if (instruction->metadata.write_destination)
		WriteOperand(state, instruction, 1, destination_value, &callbacks->write);
}

void Z80_DoCycle(Z80_State *state, const Z80_ReadAndWriteCallbacks *callbacks)
{
	const DecodeInstructionCallbackData callback_data = {state, &callbacks->read};
	const Z80_ReadInstructionCallback read_instruction_callback = {DecodeInstructionCallback, (void*)&callback_data};

	Z80_Instruction instruction;

	Z80_DecodeInstruction(&instruction, state->instruction_mode, &read_instruction_callback);

	Z80_ExecuteInstruction(state, &instruction, callbacks);
}
