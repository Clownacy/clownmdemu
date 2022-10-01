#ifndef M68K_EMIT_INSTRUCTIONS
#define M68K_EMIT_INSTRUCTIONS

#include "instructions.h"

void EmitInstructionSize(const Instruction instruction);
void EmitInstructionReadSourceOperand(const Instruction instruction);
void EmitInstructionReadDestinationOperand(const Instruction instruction);
void EmitInstructionWriteDestinationOperand(const Instruction instruction);
void EmitInstructionAction(const Instruction instruction);
void EmitInstructionConditionCodes(const Instruction instruction);

#endif /* M68K_EMIT_INSTRUCTIONS */
