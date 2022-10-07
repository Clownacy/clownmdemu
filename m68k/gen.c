case INSTRUCTION_ABCD:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("ABCD");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADD:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode or secondary data register, based on direction bit. */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);
	else
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Primary address mode or secondary data register, based on direction bit */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);
	else
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value + source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDA:
	/* Obtain instruction size. */
	/* Word or longword based on bit 8. */
	operation_size = opcode.bit_8 ? 4 : 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Full secondary address register */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	if (!opcode.bit_8)
		source_value = CC_SIGN_EXTEND_ULONG(15, source_value);

	result_value = destination_value + source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ADDAQ:
	/* Obtain instruction size. */
	/* Hardcoded to a longword. */
	operation_size = 4;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value + source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ADDI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value + source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDQ:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value + source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_ADDX:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("ADDX");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_AND:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode or secondary data register, based on direction bit. */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);
	else
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Primary address mode or secondary data register, based on direction bit */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);
	else
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value & source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value & source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ANDI_TO_CCR:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value & source_value;

	/* Write destination operand. */
	/* Write to condition code register */
	state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ANDI_TO_SR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value & source_value;

	/* Write destination operand. */
	/* Write to status register */
	state->status_register = (unsigned short)result_value;

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ASD_MEMORY:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	const unsigned long original_sign_bit = destination_value & sign_bit_bitmask;
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = 1;

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			state->status_register |= CONDITION_CODE_OVERFLOW * ((result_value & sign_bit_bitmask) != original_sign_bit);
			result_value <<= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;
			result_value |= original_sign_bit;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ASD_REGISTER:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	const unsigned long original_sign_bit = destination_value & sign_bit_bitmask;
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = opcode.raw & 0x0020 ? state->data_registers[opcode.secondary_register] % 64 : ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			state->status_register |= CONDITION_CODE_OVERFLOW * ((result_value & sign_bit_bitmask) != original_sign_bit);
			result_value <<= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;
			result_value |= original_sign_bit;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCC_SHORT:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	if (IsOpcodeConditionTrue(state, opcode.raw))
	{
		state->program_counter += CC_SIGN_EXTEND_ULONG(7, opcode.raw);
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCC_WORD:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Immediate value (word). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	if (IsOpcodeConditionTrue(state, opcode.raw))
	{
		state->program_counter -= 2;
		state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCHG_DYNAMIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Secondary data register. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value ^ (1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCHG_STATIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Immediate value (byte). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value ^ (1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCLR_DYNAMIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Secondary data register. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value & ~(1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BCLR_STATIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Immediate value (byte). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value & ~(1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BRA_SHORT:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->program_counter += CC_SIGN_EXTEND_ULONG(7, opcode.raw);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BRA_WORD:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Immediate value (word). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->program_counter -= 2;
	state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BSET_DYNAMIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Secondary data register. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value | (1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BSET_STATIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Immediate value (byte). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	result_value = destination_value | (1ul << source_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BSR_SHORT:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->address_registers[7] -= 4;
	WriteLongWord(&stuff, state->address_registers[7], state->program_counter);

	state->program_counter += CC_SIGN_EXTEND_ULONG(7, opcode.raw);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BSR_WORD:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Immediate value (word). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->address_registers[7] -= 4;
	WriteLongWord(&stuff, state->address_registers[7], state->program_counter);

	state->program_counter -= 2;
	state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BTST_DYNAMIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Secondary data register. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_BTST_STATIC:
	/* Obtain instruction size. */
	/* 4 if register - 1 if memory. */
	operation_size = opcode.primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;

	/* Decode source address mode. */
	/* Immediate value (byte). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	/* Modulo the source value */
	source_value &= operation_size * 8 - 1;

	/* Set the zero flag to the specified bit */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((destination_value & (1ul << source_value)) == 0);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_CHK:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	const unsigned long value = state->data_registers[opcode.secondary_register];

	if (value & 0x8000)
	{
		/* Value is smaller than 0. */
		state->status_register |= CONDITION_CODE_NEGATIVE;
		Group1Or2Exception(&stuff, 6);
	}
	else if (value > source_value)
	{
		/* Value is greater than upper bound. */
		state->status_register &= ~CONDITION_CODE_NEGATIVE;
		Group1Or2Exception(&stuff, 6);
	}
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Undefined */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* Undefined */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CLR:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* For some reason, this instruction reads from its destination even though it doesn't use it. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = 0;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMP:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Data register (secondary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPA:
	/* Obtain instruction size. */
	/* Word or longword based on bit 8. */
	operation_size = opcode.bit_8 ? 4 : 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Full secondary address register */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	if (!opcode.bit_8)
		source_value = CC_SIGN_EXTEND_ULONG(15, source_value);

	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_CMPM:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode.primary_register);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_DBCC:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	if (!IsOpcodeConditionTrue(state, opcode.raw))
	{
		unsigned int loop_counter = state->data_registers[opcode.primary_register] & 0xFFFF;

		if (loop_counter-- != 0)
		{
			state->program_counter -= 2;
			state->program_counter += CC_SIGN_EXTEND_ULONG(15, source_value);
		}

		state->data_registers[opcode.primary_register] &= ~0xFFFFul;
		state->data_registers[opcode.primary_register] |= loop_counter & 0xFFFF;
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_DIVS:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	const cc_bool source_is_negative = instruction == INSTRUCTION_DIVS && source_value & 0x8000;
	const cc_bool destination_is_negative = instruction == INSTRUCTION_DIVS && state->data_registers[opcode.secondary_register] & 0x80000000;
	const cc_bool result_is_negative = source_is_negative != destination_is_negative;

	const unsigned int absolute_source_value = source_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;
	const unsigned long absolute_destination_value = destination_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(31, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFFFFFF;

	if (source_value == 0)
	{
		Group1Or2Exception(&stuff, 5);
	}
	else
	{
		const unsigned long absolute_quotient = absolute_destination_value / absolute_source_value;
		const unsigned long quotient = result_is_negative ? 0 - absolute_quotient : absolute_quotient;

		state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO | CONDITION_CODE_OVERFLOW);

		/* Overflow detection */
		if (absolute_quotient > (instruction == INSTRUCTION_DIVU ? 0xFFFFul : (result_is_negative ? 0x8000ul : 0x7FFFul)))
		{
			state->status_register |= CONDITION_CODE_OVERFLOW;
		}
		else
		{
			const unsigned int absolute_remainder = absolute_destination_value % absolute_source_value;
			const unsigned int remainder = destination_is_negative ? 0 - absolute_remainder : absolute_remainder;

			state->data_registers[opcode.secondary_register] = (unsigned long)(quotient & 0xFFFF) | ((unsigned long)(remainder & 0xFFFF) << 16);
		}

		state->status_register |= CONDITION_CODE_NEGATIVE * ((quotient & 0x8000) != 0);
		state->status_register |= CONDITION_CODE_ZERO * (quotient == 0);
	}
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_DIVU:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	const cc_bool source_is_negative = instruction == INSTRUCTION_DIVS && source_value & 0x8000;
	const cc_bool destination_is_negative = instruction == INSTRUCTION_DIVS && state->data_registers[opcode.secondary_register] & 0x80000000;
	const cc_bool result_is_negative = source_is_negative != destination_is_negative;

	const unsigned int absolute_source_value = source_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;
	const unsigned long absolute_destination_value = destination_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(31, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFFFFFF;

	if (source_value == 0)
	{
		Group1Or2Exception(&stuff, 5);
	}
	else
	{
		const unsigned long absolute_quotient = absolute_destination_value / absolute_source_value;
		const unsigned long quotient = result_is_negative ? 0 - absolute_quotient : absolute_quotient;

		state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO | CONDITION_CODE_OVERFLOW);

		/* Overflow detection */
		if (absolute_quotient > (instruction == INSTRUCTION_DIVU ? 0xFFFFul : (result_is_negative ? 0x8000ul : 0x7FFFul)))
		{
			state->status_register |= CONDITION_CODE_OVERFLOW;
		}
		else
		{
			const unsigned int absolute_remainder = absolute_destination_value % absolute_source_value;
			const unsigned int remainder = destination_is_negative ? 0 - absolute_remainder : absolute_remainder;

			state->data_registers[opcode.secondary_register] = (unsigned long)(quotient & 0xFFFF) | ((unsigned long)(remainder & 0xFFFF) << 16);
		}

		state->status_register |= CONDITION_CODE_NEGATIVE * ((quotient & 0x8000) != 0);
		state->status_register |= CONDITION_CODE_ZERO * (quotient == 0);
	}
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_EOR:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Secondary data register. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value ^ source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value ^ source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_EORI_TO_CCR:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value ^ source_value;

	/* Write destination operand. */
	/* Write to condition code register */
	state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_EORI_TO_SR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value ^ source_value;

	/* Write destination operand. */
	/* Write to status register */
	state->status_register = (unsigned short)result_value;

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_EXG:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	unsigned long temp;

	switch (opcode.raw & 0x00F8)
	{
		case 0x0040:
			temp = state->data_registers[opcode.secondary_register];
			state->data_registers[opcode.secondary_register] = state->data_registers[opcode.primary_register];
			state->data_registers[opcode.primary_register] = temp;
			break;

		case 0x0048:
			temp = state->address_registers[opcode.secondary_register];
			state->address_registers[opcode.secondary_register] = state->address_registers[opcode.primary_register];
			state->address_registers[opcode.primary_register] = temp;
			break;

		case 0x0088:
			temp = state->data_registers[opcode.secondary_register];
			state->data_registers[opcode.secondary_register] = state->address_registers[opcode.primary_register];
			state->address_registers[opcode.primary_register] = temp;
			break;
	}
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_EXT:
	/* Obtain instruction size. */
	operation_size = opcode.raw & 0x0040 ? 4 : 2;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = CC_SIGN_EXTEND_ULONG((opcode.raw & 0x0040) != 0 ? 15 : 7, destination_value);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ILLEGAL:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	Group1Or2Exception(&stuff, 4);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_JMP:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = DecodeMemoryAddressMode(&stuff, 0, opcode.primary_address_mode, opcode.primary_register);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->program_counter = source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_JSR:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = DecodeMemoryAddressMode(&stuff, 0, opcode.primary_address_mode, opcode.primary_register);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->address_registers[7] -= 4;
	WriteLongWord(&stuff, state->address_registers[7], state->program_counter);

	state->program_counter = source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_LEA:
	/* Obtain instruction size. */
	/* Hardcoded to a longword. */
	operation_size = 4;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Address register (secondary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = DecodeMemoryAddressMode(&stuff, 0, opcode.primary_address_mode, opcode.primary_register);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_LINK:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	/* Push address register to stack */
	state->address_registers[7] -= 4;
	WriteLongWord(&stuff, state->address_registers[7], state->address_registers[opcode.primary_register]);

	/* Copy stack pointer to address register */
	state->address_registers[opcode.primary_register] = state->address_registers[7];

	/* Offset the stack pointer by the immediate value */
	state->address_registers[7] += CC_SIGN_EXTEND_ULONG(15, source_value);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_LSD_MEMORY:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = 1;

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value <<= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_LSD_REGISTER:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = opcode.raw & 0x0020 ? state->data_registers[opcode.secondary_register] % 64 : ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value <<= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVE:
	/* Obtain instruction size. */
	/* Derived from an odd bitfield. */
	switch (opcode.raw & 0x3000)
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

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Secondary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.secondary_address_mode, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MOVE_FROM_SR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = state->status_register;

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVE_TO_CCR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Write to condition code register */
	state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVE_TO_SR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Write to status register */
	state->status_register = (unsigned short)result_value;

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVE_USP:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	if (opcode.raw & 8)
		state->address_registers[opcode.primary_register] = state->user_stack_pointer;
	else
		state->user_stack_pointer = state->address_registers[opcode.primary_register];

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVEA:
	/* Obtain instruction size. */
	/* Derived from an odd bitfield. */
	switch (opcode.raw & 0x3000)
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

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Full secondary address register */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = operation_size == 2 ? CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVEM:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	/* Hot damn is this a mess */
	unsigned long memory_address = DecodeMemoryAddressMode(&stuff, 0, opcode.primary_address_mode, opcode.primary_register);
	unsigned int i;
	unsigned int bitfield;

	int delta;
	void (*write_function)(const Stuff *stuff, unsigned long address, unsigned long value);

	if (opcode.raw & 0x0040)
	{
		delta = 4;
		write_function = WriteLongWord;
	}
	else
	{
		delta = 2;
		write_function = WriteWord;
	}

	if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
		delta = -delta;

	bitfield = source_value;

	/* First group of registers */
	for (i = 0; i < 8; ++i)
	{
		if (bitfield & 1)
		{
			if (opcode.raw & 0x0400)
			{
				/* Memory to register */
				if (opcode.raw & 0x0040)
					state->data_registers[i] = ReadLongWord(&stuff, memory_address);
				else
					state->data_registers[i] = CC_SIGN_EXTEND_ULONG(15, ReadWord(&stuff, memory_address));
			}
			else
			{
				/* Register to memory */
				if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
					write_function(&stuff, memory_address + delta, state->address_registers[7 - i]);
				else
					write_function(&stuff, memory_address, state->data_registers[i]);
			}

			memory_address += delta;
		}

		bitfield >>= 1;
	}

	/* Second group of registers */
	for (i = 0; i < 8; ++i)
	{
		if (bitfield & 1)
		{
			if (opcode.raw & 0x0400)
			{
				/* Memory to register */
				if (opcode.raw & 0x0040)
					state->address_registers[i] = ReadLongWord(&stuff, memory_address);
				else
					state->address_registers[i] = CC_SIGN_EXTEND_ULONG(15, ReadWord(&stuff, memory_address));
			}
			else
			{
				/* Register to memory */
				if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT)
					write_function(&stuff, memory_address + delta, state->data_registers[7 - i]);
				else
					write_function(&stuff, memory_address, state->address_registers[i]);
			}

			memory_address += delta;
		}

		bitfield >>= 1;
	}

	if (opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT || opcode.primary_address_mode == ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_POSTINCREMENT)
		state->address_registers[opcode.primary_register] = memory_address;
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVEP:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	unsigned long memory_address = DecodeMemoryAddressMode(&stuff, 0, ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_DISPLACEMENT, opcode.primary_register);

	switch (opcode.bits_6_and_7)
	{
		case 0:
			/* Memory to register (word) */
			state->data_registers[opcode.secondary_register] &= ~0xFFFFul;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 0) << 8 * 1;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 1) << 8 * 0;
			break;

		case 1:
			/* Memory to register (longword) */
			state->data_registers[opcode.secondary_register] = 0;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 0) << 8 * 3;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 1) << 8 * 2;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 2) << 8 * 1;
			state->data_registers[opcode.secondary_register] |= ReadByte(&stuff, memory_address + 2 * 3) << 8 * 0;
			break;

		case 2:
			/* Register to memory (word) */
			WriteByte(&stuff, memory_address + 2 * 0, (state->data_registers[opcode.secondary_register] >> 8 * 1) & 0xFF);
			WriteByte(&stuff, memory_address + 2 * 1, (state->data_registers[opcode.secondary_register] >> 8 * 0) & 0xFF);
			break;

		case 3:
			/* Register to memory (longword) */
			WriteByte(&stuff, memory_address + 2 * 0, (state->data_registers[opcode.secondary_register] >> 8 * 3) & 0xFF);
			WriteByte(&stuff, memory_address + 2 * 1, (state->data_registers[opcode.secondary_register] >> 8 * 2) & 0xFF);
			WriteByte(&stuff, memory_address + 2 * 2, (state->data_registers[opcode.secondary_register] >> 8 * 1) & 0xFF);
			WriteByte(&stuff, memory_address + 2 * 3, (state->data_registers[opcode.secondary_register] >> 8 * 0) & 0xFF);
			break;
	}
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MOVEQ:
	/* Obtain instruction size. */
	/* Hardcoded to a longword. */
	operation_size = 4;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (secondary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = CC_SIGN_EXTEND_ULONG(7, opcode.raw);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_MULS:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	const cc_bool multiplier_is_negative = instruction == INSTRUCTION_MULS && source_value & 0x8000;
	const cc_bool multiplicand_is_negative = instruction == INSTRUCTION_MULS && state->data_registers[opcode.secondary_register] & 0x8000;
	const cc_bool result_is_negative = multiplier_is_negative != multiplicand_is_negative;

	const unsigned int multiplier = multiplier_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;
	const unsigned int multiplicand = multiplicand_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFF;

	const unsigned long absolute_result = (unsigned long)multiplicand * multiplier;
	const unsigned long result = result_is_negative ? 0 - absolute_result : absolute_result;

	state->data_registers[opcode.secondary_register] = result;

	state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result & 0x80000000) != 0);
	state->status_register |= CONDITION_CODE_ZERO * (result == 0);
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_MULU:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	{
	const cc_bool multiplier_is_negative = instruction == INSTRUCTION_MULS && source_value & 0x8000;
	const cc_bool multiplicand_is_negative = instruction == INSTRUCTION_MULS && state->data_registers[opcode.secondary_register] & 0x8000;
	const cc_bool result_is_negative = multiplier_is_negative != multiplicand_is_negative;

	const unsigned int multiplier = multiplier_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, source_value) : source_value;
	const unsigned int multiplicand = multiplicand_is_negative ? 0 - CC_SIGN_EXTEND_ULONG(15, state->data_registers[opcode.secondary_register]) : state->data_registers[opcode.secondary_register] & 0xFFFF;

	const unsigned long absolute_result = (unsigned long)multiplicand * multiplier;
	const unsigned long result = result_is_negative ? 0 - absolute_result : absolute_result;

	state->data_registers[opcode.secondary_register] = result;

	state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result & 0x80000000) != 0);
	state->status_register |= CONDITION_CODE_ZERO * (result == 0);
	}

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_NBCD:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("NBCD");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NEG:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = 0 - destination_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * (dm || rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NEGX:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("NEGX");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	state->status_register |= CONDITION_CODE_CARRY * (dm || rm);
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	state->status_register |= CONDITION_CODE_OVERFLOW * (dm && rm);
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_NOP:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	/* Doesn't do anything */

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_NOT:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = ~destination_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_OR:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode or secondary data register, based on direction bit. */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);
	else
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Primary address mode or secondary data register, based on direction bit */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);
	else
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value | source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value | source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_ORI_TO_CCR:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value | source_value;

	/* Write destination operand. */
	/* Write to condition code register */
	state->status_register = (result_value & 0xFF) | (state->status_register & 0xFF00);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ORI_TO_SR:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Read from status register */
	destination_value = state->status_register;

	/* Do the actual instruction. */
	result_value = destination_value | source_value;

	/* Write destination operand. */
	/* Write to status register */
	state->status_register = (unsigned short)result_value;

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_PEA:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = DecodeMemoryAddressMode(&stuff, 0, opcode.primary_address_mode, opcode.primary_register);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->address_registers[7] -= 4;
	WriteLongWord(&stuff, state->address_registers[7], source_value);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_RESET:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("RESET");

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ROD_MEMORY:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = 1;

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value = (result_value << 1) | (1 * ((result_value & sign_bit_bitmask) != 0));
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value = (result_value >> 1) | (sign_bit_bitmask * ((result_value & 1) != 0));
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ROD_REGISTER:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = opcode.raw & 0x0020 ? state->data_registers[opcode.secondary_register] % 64 : ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value = (result_value << 1) | (1 * ((result_value & sign_bit_bitmask) != 0));
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value = (result_value >> 1) | (sign_bit_bitmask * ((result_value & 1) != 0));
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ROXD_MEMORY:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = 1;

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value <<= 1;
			result_value |= 1 * ((state->status_register & CONDITION_CODE_EXTEND) != 0);

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;
			result_value |= sign_bit_bitmask * ((state->status_register & CONDITION_CODE_EXTEND) != 0);

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_ROXD_REGISTER:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	{
	const unsigned long sign_bit_bitmask = 1ul << (operation_size * 8 - 1);
	unsigned int i;
	unsigned int count;

	result_value = destination_value;

	count = opcode.raw & 0x0020 ? state->data_registers[opcode.secondary_register] % 64 : ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8 */

	state->status_register &= ~(CONDITION_CODE_OVERFLOW | CONDITION_CODE_CARRY);

	if (opcode.bit_8)
	{
		/* Left */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & sign_bit_bitmask) != 0);

			result_value <<= 1;
			result_value |= 1 * ((state->status_register & CONDITION_CODE_EXTEND) != 0);

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	else
	{
		/* Right */
		for (i = 0; i < count; ++i)
		{
			state->status_register &= ~CONDITION_CODE_CARRY;
			state->status_register |= CONDITION_CODE_CARRY * ((result_value & 1) != 0);

			result_value >>= 1;
			result_value |= sign_bit_bitmask * ((state->status_register & CONDITION_CODE_EXTEND) != 0);

			state->status_register &= ~CONDITION_CODE_EXTEND;
			state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);
		}
	}
	}

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_RTE:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->status_register = (unsigned short)ReadWord(&stuff, state->address_registers[7]);
	state->address_registers[7] += 2;
	state->program_counter = ReadLongWord(&stuff, state->address_registers[7]);
	state->address_registers[7] += 4;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_RTR:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->status_register &= 0xFF00;
	state->status_register |= ReadByte(&stuff, state->address_registers[7] + 1);
	state->address_registers[7] += 2;
	state->program_counter = ReadLongWord(&stuff, state->address_registers[7]);
	state->address_registers[7] += 4;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_RTS:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->program_counter = ReadLongWord(&stuff, state->address_registers[7]);
	state->address_registers[7] += 4;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_SBCD:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("SBCD");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Undefined */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Undefined */
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SCC:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* For some reason, this instruction reads from its destination even though it doesn't use it. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = IsOpcodeConditionTrue(state, opcode.raw) ? 0xFF : 0;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_STOP:
	/* Obtain instruction size. */
	/* Hardcoded to a word. */
	operation_size = 2;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("STOP");

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_SUB:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode or secondary data register, based on direction bit. */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);
	else
		DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Primary address mode or secondary data register, based on direction bit */
	if (opcode.bit_8)
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);
	else
		DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBA:
	/* Obtain instruction size. */
	/* Word or longword based on bit 8. */
	operation_size = opcode.bit_8 ? 4 : 2;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Full secondary address register */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, 4, ADDRESS_MODE_ADDRESS_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	if (!opcode.bit_8)
		source_value = CC_SIGN_EXTEND_ULONG(15, source_value);

	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_SUBAQ:
	/* Obtain instruction size. */
	/* Hardcoded to a longword. */
	operation_size = 4;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_SUBI:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Immediate value (any size). */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBQ:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	source_value = ((opcode.secondary_register - 1u) & 7u) + 1u; /* A little math trick to turn 0 into 8. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = destination_value - source_value;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SUBX:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Decode destination address mode. */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.raw & 0x0008 ? ADDRESS_MODE_ADDRESS_REGISTER_INDIRECT_WITH_PREDECREMENT : ADDRESS_MODE_DATA_REGISTER, opcode.secondary_register);

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	UNIMPLEMENTED_INSTRUCTION("SUBX");

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
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
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	state->status_register &= ~CONDITION_CODE_EXTEND;
	state->status_register |= CONDITION_CODE_EXTEND * ((state->status_register & CONDITION_CODE_CARRY) != 0);

	break;

case INSTRUCTION_SWAP:
	/* Obtain instruction size. */
	/* Hardcoded to a longword. */
	operation_size = 4;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Data register (primary) */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, ADDRESS_MODE_DATA_REGISTER, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	result_value = ((destination_value & 0x0000FFFF) << 16) | ((destination_value & 0xFFFF0000) >> 16);

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TAS:
	/* Obtain instruction size. */
	/* Hardcoded to a byte. */
	operation_size = 1;

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Using primary address mode */
	DecodeAddressMode(&stuff, &destination_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	destination_value = GetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode);

	/* Do the actual instruction. */
	state->status_register &= ~(CONDITION_CODE_NEGATIVE | CONDITION_CODE_ZERO);
	state->status_register |= CONDITION_CODE_NEGATIVE * ((destination_value & 0x80) != 0);
	state->status_register |= CONDITION_CODE_ZERO * (destination_value == 0);

	result_value = destination_value | 0x80;

	/* Write destination operand. */
	/* Write to destination */
	SetValueUsingDecodedAddressMode(&stuff, &destination_decoded_address_mode, result_value);

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_TRAP:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't need an address mode for its source. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = opcode.raw & 0xF;

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	Group1Or2Exception(&stuff, 32 + source_value);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_TRAPV:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	if (state->status_register & CONDITION_CODE_OVERFLOW)
		Group1Or2Exception(&stuff, 32 + 7);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_TST:
	/* Obtain instruction size. */
	/* Standard. */
	operation_size = 1 << opcode.bits_6_and_7;

	/* Decode source address mode. */
	/* Primary address mode. */
	DecodeAddressMode(&stuff, &source_decoded_address_mode, operation_size, opcode.primary_address_mode, opcode.primary_register);

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	source_value = GetValueUsingDecodedAddressMode(&stuff, &source_decoded_address_mode);

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	result_value = source_value;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	state->status_register &= ~CONDITION_CODE_CARRY;
	/* Update OVERFLOW condition code */
	state->status_register &= ~CONDITION_CODE_OVERFLOW;
	/* Update ZERO condition code */
	state->status_register &= ~CONDITION_CODE_ZERO;
	state->status_register |= CONDITION_CODE_ZERO * ((result_value & (0xFFFFFFFF >> (32 - operation_size * 8))) == 0);
	/* Update NEGATIVE condition code */
	state->status_register &= ~CONDITION_CODE_NEGATIVE;
	state->status_register |= CONDITION_CODE_NEGATIVE * ((result_value & msb_mask) != 0);
	/* Update EXTEND condition code */
	/* Unaffected */

	break;

case INSTRUCTION_UNLK:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	state->address_registers[7] = state->address_registers[opcode.primary_register];
	state->address_registers[opcode.primary_register] = ReadLongWord(&stuff, state->address_registers[7]);
	state->address_registers[7] += 4;

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_UNIMPLEMENTED_1:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	Group1Or2Exception(&stuff, 10);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

case INSTRUCTION_UNIMPLEMENTED_2:
	/* Obtain instruction size. */
	/* Doesn't have a size. */

	/* Decode source address mode. */
	/* Doesn't have a source address mode to decode. */

	/* Decode destination address mode. */
	/* Doesn't have a destination address mode to decode. */

	/* Read source operand. */
	/* Doesn't have a source value. */

	/* Read destination operand. */
	/* Doesn't read its destination (if it even has one) */

	/* Do the actual instruction. */
	Group1Or2Exception(&stuff, 11);

	/* Write destination operand. */
	/* Doesn't write anything */

	/* Update condition codes. */
	msb_mask = 1ul << (operation_size * 8 - 1);
	sm = (source_value & msb_mask) != 0;
	dm = (destination_value & msb_mask) != 0;
	rm = (result_value & msb_mask) != 0;

	/* Update CARRY condition code */
	/* Update OVERFLOW condition code */
	/* Update ZERO condition code */
	/* Update NEGATIVE condition code */
	/* Update EXTEND condition code */

	break;

