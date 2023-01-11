case INSTRUCTION_ABCD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ABCD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* TODO - "Decimal carry" */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_ADD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & dm) | (~rm & dm) | (sm & ~rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((sm & dm & ~rm) | (~sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_ADDA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDA;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ADDAQ:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ADDI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & dm) | (~rm & dm) | (sm & ~rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((sm & dm & ~rm) | (~sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_ADDQ:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & dm) | (~rm & dm) | (sm & ~rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((sm & dm & ~rm) | (~sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_ADDX:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDX;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & dm) | (~rm & dm) | (sm & ~rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((sm & dm & ~rm) | (~sm & ~dm & rm));
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_AND:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_AND;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_AND;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI_TO_CCR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_AND;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI_TO_SR:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_AND;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ASD_MEMORY:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ASD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ASD_REGISTER:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ASD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCC_SHORT:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCC_SHORT;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCC_WORD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCC_WORD;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCHG_DYNAMIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCHG;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCHG_STATIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCHG;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCLR_DYNAMIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCLR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCLR_STATIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCLR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BRA_SHORT:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BRA_SHORT;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BRA_WORD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BRA_WORD;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BSET_DYNAMIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSET;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BSET_STATIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSET;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BSR_SHORT:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSR_SHORT;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BSR_WORD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSR_WORD;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BTST_DYNAMIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BTST;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BTST_STATIC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BTST;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CHK:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_CHK;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Undefined */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* Undefined */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CLR:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_CLR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMP:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUB;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBA;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUB;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPM:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUB;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_DBCC:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DBCC;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_DIVS:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DIV;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_DIVU:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DIV;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EOR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EOR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EOR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI_TO_CCR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EOR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI_TO_SR:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EOR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EXG:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EXG;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EXT:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EXT;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ILLEGAL:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ILLEGAL;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_JMP:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_JMP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_JSR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_JSR;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_LEA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_LINK:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LINK;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_LSD_MEMORY:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LSD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_LSD_REGISTER:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LSD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_FROM_SR:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_TO_CCR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_TO_SR:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_USP:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE_USP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVEA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEA;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVEM:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEM;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVEP:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVEQ:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MULS:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MUL;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MULU:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MUL;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_NBCD:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NBCD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* TODO - "Decimal borrow" */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_NEG:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NEG;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & (dm | rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & (dm & rm);
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_NEGX:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NEGX;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & (dm | rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & (dm & rm);
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_NOP:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NOP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_NOT:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NOT;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_OR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_OR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_OR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI_TO_CCR:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_OR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI_TO_SR:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_OR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_PEA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_PEA;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_RESET:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RESET;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROD_MEMORY:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROD_REGISTER:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROXD_MEMORY:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROXD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROXD_REGISTER:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROXD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_RTE:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTE;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_RTR:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTR;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_RTS:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTS;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_SBCD:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SBCD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* TODO - "Decimal borrow" */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_SCC:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SCC;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_STOP:
	/* Only allow this instruction in supervisor mode. */
	if ((state->status_register & 0x2000) == 0)
	{
		Group1Or2Exception(&stuff, 8);
		longjmp(stuff.exception.context, 1);
	}

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_STOP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_SUB:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUB;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_SUBA:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBA;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_SUBAQ:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_SUBI:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUB;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_SUBQ:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_SUBX:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBX;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = 0 - ((source_value & msb_mask) != 0);
	dm = 0 - ((destination_value & msb_mask) != 0);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY & ((sm & ~dm) | (rm & ~dm) | (sm & rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW & ((~sm & dm & ~rm) | (sm & ~dm & rm));
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND & (0 - ((state->status_register & CONDITION_CODE_CARRY) != 0));

	break;

case INSTRUCTION_SWAP:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SWAP;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TAS:
	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TAS;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TRAP:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TRAP;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TRAPV:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TRAPV;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TST:
	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = 0 - ((result_value & msb_mask) != 0);

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO & (0 - ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0));
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE & rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_UNLK:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNLK;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_UNIMPLEMENTED_1:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNIMPLEMENTED_1;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_UNIMPLEMENTED_2:
	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNIMPLEMENTED_2;

	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Unaffected */
	/* Update NEGATIVE condition code */
	/* Unaffected */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

