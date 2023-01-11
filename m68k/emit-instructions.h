#ifndef M68K_EMIT_INSTRUCTIONS_H
#define M68K_EMIT_INSTRUCTIONS_H

#include "instruction.h"

void EmitInstructionSupervisorCheck(const Instruction instruction);
void EmitInstructionSourceAddressMode(const Instruction instruction);
void EmitInstructionDestinationAddressMode(const Instruction instruction);
void EmitInstructionReadSourceOperand(const Instruction instruction);
void EmitInstructionReadDestinationOperand(const Instruction instruction);
void EmitInstructionAction(const Instruction instruction);
void EmitInstructionWriteDestinationOperand(const Instruction instruction);
void EmitInstructionConditionCodes(const Instruction instruction);

#endif /* M68K_EMIT_INSTRUCTIONS_H */
