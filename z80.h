#ifndef Z80_H
#define Z80_H

#include "clowncommon.h"

typedef enum Opcode
{
	OPCODE_NOP,
	OPCODE_EX_AF_AF,
	OPCODE_DJNZ,
	OPCODE_JR_UNCONDITIONAL,
	OPCODE_JR_CONDITIONAL,
	OPCODE_LD_16BIT,
	OPCODE_ADD_HL,
	OPCODE_LD_8BIT,
	OPCODE_INC_16BIT,
	OPCODE_DEC_16BIT,
	OPCODE_INC_8BIT,
	OPCODE_DEC_8BIT,
	OPCODE_RLCA,
	OPCODE_RRCA,
	OPCODE_RLA,
	OPCODE_RRA,
	OPCODE_DAA,
	OPCODE_CPL,
	OPCODE_SCF,
	OPCODE_CCF,
	OPCODE_HALT,
	OPCODE_ADD_A,
	OPCODE_ADC_A,
	OPCODE_SUB,
	OPCODE_SBC_A,
	OPCODE_AND,
	OPCODE_XOR,
	OPCODE_OR,
	OPCODE_CP,
	OPCODE_RET_CONDITIONAL,
	OPCODE_POP,
	OPCODE_RET_UNCONDITIONAL,
	OPCODE_EXX,
	OPCODE_JP_HL,
	OPCODE_LD_SP_HL,
	OPCODE_JP_CONDITIONAL,
	OPCODE_JP_UNCONDITIONAL,
	OPCODE_CB_PREFIX,
	OPCODE_OUT,
	OPCODE_IN,
	OPCODE_EX_SP_HL,
	OPCODE_EX_DE_HL,
	OPCODE_DI,
	OPCODE_EI,
	OPCODE_CALL_CONDITIONAL,
	OPCODE_PUSH,
	OPCODE_CALL_UNCONDITIONAL,
	OPCODE_DD_PREFIX,
	OPCODE_ED_PREFIX,
	OPCODE_FD_PREFIX,
	OPCODE_RST,
	OPCODE_RLC,
	OPCODE_RRC,
	OPCODE_RL,
	OPCODE_RR,
	OPCODE_SLA,
	OPCODE_SRA,
	OPCODE_SLL,
	OPCODE_SRL,
	OPCODE_BIT,
	OPCODE_RES,
	OPCODE_SET,
	OPCODE_IN_REGISTER,
	OPCODE_IN_NO_REGISTER,
	OPCODE_OUT_REGISTER,
	OPCODE_OUT_NO_REGISTER,
	OPCODE_SBC_HL,
	OPCODE_ADC_HL,
	OPCODE_NEG,
	OPCODE_RETN,
	OPCODE_RETI,
	OPCODE_IM,
	OPCODE_LD_I_A,
	OPCODE_LD_R_A,
	OPCODE_LD_A_I,
	OPCODE_LD_A_R,
	OPCODE_RRD,
	OPCODE_RLD,
	OPCODE_LDI,
	OPCODE_LDD,
	OPCODE_LDIR,
	OPCODE_LDDR,
	OPCODE_CPI,
	OPCODE_CPD,
	OPCODE_CPIR,
	OPCODE_CPDR,
	OPCODE_INI,
	OPCODE_IND,
	OPCODE_INIR,
	OPCODE_INDR,
	OPCODE_OUTI,
	OPCODE_OUTD,
	OPCODE_OTIR,
	OPCODE_OTDR
} Opcode;

typedef enum Operand
{
	OPERAND_NONE,
	OPERAND_A,
	OPERAND_B,
	OPERAND_C,
	OPERAND_D,
	OPERAND_E,
	OPERAND_H,
	OPERAND_L,
	OPERAND_IXH,
	OPERAND_IXL,
	OPERAND_IYH,
	OPERAND_IYL,
	OPERAND_AF,
	OPERAND_BC,
	OPERAND_DE,
	OPERAND_HL,
	OPERAND_IX,
	OPERAND_IY,
	OPERAND_PC,
	OPERAND_SP,
	OPERAND_BC_INDIRECT,
	OPERAND_DE_INDIRECT,
	OPERAND_HL_INDIRECT,
	OPERAND_IX_INDIRECT,
	OPERAND_IY_INDIRECT,
	OPERAND_ADDRESS,
	OPERAND_LITERAL_8BIT,
	OPERAND_LITERAL_16BIT
} Operand;

typedef enum Condition
{
	CONDITION_NOT_ZERO = 0,
	CONDITION_ZERO = 1,
	CONDITION_NOT_CARRY = 2,
	CONDITION_CARRY = 3,
	CONDITION_PARITY_OVERFLOW = 4,
	CONDITION_PARITY_EQUALITY = 5,
	CONDITION_PLUS = 6,
	CONDITION_MINUS = 7
} Condition;


typedef enum Z80_RegisterMode
{
	Z80_REGISTER_MODE_HL,
	Z80_REGISTER_MODE_IX,
	Z80_REGISTER_MODE_IY
} Z80_RegisterMode;

typedef struct InstructionMetadata
{
	Opcode opcode;
	Operand operands[2];
	Condition condition;
	unsigned int embedded_literal;
	cc_bool read_destination;
	cc_bool write_destination;
	cc_bool has_displacement;
	cc_bool indirect_16bit;
} InstructionMetadata;

typedef struct Z80_Constant
{
	InstructionMetadata instruction_metadata_lookup_normal[3][0x100];
	InstructionMetadata instruction_metadata_lookup_bits[0x100];
	InstructionMetadata instruction_metadata_lookup_misc[0x100];
	unsigned char parity_lookup[0x100];
} Z80_Constant;

typedef struct Z80_State
{
	Z80_RegisterMode register_mode;
	unsigned int cycles;
	unsigned short program_counter;
	unsigned short stack_pointer;
	unsigned char a, f, b, c, d, e, h, l;
	unsigned char a_, f_, b_, c_, d_, e_, h_, l_; /* Backup registers. */
	unsigned char ixh, ixl, iyh, iyl;
	unsigned char r, i;
	cc_bool interrupts_enabled;
} Z80_State;

typedef struct Z80_ReadAndWriteCallbacks
{
	unsigned int (*read)(void *user_data, unsigned int address);
	void (*write)(void *user_data, unsigned int address, unsigned int value);
	void *user_data;
} Z80_ReadAndWriteCallbacks;

typedef struct Z80
{
	const Z80_Constant *constant;
	Z80_State *state;
} Z80;

void Z80_Constant_Initialise(Z80_Constant *constant);
void Z80_State_Initialise(Z80_State *state);
void Z80_Reset(const Z80 *z80);
void Z80_Interrupt(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks);
void Z80_DoCycle(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks);

#endif /* Z80_H */
