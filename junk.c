		/* Figure out the operation size */
		switch (instruction)
		{
			case INSTRUCTION_ORI_TO_CCR:
			case INSTRUCTION_ORI_TO_SR:
			case INSTRUCTION_ORI:
			case INSTRUCTION_ANDI_TO_CCR:
			case INSTRUCTION_ANDI_TO_SR:
			case INSTRUCTION_ANDI:
			case INSTRUCTION_SUBBI:
			case INSTRUCTION_ADDI:
			case INSTRUCTION_EORI_TO_CCR:
			case INSTRUCTION_EORI_TO_SR:
			case INSTRUCTION_EORI:
			case INSTRUCTION_CMPI:
			case INSTRUCTION_NEGX:
			case INSTRUCTION_CLR:
			case INSTRUCTION_NEG:
			case INSTRUCTION_NOT:
			case INSTRUCTION_TST:
			case INSTRUCTION_ADDQ:
			case INSTRUCTION_SUBQ:
			case INSTRUCTION_OR:
			case INSTRUCTION_SUB:
			case INSTRUCTION_SUBX:
			case INSTRUCTION_EOR:
			case INSTRUCTION_CMPM:
			case INSTRUCTION_CMP:
			case INSTRUCTION_AND:
			case INSTRUCTION_ADD:
			case INSTRUCTION_ADDX:
			case INSTRUCTION_ASD_REGISTER:
			case INSTRUCTION_LSD_REGISTER:
			case INSTRUCTION_ROXD_REGISTER:
			case INSTRUCTION_ROD_REGISTER:
				/* Derived from the 'standard' operation size bits */
				operation_size = 1 << opcode_bits_6_and_7;
				break;

			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE:
				/* Derived from an odd bitfield */
				switch (opcode & 0x3000)
				{
					case 0x1000:
						operation_size = 1;
						break;

					case 0x2000:
						operation_size = 4; /* Yup, this isn't a typo */
						break;

					case 0x3000:
						operation_size = 2;
						break;
				}
				break;

			case INSTRUCTION_MOVEP:
			case INSTRUCTION_EXT:
			case INSTRUCTION_MOVEM:
				/* Derived from bit 6 */
				operation_size = opcode & 0x0040 ? 4 : 2;
				break;

			case INSTRUCTION_SUBA:
			case INSTRUCTION_CMPA:
			case INSTRUCTION_ADDA:
				/* Derived from bit 8 */
				operation_size = opcode_bit_8 ? 4 : 2;
				break;

			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
				operation_size = opcode_primary_address_mode == ADDRESS_MODE_DATA_REGISTER ? 4 : 1;
				break;

			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_NBCD:
			case INSTRUCTION_TAS:
				/* Hardcoded to a byte */
				operation_size = 1;
				break;

			case INSTRUCTION_MOVE_FROM_SR:
			case INSTRUCTION_MOVE_TO_SR:
			case INSTRUCTION_LINK:
			case INSTRUCTION_CHK:
				/* Hardcoded to a word */
				operation_size = 2;
				break;

			case INSTRUCTION_SWAP:
			case INSTRUCTION_PEA:
			case INSTRUCTION_MOVE_USP:
			case INSTRUCTION_LEA:
				/* Hardcoded to a longword */
				operation_size = 4;
				break;

			case 
				/* Doesn't need a size specifying here */
				break;
		}

		/* If we couldn't figure out what instruction this is, flag an error */
		/* TODO - Raise an illegal instruction exception instead */
		if (instruction == INSTRUCTION_UNKNOWN)
			PrintError("Unknown instruction encountered around address 0x%X", state->program_counter);

		/* Fetch first value (if needed) */
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
				/* Hardcoded immediate data */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size_in_bytes, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_BTST_STATIC:
			case INSTRUCTION_BCHG_STATIC:
			case INSTRUCTION_BCLR_STATIC:
			case INSTRUCTION_BSET_STATIC:
				/* Hardcoded immediate data with hardcoded size (byte) */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 1, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_BTST_DYNAMIC:
			case INSTRUCTION_BCHG_DYNAMIC:
			case INSTRUCTION_BCLR_DYNAMIC:
			case INSTRUCTION_BSET_DYNAMIC:
				/* Hardcoded data register with hardcoded size (byte) */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 1, ADDRESS_MODE_DATA_REGISTER, opcode_secondary_register);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVEP:
				/* Custom MOVEP behaviour */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, opcode & 0x0040 ? 4 : 2, opcode & 0x0080 ? ADDRESS_MODE_DATA_REGISTER : ADDRESS_MODE_ADDRESS_REGISTER, opcode & 0x0080 ? opcode_secondary_register : opcode_primary_register);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVEA:
			case INSTRUCTION_MOVE:
				/* Custom MOVE/MOVEA behaviour */
				
				break;

			case INSTRUCTION_MOVE_TO_CCR:
			case INSTRUCTION_MOVE_TO_SR:
				/* Standard: use opcode's primary address mode and size */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, operation_size_in_bytes, opcode_primary_address_mode, opcode_primary_register);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVE_FROM_SR:
				/* Custom MOVE FROM SR behaviour */
				value = state->status_register;
				break;

			case INSTRUCTION_PEA:
				/* Custom PEA behaviour */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 4, opcode_primary_address_mode, opcode_primary_register);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			case INSTRUCTION_MOVEM:
				/* Hardcoded immediate data with hardcoded size (word) */
				DecodeAddressMode(state, callbacks, &source_decoded_address_mode, 2, ADDRESS_MODE_SPECIAL, ADDRESS_MODE_REGISTER_SPECIAL_IMMEDIATE);
				value = GetValueUsingDecodedAddressMode(callbacks, &source_decoded_address_mode);
				break;

			default:
				/* These ones don't have any source data */
				break;
		}
