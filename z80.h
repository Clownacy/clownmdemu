#ifndef Z80_H
#define Z80_H

#include "clowncommon.h"

typedef enum Z80_Opcode
{
	Z80_OPCODE_NOP,
	Z80_OPCODE_EX_AF_AF,
	Z80_OPCODE_DJNZ,
	Z80_OPCODE_JR_UNCONDITIONAL,
	Z80_OPCODE_JR_CONDITIONAL,
	Z80_OPCODE_LD_16BIT,
	Z80_OPCODE_ADD_HL,
	Z80_OPCODE_LD_8BIT,
	Z80_OPCODE_INC_16BIT,
	Z80_OPCODE_DEC_16BIT,
	Z80_OPCODE_INC_8BIT,
	Z80_OPCODE_DEC_8BIT,
	Z80_OPCODE_RLCA,
	Z80_OPCODE_RRCA,
	Z80_OPCODE_RLA,
	Z80_OPCODE_RRA,
	Z80_OPCODE_DAA,
	Z80_OPCODE_CPL,
	Z80_OPCODE_SCF,
	Z80_OPCODE_CCF,
	Z80_OPCODE_HALT,
	Z80_OPCODE_ADD_A,
	Z80_OPCODE_ADC_A,
	Z80_OPCODE_SUB,
	Z80_OPCODE_SBC_A,
	Z80_OPCODE_AND,
	Z80_OPCODE_XOR,
	Z80_OPCODE_OR,
	Z80_OPCODE_CP,
	Z80_OPCODE_RET_CONDITIONAL,
	Z80_OPCODE_POP,
	Z80_OPCODE_RET_UNCONDITIONAL,
	Z80_OPCODE_EXX,
	Z80_OPCODE_JP_HL,
	Z80_OPCODE_LD_SP_HL,
	Z80_OPCODE_JP_CONDITIONAL,
	Z80_OPCODE_JP_UNCONDITIONAL,
	Z80_OPCODE_CB_PREFIX,
	Z80_OPCODE_OUT,
	Z80_OPCODE_IN,
	Z80_OPCODE_EX_SP_HL,
	Z80_OPCODE_EX_DE_HL,
	Z80_OPCODE_DI,
	Z80_OPCODE_EI,
	Z80_OPCODE_CALL_CONDITIONAL,
	Z80_OPCODE_PUSH,
	Z80_OPCODE_CALL_UNCONDITIONAL,
	Z80_OPCODE_DD_PREFIX,
	Z80_OPCODE_ED_PREFIX,
	Z80_OPCODE_FD_PREFIX,
	Z80_OPCODE_RST,
	Z80_OPCODE_RLC,
	Z80_OPCODE_RRC,
	Z80_OPCODE_RL,
	Z80_OPCODE_RR,
	Z80_OPCODE_SLA,
	Z80_OPCODE_SRA,
	Z80_OPCODE_SLL,
	Z80_OPCODE_SRL,
	Z80_OPCODE_BIT,
	Z80_OPCODE_RES,
	Z80_OPCODE_SET,
	Z80_OPCODE_IN_REGISTER,
	Z80_OPCODE_IN_NO_REGISTER,
	Z80_OPCODE_OUT_REGISTER,
	Z80_OPCODE_OUT_NO_REGISTER,
	Z80_OPCODE_SBC_HL,
	Z80_OPCODE_ADC_HL,
	Z80_OPCODE_NEG,
	Z80_OPCODE_RETN,
	Z80_OPCODE_RETI,
	Z80_OPCODE_IM,
	Z80_OPCODE_LD_I_A,
	Z80_OPCODE_LD_R_A,
	Z80_OPCODE_LD_A_I,
	Z80_OPCODE_LD_A_R,
	Z80_OPCODE_RRD,
	Z80_OPCODE_RLD,
	Z80_OPCODE_LDI,
	Z80_OPCODE_LDD,
	Z80_OPCODE_LDIR,
	Z80_OPCODE_LDDR,
	Z80_OPCODE_CPI,
	Z80_OPCODE_CPD,
	Z80_OPCODE_CPIR,
	Z80_OPCODE_CPDR,
	Z80_OPCODE_INI,
	Z80_OPCODE_IND,
	Z80_OPCODE_INIR,
	Z80_OPCODE_INDR,
	Z80_OPCODE_OUTI,
	Z80_OPCODE_OUTD,
	Z80_OPCODE_OTIR,
	Z80_OPCODE_OTDR
} Z80_Opcode;

typedef enum Z80_Operand
{
	Z80_OPERAND_NONE,
	Z80_OPERAND_A,
	Z80_OPERAND_B,
	Z80_OPERAND_C,
	Z80_OPERAND_D,
	Z80_OPERAND_E,
	Z80_OPERAND_H,
	Z80_OPERAND_L,
	Z80_OPERAND_AF,
	Z80_OPERAND_BC,
	Z80_OPERAND_DE,
	Z80_OPERAND_HL,
	Z80_OPERAND_PC,
	Z80_OPERAND_SP,
	Z80_OPERAND_BC_INDIRECT,
	Z80_OPERAND_DE_INDIRECT,
	Z80_OPERAND_HL_INDIRECT,
	Z80_OPERAND_ADDRESS,
	Z80_OPERAND_LITERAL_8BIT,
	Z80_OPERAND_LITERAL_16BIT
} Z80_Operand;

typedef enum Z80_Condition
{
	Z80_CONDITION_NOT_ZERO = 0,
	Z80_CONDITION_ZERO = 1,
	Z80_CONDITION_NOT_CARRY = 2,
	Z80_CONDITION_CARRY = 3,
	Z80_CONDITION_PARITY_OVERFLOW = 4,
	Z80_CONDITION_PARITY_EQUALITY = 5,
	Z80_CONDITION_PLUS = 6,
	Z80_CONDITION_MINUS = 7
} Z80_Condition;

enum
{
	Z80_FLAG_CARRY           = 1 << 0,
	Z80_FLAG_ADD_SUBTRACT    = 1 << 1,
	Z80_FLAG_PARITY_OVERFLOW = 1 << 2,
	Z80_FLAG_HALF_CARRY      = 1 << 4,
	Z80_FLAG_ZERO            = 1 << 6,
	Z80_FLAG_SIGN            = 1 << 7
};

typedef enum Z80_RegisterMode
{
	Z80_REGISTER_MODE_HL,
	Z80_REGISTER_MODE_IX,
	Z80_REGISTER_MODE_IY
} Z80_RegisterMode;

typedef enum Z80_InstructionMode
{
	Z80_INSTRUCTION_MODE_NORMAL,
	Z80_INSTRUCTION_MODE_BITS,
	Z80_INSTRUCTION_MODE_MISC
} Z80_InstructionMode;

typedef struct Z80_InstructionMetadata
{
	Z80_Opcode opcode;
	Z80_Operand operands[2];
	Z80_Condition condition;
	unsigned int embedded_literal;
	cc_bool read_destination;
	cc_bool write_destination;
	cc_bool has_displacement;
	cc_bool indirect_16bit;
} Z80_InstructionMetadata;

typedef struct Z80_Instruction
{
	Z80_InstructionMetadata metadata;
	unsigned short literal;
	signed char displacement;
} Z80_Instruction;

typedef struct Z80_RegistersAF
{
	unsigned char a;
	unsigned char f;
} Z80_RegistersAF;

typedef struct Z80_RegistersBCDEHL
{
	unsigned char b;
	unsigned char c;
	unsigned char d;
	unsigned char e;
	unsigned char h;
	unsigned char l;
} Z80_RegistersBCDEHL;

typedef struct Z80_State
{
	Z80_RegisterMode register_mode;
	Z80_InstructionMode instruction_mode;
	unsigned int cycles;
	unsigned short program_counter;
	unsigned short stack_pointer;
	Z80_RegistersAF af[2];
	Z80_RegistersBCDEHL bc_de_hl[2];
	unsigned char ixh, ixl, iyh, iyl;
	cc_bool alternate_af;
	cc_bool alternate_bc_de_hl;
	cc_bool interrupts_enabled;
} Z80_State;

typedef struct Z80_ReadInstructionCallback
{
	unsigned int (*callback)(void *user_data);
	void *user_data;
} Z80_ReadInstructionCallback;

typedef struct Z80_ReadCallback
{
	unsigned int (*callback)(void *user_data, unsigned int address);
	void *user_data;
} Z80_ReadCallback;

typedef struct Z80_WriteCallback
{
	void (*callback)(void *user_data, unsigned int address, unsigned int value);
	void *user_data;
} Z80_WriteCallback;

typedef struct Z80_ReadAndWriteCallbacks
{
	Z80_ReadCallback read;
	Z80_WriteCallback write;
} Z80_ReadAndWriteCallbacks;

void Z80_State_Initialise(Z80_State *state);
void Z80_Reset(Z80_State *state);
void Z80_Interrupt(Z80_State *state, const Z80_ReadAndWriteCallbacks *callbacks);
void Z80_DecodeInstructionMetadata(Z80_InstructionMetadata *metadata, Z80_InstructionMode instruction_mode, unsigned char opcode);
void Z80_DecodeInstruction(Z80_Instruction *instruction, Z80_InstructionMode instruction_mode, Z80_RegisterMode register_mode, const Z80_ReadInstructionCallback *read_callback);
void Z80_ExecuteInstruction(Z80_State *state, const Z80_Instruction *instruction, const Z80_ReadAndWriteCallbacks *callbacks);
void Z80_DoCycle(Z80_State *state, const Z80_ReadAndWriteCallbacks *callbacks);

#endif /* Z80_H */
