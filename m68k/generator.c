#include <stdio.h>
#include <stdlib.h>

#include "emit.h"
#include "emit-instructions.h"

int main(int argc, char **argv)
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

	size_t i;

	(void)argc;
	(void)argv;

	emit_file = stdout;

	for (i = 0; i < CC_COUNT_OF(instruction_strings); ++i)
	{
		EmitFormatted("case %s:", instruction_strings[i]);
		++emit_indentation;
		Emit("/* Obtain instruction size. */");
		EmitInstructionSize(i);
		Emit("");
		Emit("/* Decode source address mode. */");
		EmitInstructionSourceAddressMode(i);
		Emit("");
		Emit("/* Decode destination address mode. */");
		EmitInstructionDestinationAddressMode(i);
		Emit("");
		Emit("/* Read source operand. */");
		EmitInstructionReadSourceOperand(i);
		Emit("");
		Emit("/* Read destination operand. */");
		EmitInstructionReadDestinationOperand(i);
		Emit("");
		Emit("/* Do the actual instruction. */");
		EmitInstructionAction(i);
		Emit("");
		Emit("/* Write destination operand. */");
		EmitInstructionWriteDestinationOperand(i);
		Emit("");
		Emit("/* Update condition codes. */");
		EmitInstructionConditionCodes(i);
		Emit("");
		Emit("break;");
		--emit_indentation;
		Emit("");
	}

	return EXIT_SUCCESS;
}
