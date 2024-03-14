#include "bus-main-m68k.h"

#include <assert.h>

#include "bus-sub-m68k.h"
#include "bus-z80.h"
#include "error.h"
#include "io-port.h"

#include "mcd-boot.c"

static cc_u16f VDPReadCallback(void *user_data, cc_u32f address)
{
	return M68kReadCallbackWithDMA(user_data, address / 2, cc_true, cc_true, cc_true);
}

void SyncM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	cc_u16f m68k_countdown;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = other_state;

	/* Store this in a local variable to make the upcoming code faster. */
	m68k_countdown = clownmdemu->state->m68k.cycle_countdown;

	while (other_state->m68k_current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(m68k_countdown, target_cycle - other_state->m68k_current_cycle);

		assert(target_cycle >= other_state->m68k_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

		m68k_countdown -= cycles_to_do;

		if (m68k_countdown == 0)
		{
			Clown68000_DoCycle(clownmdemu->m68k, &m68k_read_write_callbacks);
			m68k_countdown = CLOWNMDEMU_M68K_CLOCK_DIVIDER * 10; /* TODO: The '* 10' is a temporary hack until 68000 instruction durations are added. */
		}

		other_state->m68k_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->m68k.cycle_countdown = m68k_countdown;
}

cc_u16f M68kReadCallbackWithCycleWithDMA(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u32f target_cycle, const cc_bool is_vdp_dma)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;
	cc_u16f value = 0;

	if (address < 0x800000 / 2)
	{
		if (((address & 0x200000) == 0) != clownmdemu->state->mega_cd.boot_from_cd)
		{
			/* Cartridge */
			if (do_high_byte)
				value |= frontend_callbacks->cartridge_read((void*)frontend_callbacks->user_data, (address & 0x1FFFFF) * 2 + 0) << 8;
			if (do_low_byte)
				value |= frontend_callbacks->cartridge_read((void*)frontend_callbacks->user_data, (address & 0x1FFFFF) * 2 + 1) << 0;
		}
		else
		{
			if ((address & 0x100000) != 0)
			{
				/* WORD-RAM */
				if (clownmdemu->state->mega_cd.word_ram.in_1m_mode)
				{
					if ((address & 0x10000) != 0)
					{
						/* TODO */
						PrintError("MAIN-CPU attempted to read from that weird half of 1M WORD-RAM at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
					}
					else
					{
						value = clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + clownmdemu->state->mega_cd.word_ram.ret];

						if (is_vdp_dma)
						{
							/* Delay Word RAM DMA transfers. This is a real bug on the Mega CD that games have to work around. */
							/* This can easily be seen in Sonic CD's FMVs. */
							const cc_u16f delayed_value = value;

							value = clownmdemu->state->mega_cd.delayed_dma_word;
							clownmdemu->state->mega_cd.delayed_dma_word = delayed_value;
						}
					}
				}
				else
				{
					if (clownmdemu->state->mega_cd.word_ram.dmna)
					{
						PrintError("MAIN-CPU attempted to read from WORD-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
					}
					else
					{
						value = clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF];
					}
				}
			}
			else if ((address & 0x10000) == 0)
			{
				/* Mega CD BIOS */
				if ((address & 0xFFFF) == 0x72 / 2)
				{
					/* The Mega CD has this strange hack in its bug logic, which allows
					   the H-Int interrupt address to be overridden with a register. */
					value = clownmdemu->state->mega_cd.hblank_address;
				}
				else
				{
					value = megacd_boot_rom[address & 0xFFFF];
				}
			}
			else
			{
				/* PRG-RAM */
				if (!clownmdemu->state->mega_cd.m68k.bus_requested)
				{
					PrintError("MAIN-CPU attempted to read from PRG-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
				}
				else
				{
					value = clownmdemu->state->mega_cd.prg_ram.buffer[0x10000 * clownmdemu->state->mega_cd.prg_ram.bank + (address & 0xFFFF)];
				}
			}
		}
	}
	else if ((address >= 0xA00000 / 2 && address <= 0xA01FFF / 2) || address == 0xA04000 / 2 || address == 0xA04002 / 2)
	{
		/* Z80 RAM and YM2612 */
		if (!clownmdemu->state->z80.bus_requested)
		{
			PrintError("68k attempted to read Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (clownmdemu->state->z80.reset_held)
		{
			/* TODO: Does this actually bother real hardware? */
			/* TODO: According to Devon, yes it does. */
			PrintError("68k attempted to read Z80 memory/YM2612 ports while Z80 reset request was active at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized read of Z80 memory/YM2612 ports at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else
		{
			/* This is unnecessary, as the Z80 bus will have to have been requested, causing a sync. */
			/*SyncZ80(clownmdemu, callback_user_data, target_cycle);*/

			if (do_high_byte)
				value = Z80ReadCallbackWithCycle(user_data, (address * 2 + 0) & 0xFFFF, target_cycle) << 8;
			else /*if (do_low_byte)*/
				value = Z80ReadCallbackWithCycle(user_data, (address * 2 + 1) & 0xFFFF, target_cycle) << 0;
		}
	}
	else if (address >= 0xA10000 / 2 && address <= 0xA1001F / 2)
	{
		/* I/O AREA */
		/* TODO */
		switch (address)
		{
			case 0xA10000 / 2:
				if (do_low_byte)
					value |= ((clownmdemu->configuration->general.region == CLOWNMDEMU_REGION_OVERSEAS) << 7) | ((clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL) << 6) | (0 << 5);	/* Bit 5 clear = Mega CD attached */

				break;

			case 0xA10002 / 2:
			case 0xA10004 / 2:
				if (do_low_byte)
				{
					IOPortToController_Parameters parameters;

					const cc_u16f joypad_index = address - 0xA10002 / 2;

					parameters.controller = &clownmdemu->state->controllers[joypad_index];
					parameters.frontend_callbacks = frontend_callbacks;

					value = IOPort_ReadData(&clownmdemu->state->io_ports[joypad_index], 0, &parameters); /* TODO: Cycles. */
				}

				break;

			case 0xA10006 / 2:
				value = 0xFF;
				break;

			case 0xA10008 / 2:
			case 0xA1000A / 2:
			case 0xA1000C / 2:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = address - 0xA10008 / 2;

					value = IOPort_ReadControl(&clownmdemu->state->io_ports[joypad_index]);
				}

				break;
		}
	}
	else if (address == 0xA11000 / 2)
	{
		/* MEMORY MODE */
		/* TODO */
		/* https://gendev.spritesmind.net/forum/viewtopic.php?p=28843&sid=65d8f210be331ff257a43b4e3dddb7c3#p28843 */
		/* According to this, this flag is only functional on earlier models, and effectively halves the 68k's speed when running from cartridge. */
	}
	else if (address == 0xA11100 / 2)
	{
		/* Z80 BUSREQ */
		/* TODO: On real hardware, it seems that bus requests do not complete if a reset is being held. */
		/* http://gendev.spritesmind.net/forum/viewtopic.php?f=2&t=2195 */
		const cc_bool z80_running = !clownmdemu->state->z80.bus_requested;

		value = z80_running << 8;
	}
	else if (address == 0xA11200 / 2)
	{
		/* Z80 RESET */
		/* TODO */
	}
	else if (address == 0xA12000 / 2)
	{
		/* RESET, HALT */
		value = ((cc_u16f)clownmdemu->state->mega_cd.irq.enabled[1] << 15) |
			((cc_u16f)clownmdemu->state->mega_cd.m68k.bus_requested << 1) |
			((cc_u16f)!clownmdemu->state->mega_cd.m68k.reset_held << 0);
	}
	else if (address == 0xA12002 / 2)
	{
		/* Memory mode / Write protect */
		value = ((cc_u16f)clownmdemu->state->mega_cd.prg_ram.bank << 6) | ((cc_u16f)clownmdemu->state->mega_cd.word_ram.dmna << 1) | ((cc_u16f)clownmdemu->state->mega_cd.word_ram.in_1m_mode << 2) | ((cc_u16f)clownmdemu->state->mega_cd.word_ram.ret << 0);
	}
	else if (address == 0xA12004 / 2)
	{
		/* CDC mode */
		PrintError("MAIN-CPU attempted to read from CDC mode register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA12006 / 2)
	{
		/* H-INT vector */
		value = clownmdemu->state->mega_cd.hblank_address;
	}
	else if (address == 0xA12008 / 2)
	{
		/* CDC host data */
		PrintError("MAIN-CPU attempted to read from CDC host data register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA1200C / 2)
	{
		/* Stop watch */
		PrintError("MAIN-CPU attempted to read from stop watch register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA1200E / 2)
	{
		/* Communication flag */
		SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.flag;
	}
	else if (address >= 0xA12010 / 2 && address < 0xA12020 / 2)
	{
		/* Communication command */
		SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.command[address - 0xA12010 / 2];
	}
	else if (address >= 0xA12020 / 2 && address < 0xA12030 / 2)
	{
		/* Communication status */
		SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
		value = clownmdemu->state->mega_cd.communication.status[address - 0xA12020 / 2];
	}
	else if (address == 0xA12030 / 2)
	{
		/* Timer W/INT3 */
		PrintError("MAIN-CPU attempted to read from Timer W/INT3 register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA12032 / 2)
	{
		/* Interrupt mask control */
		PrintError("MAIN-CPU attempted to read from interrupt mask control register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xC00000 / 2 || address == 0xC00002 / 2 || address == 0xC00004 / 2 || address == 0xC00006 / 2)
	{
		if (address == 0xC00000 / 2 || address == 0xC00002 / 2)
		{
			/* VDP data port */
			/* TODO - Reading from the data port causes real Mega Drives to crash (if the VDP isn't in read mode) */
			value = VDP_ReadData(&clownmdemu->vdp);
		}
		else /*if (address == 0xC00004 || address == 0xC00006)*/
		{
			/* VDP control port */
			value = VDP_ReadControl(&clownmdemu->vdp);

			/* Temporary stupid hack: shove the PAL bit in here. */
			/* TODO: This should be moved to the VDP core once it becomes sensitive to PAL mode differences. */
			value |= (clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL);
		}
	}
	else if (address == 0xC00008 / 2)
	{
		/* H/V COUNTER */
		/* TODO: H counter. */
		/* TODO: Double-resolution mode. */
		/* TODO: The V counter emulation is incredibly inaccurate: the timing is likely wrong, and it should be incremented while in the blanking areas too. */
		value = clownmdemu->state->current_scanline << 8;
	}
	else if (address >= 0xC00010 / 2 && address <= 0xC00016 / 2)
	{
		/* PSG */
		/* TODO - What's supposed to happen here, if you read from the PSG? */
		/* TODO: It freezes the 68k, that's what:
		   https://forums.sonicretro.org/index.php?posts/1066059/ */
	}
	else if (address >= 0xE00000 / 2 && address <= 0xFFFFFF / 2)
	{
		/* 68k RAM */
		value = clownmdemu->state->m68k.ram[address & 0x7FFF];
	}
	else
	{
		PrintError("Attempted to read invalid 68k address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->state->m68k.state.program_counter);
	}

	return value;
}

cc_u16f M68kReadCallbackWithCycle(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u32f target_cycle)
{
	return M68kReadCallbackWithCycleWithDMA(user_data, address, do_high_byte, do_low_byte, target_cycle, cc_false);
}

cc_u16f M68kReadCallbackWithDMA(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_bool is_vdp_dma)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	return M68kReadCallbackWithCycleWithDMA(user_data, address, do_high_byte, do_low_byte, callback_user_data->m68k_current_cycle, is_vdp_dma);
}

cc_u16f M68kReadCallback(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte)
{
	return M68kReadCallbackWithDMA(user_data, address, do_high_byte, do_low_byte, cc_false);
}

void M68kWriteCallbackWithCycle(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u16f value, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;

	const cc_u16f high_byte = (value >> 8) & 0xFF;
	const cc_u16f low_byte = (value >> 0) & 0xFF;

	cc_u16f mask = 0;

	if (do_high_byte)
		mask |= 0xFF00;
	if (do_low_byte)
		mask |= 0x00FF;

	if (address < 0x800000 / 2)
	{
		if (((address & 0x200000) == 0) != clownmdemu->state->mega_cd.boot_from_cd)
		{
			/* Cartridge */
			if (do_high_byte)
				frontend_callbacks->cartridge_written((void*)frontend_callbacks->user_data, (address & 0x1FFFFF) * 2 + 0, high_byte);
			if (do_low_byte)
				frontend_callbacks->cartridge_written((void*)frontend_callbacks->user_data, (address & 0x1FFFFF) * 2 + 1, low_byte);

			/* TODO - This is temporary, just to catch possible bugs in the 68k emulator */
			PrintError("Attempted to write to ROM address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->state->m68k.state.program_counter);
		}
		else
		{
			if ((address & 0x100000) != 0)
			{
				/* WORD-RAM */
				if (clownmdemu->state->mega_cd.word_ram.in_1m_mode)
				{
					if ((address & 0x10000) != 0)
					{
						/* TODO */
						PrintError("MAIN-CPU attempted to write to that weird half of 1M WORD-RAM at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
					}
					else
					{
						clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + clownmdemu->state->mega_cd.word_ram.ret] &= ~mask;
						clownmdemu->state->mega_cd.word_ram.buffer[(address & 0xFFFF) * 2 + clownmdemu->state->mega_cd.word_ram.ret] |= value & mask;
					}
				}
				else
				{
					if (clownmdemu->state->mega_cd.word_ram.dmna)
					{
						PrintError("MAIN-CPU attempted to write to WORD-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
					}
					else
					{
						clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF] &= ~mask;
						clownmdemu->state->mega_cd.word_ram.buffer[address & 0x1FFFF] |= value & mask;
					}
				}
			}
			else if ((address & 0x10000) == 0)
			{
				/* Mega CD BIOS */
				PrintError("MAIN-CPU attempted to write to BIOS (0x%" CC_PRIXFAST32 ") at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->state->m68k.state.program_counter);
			}
			else
			{
				/* PRG-RAM */
				if (!clownmdemu->state->mega_cd.m68k.bus_requested)
				{
					PrintError("MAIN-CPU attempted to write to PRG-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
				}
				else
				{
					clownmdemu->state->mega_cd.prg_ram.buffer[0x10000 * clownmdemu->state->mega_cd.prg_ram.bank + (address & 0xFFFF)] &= ~mask;
					clownmdemu->state->mega_cd.prg_ram.buffer[0x10000 * clownmdemu->state->mega_cd.prg_ram.bank + (address & 0xFFFF)] |= value & mask;
				}
			}
		}
	}
	else if ((address >= 0xA00000 / 2 && address <= 0xA01FFF / 2) || address == 0xA04000 / 2 || address == 0xA04002 / 2)
	{
		/* Z80 RAM and YM2612 */
		if (!clownmdemu->state->z80.bus_requested)
		{
			PrintError("68k attempted to write Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (clownmdemu->state->z80.reset_held)
		{
			/* TODO: Does this actually bother real hardware? */
			/* TODO: According to Devon, yes it does. */
			PrintError("68k attempted to write Z80 memory/YM2612 ports while Z80 reset request was active at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized write of Z80 memory/YM2612 ports at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else
		{
			/* This is unnecessary, as the Z80 bus will have to have been requested, causing a sync. */
			/*SyncZ80(clownmdemu, callback_user_data, target_cycle);*/

			if (do_high_byte)
				Z80WriteCallbackWithCycle(user_data, (address * 2 + 0) & 0xFFFF, high_byte, target_cycle);
			else /*if (do_low_byte)*/
				Z80WriteCallbackWithCycle(user_data, (address * 2 + 1) & 0xFFFF, low_byte, target_cycle);
		}
	}
	else if (address >= 0xA10000 / 2 && address <= 0xA1001F / 2)
	{
		/* I/O AREA */
		/* TODO */
		switch (address)
		{
			case 0xA10002 / 2:
			case 0xA10004 / 2:
			case 0xA10006 / 2:
				if (do_low_byte)
				{
					IOPortToController_Parameters parameters;

					const cc_u16f joypad_index = address - 0xA10002 / 2;

					parameters.controller = &clownmdemu->state->controllers[joypad_index];
					parameters.frontend_callbacks = frontend_callbacks;

					IOPort_WriteData(&clownmdemu->state->io_ports[joypad_index], low_byte, 0, &parameters); /* TODO: Cycles. */
				}

				break;

			case 0xA10008 / 2:
			case 0xA1000A / 2:
			case 0xA1000C / 2:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = address - 0xA10008 / 2;

					IOPort_WriteControl(&clownmdemu->state->io_ports[joypad_index], low_byte);
				}

				break;
		}
	}
	else if (address == 0xA11000 / 2)
	{
		/* MEMORY MODE */
		/* TODO: Make setting this to DRAM mode make the cartridge writeable. */
	}
	else if (address == 0xA11100 / 2)
	{
		/* Z80 BUSREQ */
		if (do_high_byte)
		{
			const cc_bool bus_request = (high_byte & 1) != 0;

			if (clownmdemu->state->z80.bus_requested != bus_request)
				SyncZ80(clownmdemu, callback_user_data, target_cycle);

			clownmdemu->state->z80.bus_requested = bus_request;
		}
	}
	else if (address == 0xA11200 / 2)
	{
		/* Z80 RESET */
		if (do_high_byte)
		{
			const cc_bool new_reset_held = (high_byte & 1) == 0;

			if (clownmdemu->state->z80.reset_held && !new_reset_held)
			{
				SyncZ80(clownmdemu, callback_user_data, target_cycle);
				Z80_Reset(&clownmdemu->z80);
				FM_State_Initialise(&clownmdemu->state->fm);
			}

			clownmdemu->state->z80.reset_held = new_reset_held;
		}
	}
	else if (address == 0xA12000 / 2)
	{
		/* RESET, HALT */
		Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;

		const cc_bool interrupt = (high_byte & (1 << 0)) != 0;
		const cc_bool bus_request = (low_byte & (1 << 1)) != 0;
		const cc_bool reset = (low_byte & (1 << 0)) == 0;

		m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
		m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
		m68k_read_write_callbacks.user_data = callback_user_data;

		if (clownmdemu->state->mega_cd.m68k.bus_requested != bus_request)
			SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);

		if (clownmdemu->state->mega_cd.m68k.reset_held && !reset)
		{
			SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
			Clown68000_Reset(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);
		}

		if (interrupt && clownmdemu->state->mega_cd.irq.enabled[1])
		{
			SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
			Clown68000_Interrupt(clownmdemu->mcd_m68k, &m68k_read_write_callbacks, 2);
		}

		clownmdemu->state->mega_cd.m68k.bus_requested = bus_request;
		clownmdemu->state->mega_cd.m68k.reset_held = reset;
	}
	else if (address == 0xA12002 / 2)
	{
		/* Memory mode / Write protect */
		if (do_low_byte)
		{
			if ((low_byte & (1 << 1)) != 0)
			{
				SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);

				clownmdemu->state->mega_cd.word_ram.dmna = cc_true;

				if (!clownmdemu->state->mega_cd.word_ram.in_1m_mode)
					clownmdemu->state->mega_cd.word_ram.ret = cc_false;
			}

			clownmdemu->state->mega_cd.prg_ram.bank = (low_byte >> 6) & 3;
		}
	}
	else if (address == 0xA12004 / 2)
	{
		/* CDC mode */
		PrintError("MAIN-CPU attempted to write to CDC mode register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA12006 / 2)
	{
		/* H-INT vector */
		clownmdemu->state->mega_cd.hblank_address &= ~mask;
		clownmdemu->state->mega_cd.hblank_address |= value & mask;
	}
	else if (address == 0xA12008 / 2)
	{
		/* CDC host data */
		PrintError("MAIN-CPU attempted to write to CDC host data register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA1200C / 2)
	{
		/* Stop watch */
		PrintError("MAIN-CPU attempted to write to stop watch register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA1200E / 2)
	{
		/* Communication flag */
		if (do_high_byte)
		{
			SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
			clownmdemu->state->mega_cd.communication.flag = (clownmdemu->state->mega_cd.communication.flag & 0x00FF) | (value & 0xFF00);
		}

		if (do_low_byte)
			PrintError("MAIN-CPU attempted to write to SUB-CPU's communication flag at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address >= 0xA12010 / 2 && address < 0xA12020 / 2)
	{
		/* Communication command */
		SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
		clownmdemu->state->mega_cd.communication.command[address - 0xA12010 / 2] &= ~mask;
		clownmdemu->state->mega_cd.communication.command[address - 0xA12010 / 2] |= value & mask;
	}
	else if (address >= 0xA12020 / 2 && address < 0xA12030 / 2)
	{
		/* Communication status */
		PrintError("MAIN-CPU attempted to write to SUB-CPU's communication status at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA12030 / 2)
	{
		/* Timer W/INT3 */
		PrintError("MAIN-CPU attempted to write to Timer W/INT3 register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xA12032 / 2)
	{
		/* Interrupt mask control */
		PrintError("MAIN-CPU attempted to write to interrupt mask control register at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xC00000 / 2 || address == 0xC00002 / 2 || address == 0xC00004 / 2 || address == 0xC00006 / 2)
	{
		if (address == 0xC00000 / 2 || address == 0xC00002 / 2)
		{
			/* VDP data port */
			VDP_WriteData(&clownmdemu->vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data);
		}
		else /*if (address == 0xC00004 || address == 0xC00006)*/
		{
			/* VDP control port */
			VDP_WriteControl(&clownmdemu->vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data, VDPReadCallback, callback_user_data);
		}
	}
	else if (address == 0xC00008 / 2)
	{
		/* H/V COUNTER */
		/* TODO */
	}
	else if (address >= 0xC00010 / 2 && address <= 0xC00016 / 2)
	{
		/* PSG */
		if (do_low_byte)
		{
			SyncZ80(clownmdemu, callback_user_data, target_cycle);
			SyncPSG(callback_user_data, target_cycle);

			/* Alter the PSG's state */
			PSG_DoCommand(&clownmdemu->psg, low_byte);
		}
	}
	else if (address >= 0xE00000 / 2 && address <= 0xFFFFFF / 2)
	{
		/* 68k RAM */
		clownmdemu->state->m68k.ram[address & 0x7FFF] &= ~mask;
		clownmdemu->state->m68k.ram[address & 0x7FFF] |= value & mask;
	}
	else
	{
		PrintError("Attempted to write invalid 68k address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->state->m68k.state.program_counter);
	}
}

void M68kWriteCallback(const void* const user_data, const cc_u32f address, const cc_bool do_high_byte, const cc_bool do_low_byte, const cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	M68kWriteCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, value, callback_user_data->m68k_current_cycle);
}
