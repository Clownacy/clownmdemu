#include "bus-sub-m68k.h"

#include <assert.h>

static cc_u8f To2DigitBCD(const cc_u8f value)
{
	const cc_u8f lower_digit = value % 10;
	const cc_u8f upper_digit = (value / 10) % 10;
	return (upper_digit << 4) | (lower_digit << 0);
}

static cc_u32f GetCDSectorHeader(const ClownMDEmu* const clownmdemu)
{
	const cc_u32f frames = To2DigitBCD(clownmdemu->state->mega_cd.cd.current_sector % 75);
	const cc_u32f seconds = To2DigitBCD((clownmdemu->state->mega_cd.cd.current_sector / 75) % 60);
	const cc_u32f minutes = To2DigitBCD(clownmdemu->state->mega_cd.cd.current_sector / (75 * 60));

	return ((cc_u32f)0x01 << (8 * 0))
	     | (frames  << (8 * 1))
	     | (seconds << (8 * 2))
	     | (minutes << (8 * 3));
}

static cc_u16f MCDM68kReadWord(const void* const user_data, const cc_u32f address, const cc_u32f target_cycle)
{
	assert(address % 2 == 0);

	return MCDM68kReadCallbackWithCycle(user_data, (address & 0xFFFFFF) / 2, cc_true, cc_true, target_cycle);
}

static cc_u16f MCDM68kReadLongword(const void* const user_data, const cc_u32f address, const cc_u32f target_cycle)
{
	cc_u32f longword;
	longword = (cc_u32f)MCDM68kReadWord(user_data, address + 0, target_cycle) << 16;
	longword |= (cc_u32f)MCDM68kReadWord(user_data, address + 2, target_cycle) << 0;
	return longword;
}

static void MCDM68kWriteWord(const void* const user_data, const cc_u32f address, const cc_u16f value, const cc_u32f target_cycle)
{
	assert(address % 2 == 0);

	MCDM68kWriteCallbackWithCycle(user_data, (address & 0xFFFFFF) / 2, cc_true, cc_true, value, target_cycle);
}

static void MCDM68kWriteLongword(const void* const user_data, const cc_u32f address, const cc_u32f value, const cc_u32f target_cycle)
{
	assert(value <= 0xFFFFFFFF);
	MCDM68kWriteWord(user_data, address + 0, value >> 16, target_cycle);
	MCDM68kWriteWord(user_data, address + 2, value & 0xFFFF, target_cycle);
}

/* TODO: Move this to its own file? */
static void MegaCDBIOSCall(const ClownMDEmu* const clownmdemu, const void* const user_data, const ClownMDEmu_Callbacks* const frontend_callbacks, const cc_u32f target_cycle)
{
	/* TODO: None of this shit is accurate at all. */
	const cc_u16f command = clownmdemu->mcd_m68k->data_registers[0] & 0xFFFF;

	switch (command)
	{
		case 0x20:
		{
			/* ROMREADN */
			const cc_u32f starting_sector = MCDM68kReadLongword(user_data, clownmdemu->mcd_m68k->address_registers[0] + 0, target_cycle);
			const cc_u32f total_sectors = MCDM68kReadLongword(user_data, clownmdemu->mcd_m68k->address_registers[0] + 4, target_cycle);

			frontend_callbacks->cd_seeked((void*)frontend_callbacks->user_data, starting_sector);
			clownmdemu->state->mega_cd.cd.current_sector = starting_sector;
			clownmdemu->state->mega_cd.cd.total_buffered_sectors = total_sectors;
			break;
		}

		case 0x8A:
			/* CDCSTAT */
			clownmdemu->mcd_m68k->status_register &= ~1; /* Clear carry flag to signal that there's always a sector ready. */
			break;

		case 0x8B:
			/* CDCREAD */
			if (clownmdemu->state->mega_cd.cd.total_buffered_sectors == 0)
			{
				/* Sonic Megamix 4.0b relies on this. */
				clownmdemu->mcd_m68k->status_register |= 1; /* Set carry flag to signal that a sector has not been prepared. */
			}
			else
			{
				--clownmdemu->state->mega_cd.cd.total_buffered_sectors;
				clownmdemu->state->mega_cd.cd.cdc_ready = cc_true;

				clownmdemu->mcd_m68k->status_register &= ~1; /* Clear carry flag to signal that a sector has been prepared. */
				clownmdemu->mcd_m68k->data_registers[0] = GetCDSectorHeader(clownmdemu);
			}

			break;

		case 0x8C:
		{
			/* CDCTRN */
			if (!clownmdemu->state->mega_cd.cd.cdc_ready)
			{
				clownmdemu->mcd_m68k->status_register |= 1; /* Set carry flag to signal that there's not a sector ready. */
			}
			else
			{
				cc_u32f i;
				const cc_u8l* const sector_bytes = frontend_callbacks->cd_sector_read((void*)frontend_callbacks->user_data);
				const cc_u32f sector_header = GetCDSectorHeader(clownmdemu);

				clownmdemu->state->mega_cd.cd.cdc_ready = cc_false;
				++clownmdemu->state->mega_cd.cd.current_sector;

				for (i = 0; i < 0x800; i += 2)
				{
					const cc_u32f address = clownmdemu->mcd_m68k->address_registers[0] + i;
					const cc_u16f sector_word = ((cc_u16f)sector_bytes[i + 0] << 8) | ((cc_u16f)sector_bytes[i + 1] << 0);

					MCDM68kWriteWord(user_data, address, sector_word, target_cycle);
				}

				MCDM68kWriteLongword(user_data, clownmdemu->mcd_m68k->address_registers[1], sector_header, target_cycle);

				clownmdemu->mcd_m68k->address_registers[0] = (clownmdemu->mcd_m68k->address_registers[0] + 0x800) & 0xFFFFFFFF;
				clownmdemu->mcd_m68k->address_registers[1] = (clownmdemu->mcd_m68k->address_registers[1] + 4) & 0xFFFFFFFF;
				clownmdemu->mcd_m68k->status_register &= ~1; /* Clear carry flag to signal that there's always a sector ready. */
			}

			break;
		}

		case 0x8D:
			/* CDCACK */
			/* TODO: Anything. */
			break;

		default:
			PrintError("UNRECOGNISED BIOS CALL DETECTED (0x%02" CC_PRIXFAST16 ")", command);
			break;
	}
}

void SyncMCDM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	cc_u16f m68k_countdown;

	m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	m68k_read_write_callbacks.user_data = other_state;

	/* Store this in a local variable to make the upcoming code faster. */
	m68k_countdown = clownmdemu->state->mega_cd.m68k.cycle_countdown;

	while (other_state->mcd_m68k_current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(m68k_countdown, target_cycle - other_state->mcd_m68k_current_cycle);

		assert(target_cycle >= other_state->mcd_m68k_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

		m68k_countdown -= cycles_to_do;

		if (m68k_countdown == 0)
		{
			if (!clownmdemu->state->mega_cd.m68k.bus_requested && !clownmdemu->state->mega_cd.m68k.reset_held)
				Clown68000_DoCycle(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);

			m68k_countdown = CLOWNMDEMU_MCD_M68K_CLOCK_DIVIDER * 10; /* TODO: The '* 10' is a temporary hack until 68000 instruction durations are added. */
			/* TODO: Handle the MCD's master clock! */
		}

		other_state->mcd_m68k_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->mega_cd.m68k.cycle_countdown = m68k_countdown;
}

cc_u16f MCDM68kReadCallbackWithCycle(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;
	cc_u16f value = 0;

	(void)do_high_byte;
	(void)do_low_byte;

	if (/*address >= 0 &&*/ address < 0x80000 / 2)
	{
		/* PRG-RAM */
		if (address == 0x5F16 / 2 && clownmdemu->mcd_m68k->program_counter == 0x5F16)
		{
			/* BRAM call! */
			/* TODO: None of this shit is accurate at all. */
			const cc_u16f command = clownmdemu->mcd_m68k->data_registers[0] & 0xFFFF;

			switch (command)
			{
				case 0x00:
					/* BRMINIT */
					clownmdemu->mcd_m68k->status_register &= ~1; /* Formatted RAM is present. */
					/* Size of Backup RAM. */
					clownmdemu->mcd_m68k->data_registers[0] &= 0xFFFF0000;
					clownmdemu->mcd_m68k->data_registers[0] |= 0x100; /* Maximum officially-allowed size. */
					/* "Display strings". */
					/*clownmdemu->mcd_m68k->address_registers[1] = I have no idea; */
					break;

				case 0x01:
					/* BRMSTAT */
					clownmdemu->mcd_m68k->data_registers[0] &= 0xFFFF0000;
					clownmdemu->mcd_m68k->data_registers[1] &= 0xFFFF0000;
					break;

				case 0x02:
					/* BRMSERCH */
					clownmdemu->mcd_m68k->status_register |= 1; /* File not found */
					break;

				case 0x03:
					/* BRMREAD */
					clownmdemu->mcd_m68k->status_register &= ~1; /* Okay */
					clownmdemu->mcd_m68k->data_registers[0] &= 0xFFFF0000;
					clownmdemu->mcd_m68k->data_registers[1] &= 0xFFFFFF00;
					break;

				case 0x04:
					/* BRMWRITE */
					clownmdemu->mcd_m68k->status_register |= 1; /* Error */
					break;

				case 0x05:
					/* BRMDEL */
					clownmdemu->mcd_m68k->status_register &= ~1; /* Okay */
					break;

				case 0x06:
					/* BRMFORMAT */
					clownmdemu->mcd_m68k->status_register &= ~1; /* Okay */
					break;

				case 0x07:
					/* BRMDIR */
					clownmdemu->mcd_m68k->status_register |= 1; /* Error */
					break;

				case 0x08:
					/* BRMVERIFY */
					clownmdemu->mcd_m68k->status_register &= ~1; /* Okay */
					break;

				default:
					PrintError("UNRECOGNISED BRAM CALL DETECTED (0x%02" CC_PRIXFAST16 ")", command);
					break;
			}

			value = 0x4E75; /* 'rts' instruction */
		}
		else if (address == 0x5F22 / 2 && clownmdemu->mcd_m68k->program_counter == 0x5F22)
		{
			static void MegaCDBIOSCall(const ClownMDEmu* const clownmdemu, const void* const user_data, const ClownMDEmu_Callbacks* const frontend_callbacks, const cc_u32f target_cycle);

			/* BIOS call! */
			MegaCDBIOSCall(clownmdemu, user_data, frontend_callbacks, target_cycle);

			value = 0x4E75; /* 'rts' instruction */
		}
		else
		{
			value = clownmdemu->state->mega_cd.prg_ram.buffer[address];
		}
	}
	else if (address < 0xC0000 / 2)
	{
		/* WORD-RAM */
		if (clownmdemu->state->mega_cd.word_ram.in_1m_mode)
		{
			/* TODO. */
			PrintError("SUB-CPU attempted to read from the weird half of 1M WORD-RAM at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else if (!clownmdemu->state->mega_cd.word_ram.dmna)
		{
			/* TODO: According to Page 24 of MEGA-CD HARDWARE MANUAL, this should cause the CPU to hang, just like the Z80 accessing the ROM during a DMA transfer. */
			PrintError("SUB-CPU attempted to read from WORD-RAM while MAIN-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else
		{
			value = clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF];
		}
	}
	else if (address < 0xE0000 / 2)
	{
		/* WORD-RAM */
		if (!clownmdemu->state->mega_cd.word_ram.in_1m_mode)
		{
			/* TODO. */
			PrintError("SUB-CPU attempted to read from the 1M half of WORD-RAM in 2M mode at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else
		{
			value = clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + !clownmdemu->state->mega_cd.word_ram.ret];
		}
	}
	else if (address >= 0xFF0000 / 2 && address < 0xFF8000 / 2)
	{
		if ((address & 0x1000) != 0)
		{
			/* PCM wave RAM */
			PrintError("SUB-CPU attempted to read from PCM wave RAM at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
		}
		else
		{
			/* PCM register */
			SyncMCDPCM(callback_user_data, target_cycle);
			value = (cc_u16f)PCM_ReadRegister(&clownmdemu->pcm, address & 0xFFF);
		}
	}
	else if (address == 0xFF8002 / 2)
	{
		/* Memory mode / Write protect */
		value = ((cc_u16f)clownmdemu->state->mega_cd.word_ram.in_1m_mode << 2) | ((cc_u16f)clownmdemu->state->mega_cd.word_ram.dmna << 1) | ((cc_u16f)clownmdemu->state->mega_cd.word_ram.ret << 0);
	}
	else if (address == 0xFF8004 / 2)
	{
		/* CDC mode / device destination */
		value = 0x4000;
	}
	else if (address == 0xFF8006 / 2)
	{
		/* H-INT vector */
		PrintError("SUB-CPU attempted to read from H-INT vector register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF8008 / 2)
	{
		/* CDC host data */
		PrintError("SUB-CPU attempted to read from CDC host data register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF800C / 2)
	{
		/* Stop watch */
		PrintError("SUB-CPU attempted to read from stop watch register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF800E / 2)
	{
		/* Communication flag */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.flag;
	}
	else if (address >= 0xFF8010 / 2 && address < 0xFF8020 / 2)
	{
		/* Communication command */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.command[address - 0xFF8010 / 2];
	}
	else if (address >= 0xFF8020 / 2 && address < 0xFF8030 / 2)
	{
		/* Communication status */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.status[address - 0xFF8020 / 2];
	}
	else if (address == 0xFF8030 / 2)
	{
		/* Timer W/INT3 */
		PrintError("SUB-CPU attempted to read from Timer W/INT3 register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF8032 / 2)
	{
		/* Interrupt mask control */
		cc_u8f i;

		value = 0;

		for (i = 0; i < CC_COUNT_OF(clownmdemu->state->mega_cd.irq.enabled); ++i)
			value |= (cc_u16f)clownmdemu->state->mega_cd.irq.enabled[i] << (1 + i);
	}
	else if (address == 0xFF8058 / 2)
	{
		/* Stamp data size */
		/* TODO */
		value = 0;
	}
	else if (address == 0xFF8064 / 2)
	{
		/* Image buffer vertical draw size */
		/* TODO */
		value = 0;
	}
	else if (address == 0xFF8066 / 2)
	{
		/* Trace vector base address */
		/* TODO */
	}
	else
	{
		PrintError("Attempted to read invalid MCD 68k address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->mcd_m68k->program_counter);
	}

	return value;
}

cc_u16f MCDM68kReadCallback(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	return MCDM68kReadCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, callback_user_data->mcd_m68k_current_cycle);
}

void MCDM68kWriteCallbackWithCycle(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u16f value, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	cc_u16f mask = 0;

	if (do_high_byte)
		mask |= 0xFF00;
	if (do_low_byte)
		mask |= 0x00FF;

	if (/*address >= 0 &&*/ address < 0x80000 / 2)
	{
		/* PRG-RAM */
		clownmdemu->state->mega_cd.prg_ram.buffer[address] &= ~mask;
		clownmdemu->state->mega_cd.prg_ram.buffer[address] |= value & mask;
	}
	else if (address < 0xC0000 / 2)
	{
		/* WORD-RAM */
		if (clownmdemu->state->mega_cd.word_ram.in_1m_mode)
		{
			/* TODO. */
			PrintError("SUB-CPU attempted to write to the weird half of 1M WORD-RAM at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else if (!clownmdemu->state->mega_cd.word_ram.dmna)
		{
			PrintError("SUB-CPU attempted to write to WORD-RAM while MAIN-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else
		{
			clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF] &= ~mask;
			clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF] |= value & mask;
		}
	}
	else if (address < 0xE0000 / 2)
	{
		/* WORD-RAM */
		if (!clownmdemu->state->mega_cd.word_ram.in_1m_mode)
		{
			/* TODO. */
			PrintError("SUB-CPU attempted to write to the 1M half of WORD-RAM in 2M mode at 0x%" CC_PRIXLEAST32, clownmdemu->state->mega_cd.m68k.state.program_counter);
		}
		else
		{
			clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + !clownmdemu->state->mega_cd.word_ram.ret] &= ~mask;
			clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + !clownmdemu->state->mega_cd.word_ram.ret] |= value & mask;
		}
	}
	else if (address >= 0xFF0000 / 2 && address < 0xFF8000 / 2)
	{
		if (do_low_byte)
		{
			SyncMCDPCM(callback_user_data, target_cycle);

			if ((address & 0x1000) != 0)
			{
				/* PCM wave RAM */
				PCM_WriteWaveRAM(&clownmdemu->pcm, address & 0xFFF, (cc_u8f)value);
			}
			else
			{
				/* PCM register */
				PCM_WriteRegister(&clownmdemu->pcm, address & 0xFFF, (cc_u8f)value);
			}
		}
	}
	else if (address == 0xFF8002 / 2)
	{
		/* Memory mode / Write protect */
		if (do_low_byte)
		{
			const cc_bool ret = (value & (1 << 0)) != 0;

			SyncM68k(clownmdemu, callback_user_data, target_cycle);

			clownmdemu->state->mega_cd.word_ram.in_1m_mode = (value & (1 << 2)) != 0;

			if (ret || clownmdemu->state->mega_cd.word_ram.in_1m_mode)
			{
				clownmdemu->state->mega_cd.word_ram.dmna = cc_false;
				clownmdemu->state->mega_cd.word_ram.ret = ret;
			}
		}
	}
	else if (address == 0xFF8004 / 2)
	{
		/* CDC mode / device destination */
		PrintError("SUB-CPU attempted to write to CDC mode/destination register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF8006 / 2)
	{
		/* H-INT vector */
		PrintError("SUB-CPU attempted to write to H-INT vector register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF8008 / 2)
	{
		/* CDC host data */
		PrintError("SUB-CPU attempted to write to CDC host data register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF800C / 2)
	{
		/* Stop watch */
		PrintError("SUB-CPU attempted to write to stop watch register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF800E / 2)
	{
		/* Communication flag */
		if (do_high_byte)
			PrintError("SUB-CPU attempted to write to MAIN-CPU's communication flag at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);

		if (do_low_byte)
		{
			SyncM68k(clownmdemu, callback_user_data, target_cycle);
			clownmdemu->state->mega_cd.communication.flag = (clownmdemu->state->mega_cd.communication.flag & 0xFF00) | (value & 0x00FF);
		}
	}
	else if (address >= 0xFF8010 / 2 && address < 0xFF8020 / 2)
	{
		/* Communication command */
		PrintError("SUB-CPU attempted to write to MAIN-CPU's communication command at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address >= 0xFF8020 / 2 && address < 0xFF8030 / 2)
	{
		/* Communication status */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		clownmdemu->state->mega_cd.communication.status[address - 0xFF8020 / 2] &= ~mask;
		clownmdemu->state->mega_cd.communication.status[address - 0xFF8020 / 2] |= value & mask;
	}
	else if (address == 0xFF8030 / 2)
	{
		/* Timer W/INT3 */
		PrintError("SUB-CPU attempted to write to Timer W/INT3 register at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address == 0xFF8032 / 2)
	{
		/* Interrupt mask control */
		if (do_low_byte)
		{
			cc_u8f i;

			for (i = 0; i < CC_COUNT_OF(clownmdemu->state->mega_cd.irq.enabled); ++i)
				clownmdemu->state->mega_cd.irq.enabled[i] = (value & (1 << (1 + i))) != 0;

			if (!clownmdemu->state->mega_cd.irq.enabled[0])
				clownmdemu->state->mega_cd.irq.irq1_pending = cc_false;
		}
	}
	else if (address == 0xFF8058 / 2)
	{
		/* Stamp data size */
		/* TODO */
	}
	else if (address == 0xFF8064 / 2)
	{
		/* Image buffer vertical draw size */
		/* TODO */
	}
	else if (address == 0xFF8066 / 2)
	{
		/* Trace vector base address */
		/* TODO */
		if (clownmdemu->state->mega_cd.irq.enabled[0])
			clownmdemu->state->mega_cd.irq.irq1_pending = cc_true;
	}
	else
	{
		PrintError("Attempted to write invalid MCD 68k address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->mcd_m68k->program_counter);
	}
}

void MCDM68kWriteCallback(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	MCDM68kWriteCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, value, callback_user_data->mcd_m68k_current_cycle);
}
