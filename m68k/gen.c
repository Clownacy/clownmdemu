case INSTRUCTION_ABCD:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADD:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDA:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDQ:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ADDQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDX:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && dm) || (!rm && dm) || (sm && !rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((sm && dm && !rm) || (!sm && !dm && rm));
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_AND:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI_TO_CCR:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ASD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ASD_REGISTER:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ASD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_BCC_SHORT:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCC_SHORT;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BCC_WORD;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BRA_SHORT;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BRA_WORD;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSR_SHORT;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_BSR_WORD;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_CHK;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_CLR;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMP:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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
	/* Doesn't write anything */

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPA:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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
	/* Doesn't write anything */

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPI:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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
	/* Doesn't write anything */

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPM:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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
	/* Doesn't write anything */

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_DBCC:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DBCC;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DIV;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_DIV;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI_TO_CCR:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EXG;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_EXT;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ILLEGAL:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ILLEGAL;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_JMP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_JSR;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LINK;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LSD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_LSD_REGISTER:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_LSD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_FROM_SR:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE_USP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEA;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVEQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MULS:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MUL;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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

case INSTRUCTION_MULU:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MUL;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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

case INSTRUCTION_NBCD:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NBCD;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NEG:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NEG;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * (dm || rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NEGX:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NEGX;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * (dm || rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NOP:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NOP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_NOT;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_OR:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI_TO_CCR:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_PEA;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RESET;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROD_REGISTER:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROXD_MEMORY:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROXD_MEMORY;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ROXD_REGISTER:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_ROXD_REGISTER;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Unaffected */
	/* Update OVERFLOW condition code */
	/* Unaffected */
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_RTE:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTE;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTR;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_RTS;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SCC:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SCC;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_STOP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBA:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBQ:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SUBQ;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBX:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

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

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * ((sm && !dm) || (rm && !dm) || (sm && rm));
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * ((!sm && dm && !rm) || (sm && !dm && rm));
	/* Update ZERO condition code */
	/* TODO - "Cleared if the result is nonzero; unchanged otherwise" */
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Standard behaviour: set to CARRY */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SWAP:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_SWAP;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TAS:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, &decoded_opcode.operands[1]);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TAS;

	/* Write destination operand. */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TRAP;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_TRAPV;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, &decoded_opcode.operands[0]);

	/* Decode destination address mode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_MOVE;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	/* Update the condition codes in the following order: */
	/* CARRY, OVERFLOW, ZERO, NEGATIVE, EXTEND */
	msb_mask = 1ul << (operation_size * 8 - 1);
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Standard behaviour: set if result is zero; clear otherwise */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	/* Standard behaviour: set if result value is negative; clear otherwise */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * rm;
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_UNLK:
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNLK;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNIMPLEMENTED_1;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	/* Obtain instruction size. */
	operation_size = decoded_opcode.size;

	/* Decode source address mode. */

	/* Decode destination address mode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	DO_INSTRUCTION_ACTION_UNIMPLEMENTED_2;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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

