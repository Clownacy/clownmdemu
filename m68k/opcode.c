#include "opcode.h"

static Instruction GetInstruction(const Opcode *opcode)
{
	Instruction instruction;

	instruction = INSTRUCTION_ILLEGAL;

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
			if (opcode->secondary_register != 0)
				instruction = (opcode->raw & 0x00FF) == 0 ? INSTRUCTION_BCC_WORD : INSTRUCTION_BCC_SHORT;
			else if (opcode->bit_8)
				instruction = (opcode->raw & 0x00FF) == 0 ? INSTRUCTION_BSR_WORD : INSTRUCTION_BSR_SHORT;
			else
				instruction = (opcode->raw & 0x00FF) == 0 ? INSTRUCTION_BRA_WORD : INSTRUCTION_BRA_SHORT;

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

static unsigned int GetSize(const Instruction instruction, const Opcode* const opcode)
{
	unsigned int operation_size;

	operation_size = 0;

	switch (instruction)
	{
		case INSTRUCTION_ORI_TO_CCR:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_EORI_TO_CCR:
		case INSTRUCTION_NBCD:
		case INSTRUCTION_TAS:
		case INSTRUCTION_SCC:
		case INSTRUCTION_SBCD:
		case INSTRUCTION_ABCD:
			/* Hardcoded to a byte. */
			operation_size = 1;
			break;

		case INSTRUCTION_ORI_TO_SR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_EORI_TO_SR:
		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_MOVE_TO_SR:
		case INSTRUCTION_MOVE_TO_CCR:
		case INSTRUCTION_LINK:
		case INSTRUCTION_MOVEM:
		case INSTRUCTION_CHK:
		case INSTRUCTION_DBCC:
		case INSTRUCTION_DIVU:
		case INSTRUCTION_DIVS:
		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_LSD_MEMORY:
		case INSTRUCTION_ROXD_MEMORY:
		case INSTRUCTION_ROD_MEMORY:
		case INSTRUCTION_STOP:
			/* Hardcoded to a word. */
			operation_size = 2;
			break;

		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_LEA:
		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_MULU:
		case INSTRUCTION_MULS:
			/* Hardcoded to a longword. */
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
			/* 4 if register - 1 if memory. */
			operation_size = opcode->primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;
			break;

		case INSTRUCTION_MOVEA:
		case INSTRUCTION_MOVE:
			/* Derived from an odd bitfield. */
			switch (opcode->raw & 0x3000)
			{
				case 0x1000:
					operation_size = 1;
					break;
			
				case 0x2000:
					operation_size = 4; /* Yup, this isn't a typo. */
					break;
			
				case 0x3000:
					operation_size = 2;
					break;
			}

			break;

		case INSTRUCTION_EXT:
			operation_size = opcode->raw & 0x0040 ? 4 : 2;
			break;

		case INSTRUCTION_SUBA:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_ADDA:
			/* Word or longword based on bit 8. */
			operation_size = opcode->bit_8 ? 4 : 2;
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
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_ADDX:
		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_LSD_REGISTER:
		case INSTRUCTION_ROXD_REGISTER:
		case INSTRUCTION_ROD_REGISTER:
			/* Standard. */
			operation_size = 1 << opcode->bits_6_and_7;
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
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_EXG:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* Doesn't have a size. */
			break;
	}

	return operation_size;
}

#define SET_OPERAND(OPERATION_SIZE, ADDRESS_MODE, ADDRESS_MODE_REGISTER)\
do\
{\
	OPERAND.operation_size_in_bytes = OPERATION_SIZE;\
	OPERAND.address_mode = ADDRESS_MODE;\
	OPERAND.address_mode_register = ADDRESS_MODE_REGISTER;\
} while(0)

#define OPERAND decoded_opcode->operands[0]

static cc_bool GetSourceOperand(DecodedOpcode* const decoded_opcode, const Opcode* const opcode)
{
	/* Obtain source value. */
	switch (decoded_opcode->instruction)
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
			/* Immediate value (any size). */
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
			break;

		case INSTRUCTION_BTST_DYNAMIC:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_EOR:
			/* Secondary data register. */
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_DATA_REGISTER, opcode->secondary_register);
			break;

		case INSTRUCTION_BTST_STATIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_STATIC:
			/* Immediate value (byte). */
			SET_OPERAND(1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
			break;

		case INSTRUCTION_PEA:
		case INSTRUCTION_JSR:
		case INSTRUCTION_JMP:
		case INSTRUCTION_LEA:
			/* Memory address */
			SET_OPERAND(0, opcode->primary_address_mode, opcode->primary_register); /* 0 is a special value that means to obtain the address rather than the data at that address. */
			break;

		case INSTRUCTION_MOVE_FROM_SR:
			OPERAND.address_mode = ADDRESS_MODE_STATUS_REGISTER;
			break;

		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_TRAP:
			/* Doesn't need an address mode for its source. */
			break;

		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_WORD:
			/* Immediate value (word). */
			SET_OPERAND(2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
			break;

		case INSTRUCTION_SBCD:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_ADDX:
			SET_OPERAND(decoded_opcode->size, opcode->raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode->primary_register);
			break;

		case INSTRUCTION_OR:
		case INSTRUCTION_SUB:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
			/* Primary address mode or secondary data register, based on direction bit. */
			if (opcode->bit_8)
				SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_DATA_REGISTER, opcode->secondary_register);
			else
				SET_OPERAND(decoded_opcode->size, opcode->primary_address_mode, opcode->primary_register);

			break;

		case INSTRUCTION_CMPM:
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode->primary_register);
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
		case INSTRUCTION_ADDA:
		case INSTRUCTION_TST:
			/* Primary address mode. */
			SET_OPERAND(decoded_opcode->size, opcode->primary_address_mode, opcode->primary_register);
			break;

		case INSTRUCTION_MULU:
		case INSTRUCTION_MULS:
			/* Primary address mode, hardcoded to word-size. */
			SET_OPERAND(2, opcode->primary_address_mode, opcode->primary_register);
			break;

		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BCC_SHORT:
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
		case INSTRUCTION_UNLK:
		case INSTRUCTION_MOVE_USP:
		case INSTRUCTION_RESET:
		case INSTRUCTION_NOP:
		case INSTRUCTION_RTE:
		case INSTRUCTION_RTS:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_RTR:
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
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* Doesn't have a source address mode to decode. */
			return cc_false;
	}

	return cc_true;
}

#undef OPERAND
#define OPERAND decoded_opcode->operands[1]

static cc_bool GetDestinationOperand(DecodedOpcode* const decoded_opcode, const Opcode* const opcode)
{
	/* Decode destination address mode */
	switch (decoded_opcode->instruction)
	{
		case INSTRUCTION_EXT:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_LSD_REGISTER:
		case INSTRUCTION_ROXD_REGISTER:
		case INSTRUCTION_ROD_REGISTER:
			/* Data register (primary) */
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_DATA_REGISTER, opcode->primary_register);
			break;

		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_CMP:
		case INSTRUCTION_MULU:
		case INSTRUCTION_MULS:
			/* Data register (secondary) */
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_DATA_REGISTER, opcode->secondary_register);
			break;

		case INSTRUCTION_LEA:
			/* Address register (secondary) */
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_ADDRESS_REGISTER, opcode->secondary_register);
			break;

		case INSTRUCTION_MOVE:
			/* Secondary address mode */
			SET_OPERAND(decoded_opcode->size, opcode->secondary_address_mode, opcode->secondary_register);
			break;

		case INSTRUCTION_SBCD:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_ADDX:
			SET_OPERAND(decoded_opcode->size, opcode->raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode->secondary_register);
			break;

		case INSTRUCTION_OR:
		case INSTRUCTION_SUB:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
			/* Primary address mode or secondary data register, based on direction bit */
			if (opcode->bit_8)
				SET_OPERAND(decoded_opcode->size, opcode->primary_address_mode, opcode->primary_register);
			else
				SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_DATA_REGISTER, opcode->secondary_register);

			break;

		case INSTRUCTION_SUBA:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_ADDA:
		case INSTRUCTION_MOVEA:
			/* Full secondary address register */
			SET_OPERAND(4, ADDRESS_MODE_ADDRESS_REGISTER, opcode->secondary_register);
			break;

		case INSTRUCTION_CMPM:
			SET_OPERAND(decoded_opcode->size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode->secondary_register);
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
		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_CLR:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_SCC:
		case INSTRUCTION_EOR:
		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_LSD_MEMORY:
		case INSTRUCTION_ROXD_MEMORY:
		case INSTRUCTION_ROD_MEMORY:
			/* Using primary address mode */
			SET_OPERAND(decoded_opcode->size, opcode->primary_address_mode, opcode->primary_register);
			break;

		case INSTRUCTION_ORI_TO_CCR:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_EORI_TO_CCR:
		case INSTRUCTION_MOVE_TO_CCR:
			OPERAND.address_mode = ADDRESS_MODE_CONDITION_CODE_REGISTER;
			break;

		case INSTRUCTION_ORI_TO_SR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_EORI_TO_SR:
		case INSTRUCTION_MOVE_TO_SR:
			OPERAND.address_mode = ADDRESS_MODE_STATUS_REGISTER;
			break;

		case INSTRUCTION_MOVEM:
			/* Memory address */
			SET_OPERAND(0, opcode->primary_address_mode, opcode->primary_register); /* 0 is a special value that means to obtain the address rather than the data at that address. */
			break;

		case INSTRUCTION_MOVEP:
			/* Memory address */
			SET_OPERAND(0, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT, opcode->primary_register); /* 0 is a special value that means to obtain the address rather than the data at that address. */
			break;

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
		case INSTRUCTION_CHK:
		case INSTRUCTION_DBCC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_DIVU:
		case INSTRUCTION_DIVS:
		case INSTRUCTION_EXG:
		case INSTRUCTION_TST:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* Doesn't have a destination address mode to decode. */
			return cc_false;
	}

	return cc_true;
}

#undef OPERAND
#undef SET_OPERAND

void DecodeOpcode(DecodedOpcode* const decoded_opcode, const Opcode* const opcode)
{
	decoded_opcode->instruction = GetInstruction(opcode);
	decoded_opcode->size = GetSize(decoded_opcode->instruction, opcode);
	GetSourceOperand(decoded_opcode, opcode);
	GetDestinationOperand(decoded_opcode, opcode);
}
