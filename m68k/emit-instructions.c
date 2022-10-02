#include "emit-instructions.h"

#include <stdio.h>

#include "emit.h"
#include "instructions.h"

void EmitInstructionSize(const Instruction instruction)
{
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
			Emit("/* Hardcoded to a byte. */");
			Emit("operation_size = 1;");
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
		case INSTRUCTION_MULU:
		case INSTRUCTION_MULS:
		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_LSD_MEMORY:
		case INSTRUCTION_ROXD_MEMORY:
		case INSTRUCTION_ROD_MEMORY:
		case INSTRUCTION_STOP:
			Emit("/* Hardcoded to a word. */");
			Emit("operation_size = 2;");
			break;

		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_LEA:
		case INSTRUCTION_MOVEQ:
			Emit("/* Hardcoded to a longword. */");
			Emit("operation_size = 4;");
			break;

		case INSTRUCTION_BTST_STATIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BTST_DYNAMIC:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BSET_DYNAMIC:
			Emit("/* 4 if register - 1 if memory. */");
			Emit("operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;");
			break;

		case INSTRUCTION_MOVEA:
		case INSTRUCTION_MOVE:
			Emit("/* Derived from an odd bitfield. */");
			Emit("switch (opcode.raw & 0x3000)");
			Emit("{");
			Emit("	case 0x1000:");
			Emit("		operation_size = 1;");
			Emit("		break;");
			Emit("");
			Emit("	case 0x2000:");
			Emit("		operation_size = 4; /* Yup, this isn't a typo. */");
			Emit("		break;");
			Emit("");
			Emit("	case 0x3000:");
			Emit("		operation_size = 2;");
			Emit("		break;");
			Emit("}");

			break;

		case INSTRUCTION_EXT:
			Emit("operation_size = opcode.raw & 0x0040 ? 4 : 2;");
			break;

		case INSTRUCTION_SUBA:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_ADDA:
			Emit("/* Word or longword based on bit 8. */");
			Emit("operation_size = opcode.bit_8 ? 4 : 2;");
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
			Emit("/* Standard. */");
			Emit("operation_size = 1 << opcode.bits_6_and_7;");
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
			Emit("/* Doesn't have a size. */");
			break;
	}
}

void EmitInstructionSourceAddressMode(const Instruction instruction)
{
	/* Obtain source value. */
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
			Emit("/* Immediate value (any size). */");
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);");
			break;

		case INSTRUCTION_BTST_DYNAMIC:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_EOR:
			Emit("/* Secondary data register. */");
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);");
			break;

		case INSTRUCTION_BTST_STATIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_STATIC:
			Emit("/* Immediate value (byte). */");
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);");
			break;

		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_PEA:
		case INSTRUCTION_JSR:
		case INSTRUCTION_JMP:
		case INSTRUCTION_LEA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_TRAP:
			Emit("/* Doesn't need an address mode for its source. */");
			break;

		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_WORD:
			Emit("/* Immediate value (word). */");
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);");
			break;

		case INSTRUCTION_SBCD:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_ADDX:
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);");
			break;

		case INSTRUCTION_OR:
		case INSTRUCTION_SUB:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
			Emit("/* Primary address mode or secondary data register, based on direction bit. */");
			Emit("if (opcode.bit_8)");
			Emit("	DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);");
			Emit("else");
			Emit("	DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);");

			break;

		case INSTRUCTION_CMPM:
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode.primary_register);");
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
		case INSTRUCTION_TST:
			Emit("/* Primary address mode. */");
			Emit("DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);");
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
			Emit("/* Doesn't have a source address mode to decode. */");
			break;
	}
}

void EmitInstructionDestinationAddressMode(const Instruction instruction)
{
	/* Decode destination address mode */
	switch (instruction)
	{
		case INSTRUCTION_EXT:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_LSD_REGISTER:
		case INSTRUCTION_ROXD_REGISTER:
		case INSTRUCTION_ROD_REGISTER:
			Emit("/* Data register (primary) */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);");
			break;

		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_CMP:
			Emit("/* Data register (secondary) */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);");
			break;

		case INSTRUCTION_LEA:
			Emit("/* Address register (secondary) */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);");
			break;

		case INSTRUCTION_MOVE:
			Emit("/* Secondary address mode */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode.secondary_address_mode, opcode.secondary_register);");
			break;

		case INSTRUCTION_SBCD:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_ADDX:
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);");
			break;

		case INSTRUCTION_OR:
		case INSTRUCTION_SUB:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
			Emit("/* Primary address mode or secondary data register, based on direction bit */");
			Emit("if (opcode.bit_8)");
			Emit("	DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);");
			Emit("else");
			Emit("	DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);");

			break;

		case INSTRUCTION_SUBA:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_ADDA:
		case INSTRUCTION_MOVEA:
			Emit("/* Full secondary address register */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);");
			break;

		case INSTRUCTION_CMPM:
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode.secondary_register);");
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
			Emit("/* Using primary address mode */");
			Emit("DecodeAddressMode(state, callbacks, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);");
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
		case INSTRUCTION_MOVEP:
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
		case INSTRUCTION_MULU:
		case INSTRUCTION_MULS:
		case INSTRUCTION_EXG:
		case INSTRUCTION_TST:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			Emit("/* Doesn't have a destination address mode to decode. */");
			break;
	}
}

void EmitInstructionReadSourceOperand(const Instruction instruction)
{
	/* Obtain source value. */
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
		case INSTRUCTION_BTST_DYNAMIC:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_EOR:
		case INSTRUCTION_BTST_STATIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_SBCD:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_ADDX:
		case INSTRUCTION_OR:
		case INSTRUCTION_SUB:
		case INSTRUCTION_AND:
		case INSTRUCTION_ADD:
		case INSTRUCTION_CMPM:
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
		case INSTRUCTION_TST:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_WORD:
			/* Read source from decoded address mode */
			Emit("source_value = GetValueUsingDecodedAddressMode(state, callbacks, &source_decoded_address_mode);");
			break;

		case INSTRUCTION_MOVE_FROM_SR:
			/* Status register. */
			Emit("source_value = state->status_register;");
			break;

		case INSTRUCTION_PEA:
		case INSTRUCTION_JSR:
		case INSTRUCTION_JMP:
		case INSTRUCTION_LEA:
			/* Effective address. */
			Emit("source_value = DecodeMemoryAddressMode(state, callbacks, 0, opcode.primary_address_mode, opcode.primary_register);");
			break;

		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
			Emit("source_value = ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8. */");
			break;

		case INSTRUCTION_TRAP:
			Emit("source_value = opcode.raw & 0xF;");
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
			Emit("/* Doesn't have a source value. */");
			break;
	}
}

void EmitInstructionReadDestinationOperand(const Instruction instruction)
{
	/* Obtain destination value */
	switch (instruction)
	{
		case INSTRUCTION_ORI_TO_CCR:
		case INSTRUCTION_ORI_TO_SR:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_EORI_TO_CCR:
		case INSTRUCTION_EORI_TO_SR:
			Emit("/* Read from status register */");
			Emit("destination_value = state->status_register;");
			break;

		case INSTRUCTION_CLR:
		case INSTRUCTION_SCC:
			Emit("/* For some reason, this instruction reads from its destination even though it doesn't use it. */");
			/* Fallthrough */
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
		case INSTRUCTION_EXT:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_SUBAQ:
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
			Emit("destination_value = GetValueUsingDecodedAddressMode(state, callbacks, &destination_decoded_address_mode);");
			break;

		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_CHK:
		case INSTRUCTION_DBCC:
		case INSTRUCTION_DIVS:
		case INSTRUCTION_DIVU:
		case INSTRUCTION_EXG:
		case INSTRUCTION_ILLEGAL:
		case INSTRUCTION_JMP:
		case INSTRUCTION_JSR:
		case INSTRUCTION_LEA:
		case INSTRUCTION_LINK:
		case INSTRUCTION_MOVE:
		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_MOVE_TO_CCR:
		case INSTRUCTION_MOVE_TO_SR:
		case INSTRUCTION_MOVE_USP:
		case INSTRUCTION_MOVEA:
		case INSTRUCTION_MOVEM:
		case INSTRUCTION_MOVEP:
		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_MULS:
		case INSTRUCTION_MULU:
		case INSTRUCTION_NOP:
		case INSTRUCTION_PEA:
		case INSTRUCTION_RESET:
		case INSTRUCTION_RTE:
		case INSTRUCTION_RTR:
		case INSTRUCTION_RTS:
		case INSTRUCTION_STOP:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_TST:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			Emit("/* Doesn't read its destination (if it even has one) */");
			break;
	}
}

void EmitInstructionAction(const Instruction instruction)
{
	/* Do the actual instruction */
	switch (instruction)
	{
		case INSTRUCTION_OR:
		case INSTRUCTION_ORI:
		case INSTRUCTION_ORI_TO_CCR:
		case INSTRUCTION_ORI_TO_SR:
			Emit("result_value = destination_value | source_value;");
			break;

		case INSTRUCTION_AND:
		case INSTRUCTION_ANDI:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
			Emit("result_value = destination_value & source_value;");
			break;

		case INSTRUCTION_CMPA:
		case INSTRUCTION_SUBA:
			Emit("if (!opcode.bit_8)");
			Emit("	source_value = CC_SIGN_EXTEND_ULONG(15, source_value);");
			Emit("");
			/* Fallthrough */
		case INSTRUCTION_CMP:
		case INSTRUCTION_CMPI:
		case INSTRUCTION_CMPM:
		case INSTRUCTION_SUB:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_SUBI:
		case INSTRUCTION_SUBQ:
			Emit("result_value = destination_value - source_value;");
			break;

		case INSTRUCTION_ADDA:
			Emit("if (!opcode.bit_8)");
			Emit("	source_value = CC_SIGN_EXTEND_ULONG(15, source_value);");
			Emit("");
			/* Fallthrough */
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ADDI:
		case INSTRUCTION_ADDQ:
			Emit("result_value = destination_value + source_value;");
			break;

		case INSTRUCTION_EOR:
		case INSTRUCTION_EORI:
		case INSTRUCTION_EORI_TO_CCR:
		case INSTRUCTION_EORI_TO_SR:
			Emit("result_value = destination_value ^ source_value;");
			break;

		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BTST_DYNAMIC:
		case INSTRUCTION_BTST_STATIC:
			Emit("/* Modulo the source value */");
			Emit("source_value &= operation_size * 8 - 1;");
			Emit("");
			Emit("/* Set the zero flag to the specified bit */");
			Emit("state->status_register &= ~CONDITION_CODE_ZERO;");
			Emit("state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);");

			switch (instruction)
			{
				case INSTRUCTION_BTST_DYNAMIC:
				case INSTRUCTION_BTST_STATIC:
					break;

				case INSTRUCTION_BCHG_DYNAMIC:
				case INSTRUCTION_BCHG_STATIC:
					Emit("");
					Emit("result_value = destination_value ^ (1ul << source_value);");
					break;

				case INSTRUCTION_BCLR_DYNAMIC:
				case INSTRUCTION_BCLR_STATIC:
					Emit("");
					Emit("result_value = destination_value & ~(1ul << source_value);");
					break;

				case INSTRUCTION_BSET_DYNAMIC:
				case INSTRUCTION_BSET_STATIC:
					Emit("");
					Emit("result_value = destination_value | (1ul << source_value);");
					break;

				default:
					/* Shut up stupid compiler warnings */
					break;
			}

			break;


		case INSTRUCTION_MOVEP:
			Emit("{");
			Emit("unsigned long memory_address = DecodeMemoryAddressMode(state, callbacks, 0, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT, opcode.primary_register);");
			Emit("");
			Emit("switch (opcode.bits_6_and_7)");
			Emit("{");
			Emit("	case 0:");
			Emit("		/* Memory to register (word) */");
			Emit("		state->data_registers[opcode.secondary_register] &= ~0xFFFFul;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 0) << 8 * 1;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 1) << 8 * 0;");
			Emit("		break;");
			Emit("");
			Emit("	case 1:");
			Emit("		/* Memory to register (longword) */");
			Emit("		state->data_registers[opcode.secondary_register] = 0;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 0) << 8 * 3;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 1) << 8 * 2;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 2) << 8 * 1;");
			Emit("		state->data_registers[opcode.secondary_register] |= ReadByte(callbacks, memory_address + 2 * 3) << 8 * 0;");
			Emit("		break;");
			Emit("");
			Emit("	case 2:");
			Emit("		/* Register to memory (word) */");
			Emit("		WriteByte(callbacks, memory_address + 2 * 0, (state->data_registers[opcode.secondary_register] >> 8 * 1) & 0xFF);");
			Emit("		WriteByte(callbacks, memory_address + 2 * 1, (state->data_registers[opcode.secondary_register] >> 8 * 0) & 0xFF);");
			Emit("		break;");
			Emit("");
			Emit("	case 3:");
			Emit("		/* Register to memory (longword) */");
			Emit("		WriteByte(callbacks, memory_address + 2 * 0, (state->data_registers[opcode.secondary_register] >> 8 * 3) & 0xFF);");
			Emit("		WriteByte(callbacks, memory_address + 2 * 1, (state->data_registers[opcode.secondary_register] >> 8 * 2) & 0xFF);");
			Emit("		WriteByte(callbacks, memory_address + 2 * 2, (state->data_registers[opcode.secondary_register] >> 8 * 1) & 0xFF);");
			Emit("		WriteByte(callbacks, memory_address + 2 * 3, (state->data_registers[opcode.secondary_register] >> 8 * 0) & 0xFF);");
			Emit("		break;");
			Emit("}");
			Emit("}");

			break;

		case INSTRUCTION_MOVEA:
			Emit("result_value = operation_size == 2 ? CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;");
			break;

		case INSTRUCTION_LEA:
		case INSTRUCTION_MOVE:
		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_MOVE_TO_CCR:
		case INSTRUCTION_MOVE_TO_SR:
		case INSTRUCTION_TST:
			Emit("result_value = source_value;");
			break;

		case INSTRUCTION_LINK:
			Emit("/* Push address register to stack */");
			Emit("state->address_registers[7] -= 4;");
			Emit("WriteLongWord(state, callbacks, state->address_registers[7], state->address_registers[opcode.primary_register]);");
			Emit("");
			Emit("/* Copy stack pointer to address register */");
			Emit("state->address_registers[opcode.primary_register] = state->address_registers[7];");
			Emit("");
			Emit("/* Offset the stack pointer by the immediate value */");
			Emit("state->address_registers[7] += CC_SIGN_EXTEND_ULONG(15, source_value);");

			break;

		case INSTRUCTION_UNLK:
			Emit("state->address_registers[7] = state->address_registers[opcode.primary_register];");
			Emit("state->address_registers[opcode.primary_register] = ReadLongWord(state, callbacks, state->address_registers[7]);");
			Emit("state->address_registers[7] += 4;");
			break;

		case INSTRUCTION_NEGX:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"NEGX\");");
			break;

		case INSTRUCTION_CLR:
			Emit("result_value = 0;");
			break;

		case INSTRUCTION_NEG:
			Emit("result_value = 0 - destination_value;");
			break;

		case INSTRUCTION_NOT:
			Emit("result_value = ~destination_value;");
			break;

		case INSTRUCTION_EXT:
			Emit("result_value = CC_SIGN_EXTEND_ULONG((opcode.raw & 0x0040) != 0 ? 15 : 7, destination_value);");
			break;

		case INSTRUCTION_NBCD:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"NBCD\");");
			break;

		case INSTRUCTION_SWAP:
			Emit("result_value = ((destination_value & 0x0000FFFF) << 16) | ((destination_value & 0xFFFF0000) >> 16);");
			break;

		case INSTRUCTION_PEA:
			Emit("state->address_registers[7] -= 4;");
			Emit("WriteLongWord(state, callbacks, state->address_registers[7], source_value);");
			break;

		case INSTRUCTION_ILLEGAL:
			/* Illegal instruction. */
			Emit("Group1Or2Exception(state, callbacks, 4);");
			break;

		case INSTRUCTION_TAS:
			/* TODO - This instruction doesn't work properly on memory on the Mega Drive */
			Emit("state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);");
			Emit("state->status_register |= CONDITION_CODE_NEGATIVE * ((destination_value & 0x80) != 0);");
			Emit("state->status_register |= CONDITION_CODE_ZERO * (destination_value == 0);");
			Emit("");
			Emit("result_value = destination_value | 0x80;");
			break;

		case INSTRUCTION_TRAP:
			Emit("Group1Or2Exception(state, callbacks, 32 + source_value);");
			break;

		case INSTRUCTION_MOVE_USP:
			Emit("if (opcode.raw & 8)");
			Emit("	state->address_registers[opcode.primary_register] = state->user_stack_pointer;");
			Emit("else");
			Emit("	state->user_stack_pointer = state->address_registers[opcode.primary_register];");

			break;

		case INSTRUCTION_RESET:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"RESET\");");
			break;

		case INSTRUCTION_STOP:
			/* TODO */
			/*if (supervisor_mode)
				state->status_register = source_value;
			else
				TRAP();*/
			Emit("UNIMPLEMENTED_INSTRUCTION(\"STOP\");");

			break;

		case INSTRUCTION_RTE:
			Emit("state->status_register = (unsigned short)ReadWord(state, callbacks, state->address_registers[7]);");
			Emit("state->address_registers[7] += 2;");
			Emit("state->program_counter = ReadLongWord(state, callbacks, state->address_registers[7]);");
			Emit("state->address_registers[7] += 4;");
			break;

		case INSTRUCTION_RTS:
			Emit("state->program_counter = ReadLongWord(state, callbacks, state->address_registers[7]);");
			Emit("state->address_registers[7] += 4;");
			break;

		case INSTRUCTION_TRAPV:
			Emit("if (state->status_register & CONDITION_CODE_OVERFLOW)");
			Emit("	Group1Or2Exception(state, callbacks, 32 + 7);");

			break;

		case INSTRUCTION_RTR:
			Emit("state->status_register &= 0xFF00;");
			Emit("state->status_register |= ReadByte(callbacks, state->address_registers[7] + 1);");
			Emit("state->address_registers[7] += 2;");
			Emit("state->program_counter = ReadLongWord(state, callbacks, state->address_registers[7]);");
			Emit("state->address_registers[7] += 4;");
			break;

		case INSTRUCTION_JSR:
			Emit("state->address_registers[7] -= 4;");
			Emit("WriteLongWord(state, callbacks, state->address_registers[7], state->program_counter);");
			Emit("");
			/* Fallthrough */
		case INSTRUCTION_JMP:
			Emit("state->program_counter = source_value;");
			break;

		case INSTRUCTION_MOVEM:
			Emit("{");
			Emit("/* Hot damn is this a mess */");
			Emit("unsigned long memory_address = DecodeMemoryAddressMode(state, callbacks, 0, opcode.primary_address_mode, opcode.primary_register);");
			Emit("unsigned int i;");
			Emit("unsigned int bitfield;");
			Emit("");
			Emit("int delta;");
			Emit("void (*write_function)(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned long address, unsigned long value);");
			Emit("");
			Emit("if (opcode.raw & 0x0040)");
			Emit("{");
			Emit("	delta = 4;");
			Emit("	write_function = WriteLongWord;");
			Emit("}");
			Emit("else");
			Emit("{");
			Emit("	delta = 2;");
			Emit("	write_function = WriteWord;");
			Emit("}");
			Emit("");
			Emit("if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)");
			Emit("	delta = -delta;");
			Emit("");
			Emit("bitfield = source_value;");
			Emit("");
			Emit("/* First group of registers */");
			Emit("for (i = 0; i < 8; ++i)");
			Emit("{");
			Emit("	if (bitfield & 1)");
			Emit("	{");
			Emit("		if (opcode.raw & 0x0400)");
			Emit("		{");
			Emit("			/* Memory to register */");
			Emit("			if (opcode.raw & 0x0040)");
			Emit("				state->data_registers[i] = ReadLongWord(state, callbacks, memory_address);");
			Emit("			else");
			Emit("				state->data_registers[i] = CC_SIGN_EXTEND_ULONG(15, ReadWord(state, callbacks, memory_address));");
			Emit("		}");
			Emit("		else");
			Emit("		{");
			Emit("			/* Register to memory */");
			Emit("			if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)");
			Emit("				write_function(state, callbacks, memory_address + delta, state->address_registers[7 - i]);");
			Emit("			else");
			Emit("				write_function(state, callbacks, memory_address, state->data_registers[i]);");
			Emit("		}");
			Emit("");
			Emit("		memory_address += delta;");
			Emit("	}");
			Emit("");
			Emit("	bitfield >>= 1;");
			Emit("}");
			Emit("");
			Emit("/* Second group of registers */");
			Emit("for (i = 0; i < 8; ++i)");
			Emit("{");
			Emit("	if (bitfield & 1)");
			Emit("	{");
			Emit("		if (opcode.raw & 0x0400)");
			Emit("		{");
			Emit("			/* Memory to register */");
			Emit("			if (opcode.raw & 0x0040)");
			Emit("				state->address_registers[i] = ReadLongWord(state, callbacks, memory_address);");
			Emit("			else");
			Emit("				state->address_registers[i] = CC_SIGN_EXTEND_ULONG(15, ReadWord(state, callbacks, memory_address));");
			Emit("		}");
			Emit("		else");
			Emit("		{");
			Emit("			/* Register to memory */");
			Emit("			if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)");
			Emit("				write_function(state, callbacks, memory_address + delta, state->data_registers[7 - i]);");
			Emit("			else");
			Emit("				write_function(state, callbacks, memory_address, state->address_registers[i]);");
			Emit("		}");
			Emit("");
			Emit("		memory_address += delta;");
			Emit("	}");
			Emit("");
			Emit("	bitfield >>= 1;");
			Emit("}");
			Emit("");
			Emit("if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT || opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT)");
			Emit("	state->address_registers[opcode.primary_register] = memory_address;");
			Emit("}");

			break;

		case INSTRUCTION_CHK:
			Emit("{");
			Emit("const unsigned long value = state->data_registers[opcode.secondary_register];");
			Emit("");
			Emit("if (value & 0x8000)");
			Emit("{");
			Emit("	/* Value is smaller than 0. */");
			Emit("	state->status_register |= CONDITION_CODE_NEGATIVE;");
			Emit("	Group1Or2Exception(state, callbacks, 6);");
			Emit("}");
			Emit("else if (value > source_value)");
			Emit("{");
			Emit("	/* Value is greater than upper bound. */");
			Emit("	state->status_register &= ~CONDITION_CODE_NEGATIVE;");
			Emit("	Group1Or2Exception(state, callbacks, 6);");
			Emit("}");
			Emit("}");

			break;

		case INSTRUCTION_SCC:
			Emit("result_value = IsOpcodeConditionTrue(state, opcode.raw) ? 0xFF : 0;");
			break;

		case INSTRUCTION_DBCC:
			Emit("if (!IsOpcodeConditionTrue(state, opcode.raw))");
			Emit("{");
			Emit("	unsigned int loop_counter = state->data_registers[opcode.primary_register] & 0xFFFF;");
			Emit("");
			Emit("	if (loop_counter-- != 0)");
			Emit("	{");
			Emit("		state->program_counter -= 2;");
			Emit("		state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);");
			Emit("	}");
			Emit("");
			Emit("	state->data_registers[opcode.primary_register] &= ~0xFFFFul;");
			Emit("	state->data_registers[opcode.primary_register] |= loop_counter & 0xFFFF;");
			Emit("}");

			break;

		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
			if (instruction == INSTRUCTION_BCC_SHORT || instruction == INSTRUCTION_BCC_WORD)
			{
				Emit("if (IsOpcodeConditionTrue(state, opcode.raw))");
				Emit("{");
				++emit_indentation;
			}
			else if (instruction == INSTRUCTION_BSR_SHORT || instruction == INSTRUCTION_BSR_WORD)
			{
				Emit("state->address_registers[7] -= 4;");
				Emit("WriteLongWord(state, callbacks, state->address_registers[7], state->program_counter);");
				Emit("");
			}

			if (instruction == INSTRUCTION_BRA_SHORT || instruction == INSTRUCTION_BSR_SHORT || instruction == INSTRUCTION_BCC_SHORT)
			{
				Emit("state->program_counter += CC_SIGN_EXTEND_ULONG(7, opcode.raw);");
			}
			else
			{
				Emit("state->program_counter -= 2;");
				Emit("state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);");
			}

			if (instruction == INSTRUCTION_BCC_SHORT || instruction == INSTRUCTION_BCC_WORD)
			{
				--emit_indentation;
				Emit("}");
			}

			break;

		case INSTRUCTION_MOVEQ:
			Emit("result_value = CC_SIGN_EXTEND_ULONG(7, opcode.raw);");
			break;

		case INSTRUCTION_DIVS:
		case INSTRUCTION_DIVU:
			Emit("{");
			Emit("const cc_bool source_is_negative = instruction == INSTRUCTION_DIVS && source_value & 0x8000;");
			Emit("const cc_bool destination_is_negative = instruction == INSTRUCTION_DIVS && state->data_registers[opcode.secondary_register] & 0x80000000;");
			Emit("const cc_bool result_is_negative = source_is_negative != destination_is_negative;");
			Emit("");
			Emit("const unsigned int absolute_source_value = source_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;");
			Emit("const unsigned long absolute_destination_value = destination_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(31, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFFFFFF;");
			Emit("");
			Emit("if (source_value == 0)");
			Emit("{");
			Emit("	Group1Or2Exception(state, callbacks, 5);");
			Emit("}");
			Emit("else");
			Emit("{");
			Emit("	const unsigned long absolute_quotient = absolute_destination_value / absolute_source_value;");
			Emit("	const unsigned long quotient = result_is_negative ? 0 - absolute_quotient : absolute_quotient;");
			Emit("");
			Emit("	state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO | CONDITION_CODE_OVERFLOW);");
			Emit("");
			Emit("	/* Overflow detection */");
			Emit("	if (absolute_quotient > (instruction == INSTRUCTION_DIVU ? 0xFFFFul : (result_is_negative ? 0x8000ul : 0x7FFFul)))");
			Emit("	{");
			Emit("		state->status_register |= CONDITION_CODE_OVERFLOW;");
			Emit("	}");
			Emit("	else");
			Emit("	{");
			Emit("		const unsigned int absolute_remainder = absolute_destination_value % absolute_source_value;");
			Emit("		const unsigned int remainder = destination_is_negative ? 0 - absolute_remainder : absolute_remainder;");
			Emit("");
			Emit("		state->data_registers[opcode.secondary_register] = (unsigned long)(quotient & 0xFFFF) | ((unsigned long)(remainder & 0xFFFF) << 16);");
			Emit("	}");
			Emit("");
			Emit("	state->status_register |= CONDITION_CODE_NEGATIVE * ((quotient & 0x8000) != 0);");
			Emit("	state->status_register |= CONDITION_CODE_ZERO * (quotient == 0);");
			Emit("}");
			Emit("}");

			break;

		case INSTRUCTION_SBCD:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"SBCD\");");
			break;

		case INSTRUCTION_SUBX:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"SUBX\");");
			break;

		case INSTRUCTION_MULS:
		case INSTRUCTION_MULU:
			Emit("{");
			Emit("const cc_bool multiplier_is_negative = instruction == INSTRUCTION_MULS && source_value & 0x8000;");
			Emit("const cc_bool multiplicand_is_negative = instruction == INSTRUCTION_MULS && state->data_registers[opcode.secondary_register] & 0x8000;");
			Emit("const cc_bool result_is_negative = multiplier_is_negative != multiplicand_is_negative;");
			Emit("");
			Emit("const unsigned int multiplier = multiplier_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;");
			Emit("const unsigned int multiplicand = multiplicand_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFF;");
			Emit("");
			Emit("const unsigned long absolute_result = (unsigned long)multiplicand * multiplier;");
			Emit("const unsigned long result = result_is_negative ? 0 - absolute_result : absolute_result;");
			Emit("");
			Emit("state->data_registers[opcode.secondary_register] = result;");
			Emit("");
			Emit("state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);");
			Emit("state->status_register |= CONDITION_CODE_NEGATIVE * ((result & 0x80000000) != 0);");
			Emit("state->status_register |= CONDITION_CODE_ZERO * (result == 0);");
			Emit("}");

			break;

		case INSTRUCTION_ABCD:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"ABCD\");");
			break;

		case INSTRUCTION_EXG:
			Emit("{");
			Emit("unsigned long temp;");
			Emit("");
			Emit("switch (opcode.raw & 0x00F8)");
			Emit("{");
			Emit("	case 0x0040:");
			Emit("		temp = state->data_registers[opcode.secondary_register];");
			Emit("		state->data_registers[opcode.secondary_register] = state->data_registers[opcode.primary_register];");
			Emit("		state->data_registers[opcode.primary_register] = temp;");
			Emit("		break;");
			Emit("");
			Emit("	case 0x0048:");
			Emit("		temp = state->address_registers[opcode.secondary_register];");
			Emit("		state->address_registers[opcode.secondary_register] = state->address_registers[opcode.primary_register];");
			Emit("		state->address_registers[opcode.primary_register] = temp;");
			Emit("		break;");
			Emit("");
			Emit("	case 0x0088:");
			Emit("		temp = state->data_registers[opcode.secondary_register];");
			Emit("		state->data_registers[opcode.secondary_register] = state->address_registers[opcode.primary_register];");
			Emit("		state->address_registers[opcode.primary_register] = temp;");
			Emit("		break;");
			Emit("}");
			Emit("}");

			break;

		case INSTRUCTION_ADDX:
			/* TODO */
			Emit("UNIMPLEMENTED_INSTRUCTION(\"ADDX\");");
			break;

		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_LSD_MEMORY:
		case INSTRUCTION_LSD_REGISTER:
		case INSTRUCTION_ROD_MEMORY:
		case INSTRUCTION_ROD_REGISTER:
		case INSTRUCTION_ROXD_MEMORY:
		case INSTRUCTION_ROXD_REGISTER:
			Emit("{");
			Emit("const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);");
			Emit("const unsigned long original_sign_bit = destination_value & sign_bit_bitmask;");
			Emit("unsigned int i;");
			Emit("unsigned int count;");
			Emit("");
			Emit("result_value = destination_value;");
			Emit("");

			switch (instruction)
			{
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROXD_MEMORY:
					Emit("count = 1;");
					break;

				case INSTRUCTION_ASD_REGISTER:
				case INSTRUCTION_LSD_REGISTER:
				case INSTRUCTION_ROD_REGISTER:
				case INSTRUCTION_ROXD_REGISTER:
					Emit("count = opcode.raw & 0x0020 ? state->data_registers[opcode.secondary_register] % 64 : ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */");
					break;

				default:
					/* Shut up dumbass compiler warnings */
					break;
			}

			Emit("");
			Emit("state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);");
			Emit("");
			Emit("if (opcode.bit_8)");
			Emit("{");
			Emit("	/* Left */");
			Emit("	for (i = 0; i < count; ++i)");
			Emit("	{");
			Emit("		state->status_register &= ~CONDITION_CODE_CARRY;");
			Emit("		state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);");
			Emit("");

			switch (instruction)
			{
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_ASD_REGISTER:
					Emit("		state->status_register |= CONDITION_CODE_OVERFLOW * ((result_value & sign_bit_bitmask) != original_sign_bit);");
					Emit("		result_value <<= 1;");
					break;

				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_LSD_REGISTER:
					Emit("		result_value <<= 1;");
					break;

				case INSTRUCTION_ROXD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
					Emit("		result_value <<= 1;");
					Emit("		result_value |= 1 * ((state->status_register & CONDITION_CODE_EXTEND) != 0);");
					break;

				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROD_REGISTER:
					Emit("		result_value = (result_value << 1) | (1 * ((result_value & sign_bit_bitmask) != 0));");
					break;

				default:
					/* Shut up dumbass compiler warnings */
					break;
			}

			if (instruction != INSTRUCTION_ROD_MEMORY && instruction != INSTRUCTION_ROD_REGISTER)
			{
				Emit("");
				Emit("		state->status_register &= ~CONDITION_CODE_EXTEND;");
				Emit("		state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);");
			}

			Emit("	}");
			Emit("}");
			Emit("else");
			Emit("{");
			Emit("	/* Right */");
			Emit("	for (i = 0; i < count; ++i)");
			Emit("	{");
			Emit("		state->status_register &= ~CONDITION_CODE_CARRY;");
			Emit("		state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);");
			Emit("");

			switch (instruction)
			{
				case INSTRUCTION_ASD_MEMORY:
				case INSTRUCTION_ASD_REGISTER:
					Emit("		result_value >>= 1;");
					Emit("		result_value |= original_sign_bit;");
					break;

				case INSTRUCTION_LSD_MEMORY:
				case INSTRUCTION_LSD_REGISTER:
					Emit("		result_value >>= 1;");
					break;

				case INSTRUCTION_ROXD_MEMORY:
				case INSTRUCTION_ROXD_REGISTER:
					Emit("		result_value >>= 1;");
					Emit("		result_value |= sign_bit_bitmask * ((state->status_register & CONDITION_CODE_EXTEND) != 0);");
					break;

				case INSTRUCTION_ROD_MEMORY:
				case INSTRUCTION_ROD_REGISTER:
					Emit("		result_value = (result_value >> 1) | (sign_bit_bitmask * ((result_value & 1) != 0));");
					break;

				default:
					/* Shut up dumbass compiler warnings */
					break;
			}

			if (instruction != INSTRUCTION_ROD_MEMORY && instruction != INSTRUCTION_ROD_REGISTER)
			{
				Emit("");
				Emit("		state->status_register &= ~CONDITION_CODE_EXTEND;");
				Emit("		state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);");
			}

			Emit("	}");
			Emit("}");
			Emit("}");

			break;

		case INSTRUCTION_UNIMPLEMENTED_1:
			Emit("Group1Or2Exception(state, callbacks, 10);");
			break;

		case INSTRUCTION_UNIMPLEMENTED_2:
			Emit("Group1Or2Exception(state, callbacks, 11);");
			break;

		case INSTRUCTION_NOP:
			Emit("/* Doesn't do anything */");
			break;
	}
}

void EmitInstructionWriteDestinationOperand(const Instruction instruction)
{
	/* Write output to destination */
	switch (instruction)
	{
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_EORI_TO_CCR:
		case INSTRUCTION_MOVE_TO_CCR:
		case INSTRUCTION_ORI_TO_CCR:
			Emit("/* Write to condition code register */");
			Emit("state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);");
			break;

		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_EORI_TO_SR:
		case INSTRUCTION_MOVE_TO_SR:
		case INSTRUCTION_ORI_TO_SR:
			Emit("/* Write to status register */");
			Emit("state->status_register = (unsigned short)result_value;");
			break;

		case INSTRUCTION_ABCD:
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ADDI:
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_ADDX:
		case INSTRUCTION_AND:
		case INSTRUCTION_ANDI:
		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_CLR:
		case INSTRUCTION_EOR:
		case INSTRUCTION_EORI:
		case INSTRUCTION_EXT:
		case INSTRUCTION_OR:
		case INSTRUCTION_ORI:
		case INSTRUCTION_LEA:
		case INSTRUCTION_LSD_MEMORY:
		case INSTRUCTION_LSD_REGISTER:
		case INSTRUCTION_MOVE:
		case INSTRUCTION_MOVE_FROM_SR:
		case INSTRUCTION_MOVEA:
		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_NBCD:
		case INSTRUCTION_NEG:
		case INSTRUCTION_NEGX:
		case INSTRUCTION_NOT:
		case INSTRUCTION_ROD_MEMORY:
		case INSTRUCTION_ROD_REGISTER:
		case INSTRUCTION_ROXD_MEMORY:
		case INSTRUCTION_ROXD_REGISTER:
		case INSTRUCTION_SBCD:
		case INSTRUCTION_SCC:
		case INSTRUCTION_SUB:
		case INSTRUCTION_SUBA:
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_SUBI:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_SUBX:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_TAS:
			Emit("/* Write to destination */");
			Emit("SetValueUsingDecodedAddressMode(state, callbacks, &destination_decoded_address_mode, result_value);");
			break;

		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
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
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_TST:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			Emit("/* Doesn't write anything */");
			break;
	}
}

void EmitInstructionConditionCodes(const Instruction instruction)
{
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	Emit("msb_mask = 1ul << (operation_size * 8 - 1);");
	Emit("sm = (source_value & msb_mask) != 0;");
	Emit("dm = (destination_value & msb_mask) != 0;");
	Emit("rm = (result_value & msb_mask) != 0;");
	Emit("");

	Emit("/* Update CARRY condition code */");
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
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDI:
		case INSTRUCTION_ADDX:
			Emit("state->status_register &= ~CONDITION_CODE_CARRY;");
			Emit("state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));");
			break;

		case INSTRUCTION_SUBQ:
		case INSTRUCTION_CMP:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_CMPI:
		case INSTRUCTION_CMPM:
		case INSTRUCTION_SUB:
		case INSTRUCTION_SUBI:
		case INSTRUCTION_SUBX:
			Emit("state->status_register &= ~CONDITION_CODE_CARRY;");
			Emit("state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));");
			break;

		case INSTRUCTION_NEG:
		case INSTRUCTION_NEGX:
			Emit("state->status_register &= ~CONDITION_CODE_CARRY;");
			Emit("state->status_register |= CONDITION_CODE_CARRY * (dm || rm);");
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
			Emit("state->status_register &= ~CONDITION_CODE_CARRY;");
			break;

		case INSTRUCTION_CHK:
			Emit("/* Undefined */");
			break;

		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
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
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* These instructions don't affect condition codes (unless they write to them directly) */
			break;
	}

	Emit("/* Update OVERFLOW condition code */");
	switch (instruction)
	{
		case INSTRUCTION_ADDQ:
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDI:
		case INSTRUCTION_ADDX:
			Emit("state->status_register &= ~CONDITION_CODE_OVERFLOW;");
			Emit("state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));");
			break;

		case INSTRUCTION_SUBQ:
		case INSTRUCTION_CMP:
		case INSTRUCTION_CMPA:
		case INSTRUCTION_CMPI:
		case INSTRUCTION_CMPM:
		case INSTRUCTION_SUB:
		case INSTRUCTION_SUBI:
		case INSTRUCTION_SUBX:
			Emit("state->status_register &= ~CONDITION_CODE_OVERFLOW;");
			Emit("state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));");
			break;

		case INSTRUCTION_NEG:
		case INSTRUCTION_NEGX:
			Emit("state->status_register &= ~CONDITION_CODE_OVERFLOW;");
			Emit("state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);");
			break;

		case INSTRUCTION_ASD_REGISTER:
		case INSTRUCTION_ASD_MEMORY:
		case INSTRUCTION_DIVS:
		case INSTRUCTION_DIVU:
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
		case INSTRUCTION_CLR:
		case INSTRUCTION_EOR:
		case INSTRUCTION_EORI:
		case INSTRUCTION_EXT:
		case INSTRUCTION_MOVE:
		case INSTRUCTION_MOVEQ:
		case INSTRUCTION_MULS:
		case INSTRUCTION_MULU:
		case INSTRUCTION_NOT:
		case INSTRUCTION_OR:
		case INSTRUCTION_ORI:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_TAS:
		case INSTRUCTION_TST:
			/* Always cleared */
			Emit("state->status_register &= ~CONDITION_CODE_OVERFLOW;");
			break;

		case INSTRUCTION_ABCD:
		case INSTRUCTION_CHK:
		case INSTRUCTION_NBCD:
		case INSTRUCTION_SBCD:
			Emit("/* Undefined */");
			break;

		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
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
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* These instructions don't affect condition codes (unless they write to them directly) */
			break;
	}

	Emit("/* Update ZERO condition code */");
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

		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDI:
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
		case INSTRUCTION_SWAP:
		case INSTRUCTION_TST:
			/* Standard behaviour: set if result is zero; clear otherwise */
			Emit("state->status_register &= ~CONDITION_CODE_ZERO;");
			Emit("state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);");
			break;

		case INSTRUCTION_DIVS:
		case INSTRUCTION_DIVU:
		case INSTRUCTION_MULS:
		case INSTRUCTION_MULU:
		case INSTRUCTION_TAS:
			/* The condition code is set in the actual instruction code */
			break;

		case INSTRUCTION_CHK:
			Emit("/* Undefined */");
			break;

		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
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
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* These instructions don't affect condition codes (unless they write to them directly) */
			break;
	}

	Emit("/* Update NEGATIVE condition code */");
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

		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDI:
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
		case INSTRUCTION_SUBX:
		case INSTRUCTION_SWAP:
		case INSTRUCTION_TST:
			/* Standard behaviour: set if result value is negative; clear otherwise */
			Emit("state->status_register &= ~CONDITION_CODE_NEGATIVE;");
			Emit("state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);");
			break;

		case INSTRUCTION_ABCD:
		case INSTRUCTION_NBCD:
		case INSTRUCTION_SBCD:
			Emit("/* Undefined */");
			break;

		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
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
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* These instructions don't affect condition codes (unless they write to them directly) */
			break;
	}

	Emit("/* Update EXTEND condition code */");
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
			/* The condition code is set in the actual instruction code */
			break;

		case INSTRUCTION_ADDQ:
		case INSTRUCTION_SUBQ:
		case INSTRUCTION_ABCD:
		case INSTRUCTION_ADD:
		case INSTRUCTION_ADDI:
		case INSTRUCTION_ADDX:
		case INSTRUCTION_NBCD:
		case INSTRUCTION_NEG:
		case INSTRUCTION_NEGX:
		case INSTRUCTION_SBCD:
		case INSTRUCTION_SUB:
		case INSTRUCTION_SUBI:
		case INSTRUCTION_SUBX:
			/* Standard behaviour: set to CARRY */
			Emit("state->status_register &= ~CONDITION_CODE_EXTEND;");
			Emit("state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);");
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
		case INSTRUCTION_TAS:
		case INSTRUCTION_TST:
			Emit("/* Unaffected */");
			break;

		case INSTRUCTION_ADDA:
		case INSTRUCTION_ADDAQ:
		case INSTRUCTION_ANDI_TO_CCR:
		case INSTRUCTION_ANDI_TO_SR:
		case INSTRUCTION_BCC_SHORT:
		case INSTRUCTION_BCC_WORD:
		case INSTRUCTION_BCHG_DYNAMIC:
		case INSTRUCTION_BCHG_STATIC:
		case INSTRUCTION_BCLR_DYNAMIC:
		case INSTRUCTION_BCLR_STATIC:
		case INSTRUCTION_BRA_SHORT:
		case INSTRUCTION_BRA_WORD:
		case INSTRUCTION_BSET_DYNAMIC:
		case INSTRUCTION_BSET_STATIC:
		case INSTRUCTION_BSR_SHORT:
		case INSTRUCTION_BSR_WORD:
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
		case INSTRUCTION_SUBAQ:
		case INSTRUCTION_TRAP:
		case INSTRUCTION_TRAPV:
		case INSTRUCTION_UNLK:
		case INSTRUCTION_UNIMPLEMENTED_1:
		case INSTRUCTION_UNIMPLEMENTED_2:
			/* These instructions don't affect condition codes (unless they write to them directly) */
			break;
	}
}
