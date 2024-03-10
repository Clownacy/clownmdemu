#include "clownmdemu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon/clowncommon.h"

#include "error.h"
#include "fm.h"
#include "clown68000/interpreter/clown68000.h"
#include "psg.h"
#include "vdp.h"
#include "z80.h"

#define MAX_ROM_SIZE (1024 * 1024 * 4) /* 4MiB */

typedef struct DataAndCallbacks
{
	const ClownMDEmu *data;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} DataAndCallbacks;

typedef struct CPUCallbackUserData
{
	DataAndCallbacks data_and_callbacks;
	cc_u32f m68k_current_cycle;
	cc_u32f z80_current_cycle;
	cc_u32f mcd_m68k_current_cycle;
	cc_u32f fm_current_cycle;
	cc_u32f psg_current_cycle;
} CPUCallbackUserData;

typedef struct IOPortToController_Parameters
{
	Controller *controller;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} IOPortToController_Parameters;

/* TODO: Please, anything but this... */
#include "mcd_boot.c"

static cc_u32f ReadU32BE(const cc_u8l* const bytes)
{
	cc_u8f i;
	cc_u32f value;

	value = 0;

	for (i = 0; i < 4; ++i)
		value |= (cc_u32f)bytes[i] << (8 * (4 - 1 - i));

	return value;
}

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

static void BytesTo68kRAM(cc_u16l* const ram, const cc_u8l* const bytes, const size_t total_bytes)
{
	size_t i;

	for (i = 0; i < total_bytes / 2; ++i)
		ram[i] = ((cc_u16f)bytes[i * 2 + 0] << 8) | bytes[i * 2 + 1];
}

static void CDSectorTo68kRAM(const ClownMDEmu_Callbacks* const callbacks, cc_u16l* const ram)
{
	const cc_u8l* const sector_bytes = callbacks->cd_sector_read((void*)callbacks->user_data);

	BytesTo68kRAM(ram, sector_bytes, 0x800);
}

static void CDSectorsTo68kRAM(const ClownMDEmu_Callbacks* const callbacks, cc_u16l* const ram, const cc_u32f start, const cc_u32f length)
{
	cc_u32f i;

	callbacks->cd_seeked((void*)callbacks->user_data, start / 0x800);

	for (i = 0; i < CC_DIVIDE_CEILING(length, 0x800); ++i)
		CDSectorTo68kRAM(callbacks, &ram[i * 0x800 / 2]);
}

static cc_u16f M68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte);
static void M68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value);
static cc_u16f Z80ReadCallbackWithCycle(const void *user_data, cc_u16f address, const cc_u32f target_cycle);
static cc_u16f Z80ReadCallback(const void *user_data, cc_u16f address);
static void Z80WriteCallbackWithCycle(const void *user_data, cc_u16f address, cc_u16f value, const cc_u32f target_cycle);
static void Z80WriteCallback(const void *user_data, cc_u16f address, cc_u16f value);
static cc_u16f MCDM68kReadCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, const cc_u32f target_cycle);
static cc_u16f MCDM68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte);
static void MCDM68kWriteCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value, const cc_u32f target_cycle);
static void MCDM68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value);

static void SyncM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
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

static void SyncZ80(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Z80_ReadAndWriteCallbacks z80_read_write_callbacks;
	cc_u16f z80_countdown;

	z80_read_write_callbacks.read = Z80ReadCallback;
	z80_read_write_callbacks.write = Z80WriteCallback;
	z80_read_write_callbacks.user_data = other_state;

	/* Store this in a local variables to make the upcoming code faster. */
	z80_countdown = clownmdemu->state->z80.cycle_countdown;

	while (other_state->z80_current_cycle < target_cycle)
	{
		/* TODO: We repeat this code in each SyncXXXX function, so maybe we should de-duplicate it somehow? */
		const cc_u32f cycles_to_do = CC_MIN(z80_countdown, target_cycle - other_state->z80_current_cycle);

		assert(target_cycle >= other_state->z80_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

		z80_countdown -= cycles_to_do;

		if (z80_countdown == 0)
		{
			const cc_bool z80_not_running = clownmdemu->state->z80.m68k_has_bus || clownmdemu->state->z80.reset_held;

			z80_countdown = CLOWNMDEMU_Z80_CLOCK_DIVIDER * (z80_not_running ? 1 : Z80_DoCycle(&clownmdemu->z80, &z80_read_write_callbacks));
		}

		other_state->z80_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->z80.cycle_countdown = z80_countdown;
}

static void SyncMCDM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
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

static void FMCallbackWrapper(const ClownMDEmu *clownmdemu, cc_s16l *sample_buffer, size_t total_frames)
{
	FM_OutputSamples(&clownmdemu->fm, sample_buffer, total_frames);
}

static void GenerateFMAudio(const void *user_data, cc_u32f total_frames)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	callback_user_data->data_and_callbacks.frontend_callbacks->fm_audio_to_be_generated((void*)callback_user_data->data_and_callbacks.frontend_callbacks->user_data, total_frames, FMCallbackWrapper);
}

static cc_u8f SyncFM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f fm_target_cycle = target_cycle / CLOWNMDEMU_M68K_CLOCK_DIVIDER;

	const cc_u32f cycles_to_do = fm_target_cycle - other_state->fm_current_cycle;

	assert(fm_target_cycle >= other_state->fm_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	other_state->fm_current_cycle = fm_target_cycle;

	return FM_Update(&other_state->data_and_callbacks.data->fm, cycles_to_do, GenerateFMAudio, other_state);
}

static void GeneratePSGAudio(const ClownMDEmu *clownmdemu, cc_s16l *sample_buffer, size_t total_samples)
{
	PSG_Update(&clownmdemu->psg, sample_buffer, total_samples);
}

static void SyncPSG(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f psg_target_cycle = target_cycle / (CLOWNMDEMU_Z80_CLOCK_DIVIDER * CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER);

	const size_t samples_to_generate = psg_target_cycle - other_state->psg_current_cycle;

	assert(psg_target_cycle >= other_state->psg_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	if (samples_to_generate != 0)
	{
		other_state->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GeneratePSGAudio);

		other_state->psg_current_cycle = psg_target_cycle;
	}
}

/* VDP memory access callback */

static cc_u16f VDPReadCallback(void *user_data, cc_u32f address)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	if (address < 0x800000)
	{
		if (((address & 0x200000) == 0) != clownmdemu->state->mega_cd.boot_from_cd && (address & 0x200000) != 0)
		{
			/* Delay Word RAM DMAs*/
			clownmdemu->vdp.state->dma_cycle_delay = cc_true;
		}
	}
	return M68kReadCallback(user_data, address / 2, cc_true, cc_true);
}

/* 68k memory access callbacks */

static cc_u16f M68kReadCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, const cc_u32f target_cycle)
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
					return clownmdemu->state->mega_cd.hblank_address;
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
		if (!clownmdemu->state->z80.m68k_has_bus)
		{
			PrintError("68k attempted to read Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (clownmdemu->state->z80.reset_held)
		{
			/* TODO: Does this actually bother real hardware? */
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
		const cc_bool z80_running = !clownmdemu->state->z80.m68k_has_bus;

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
		value = ((cc_u16f)clownmdemu->state->mega_cd.irq.irq2_enabled << 15) |
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

static cc_u16f M68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	return M68kReadCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, callback_user_data->m68k_current_cycle);
}

static void M68kWriteCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value, const cc_u32f target_cycle)
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
		if (!clownmdemu->state->z80.m68k_has_bus)
		{
			PrintError("68k attempted to write Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.state.program_counter);
		}
		else if (clownmdemu->state->z80.reset_held)
		{
			/* TODO: Does this actually bother real hardware? */
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

			if (clownmdemu->state->z80.m68k_has_bus != bus_request)
				SyncZ80(clownmdemu, callback_user_data, target_cycle);

			clownmdemu->state->z80.m68k_has_bus = bus_request;
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

		if (interrupt && clownmdemu->state->mega_cd.irq.irq2_enabled)
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

static void M68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	M68kWriteCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, value, callback_user_data->m68k_current_cycle);
}

/* Z80 memory access callbacks */
/* TODO: https://sonicresearch.org/community/index.php?threads/help-with-potentially-extra-ram-space-for-z80-sound-drivers.6763/#post-89797 */

static cc_u16f Z80ReadCallbackWithCycle(const void *user_data, cc_u16f address, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	/* I suppose, on real hardware, in an open-bus situation, this would actually
	   be a static variable that retains its value from the last call. */
	cc_u16f value;

	value = 0;

	if (address < 0x2000)
	{
		value = clownmdemu->state->z80.ram[address];
	}
	else if (address >= 0x4000 && address <= 0x4003)
	{
		/* YM2612 */
		/* TODO: Model 1 Mega Drives only do this for 0x4000 and 0x4002. */
		value = SyncFM(callback_user_data, target_cycle);
	}
	else if (address == 0x6000 || address == 0x6001)
	{
		/* TODO: Does this do *anything*? */
	}
	else if (address == 0x7F11)
	{
		/* PSG */
		/* TODO */
	}
	else if (address >= 0x8000)
	{
		/* 68k ROM window (actually a window into the 68k's address space: you can access the PSG through it IIRC). */
		const cc_u32f m68k_address = ((cc_u32f)clownmdemu->state->z80.bank * 0x8000) + (address & 0x7FFE);

		SyncM68k(clownmdemu, callback_user_data, target_cycle);

		if ((address & 1) != 0)
			value = M68kReadCallbackWithCycle(user_data, m68k_address / 2, cc_false, cc_true, target_cycle);
		else
			value = M68kReadCallbackWithCycle(user_data, m68k_address / 2, cc_true, cc_false, target_cycle) >> 8;
	}
	else
	{
		PrintError("Attempted to read invalid Z80 address 0x%" CC_PRIXFAST16 " at 0x%" CC_PRIXLEAST16, address, clownmdemu->state->z80.state.program_counter);
	}

	return value;
}

static cc_u16f Z80ReadCallback(const void *user_data, cc_u16f address)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	return Z80ReadCallbackWithCycle(user_data, address, callback_user_data->z80_current_cycle);
}

static void Z80WriteCallbackWithCycle(const void *user_data, cc_u16f address, cc_u16f value, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	if (address < 0x2000)
	{
		clownmdemu->state->z80.ram[address] = value;
	}
	else if (address >= 0x4000 && address <= 0x4003)
	{
		/* YM2612 */
		const cc_u16f port = (address & 2) != 0 ? 1 : 0;

		/* Update the FM up until this point in time. */
		SyncFM(callback_user_data, target_cycle);

		if ((address & 1) == 0)
			FM_DoAddress(&clownmdemu->fm, port, value);
		else
			FM_DoData(&clownmdemu->fm, value);
	}
	else if (address == 0x6000 || address == 0x6001)
	{
		clownmdemu->state->z80.bank >>= 1;
		clownmdemu->state->z80.bank |= (value & 1) != 0 ? 0x100 : 0;
	}
	else if (address == 0x7F11)
	{
		/* PSG (accessed through the 68k's bus). */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		M68kWriteCallbackWithCycle(user_data, 0xC00010 / 2, cc_false, cc_true, value, target_cycle);
	}
	else if (address >= 0x8000)
	{
		/* TODO: Apparently Mamono Hunter Youko needs the Z80 to be able to write to 68k RAM in order to boot?
		   777 Casino also does weird stuff like this.
		   http://gendev.spritesmind.net/forum/viewtopic.php?f=24&t=347&start=30 */

		/* 68k ROM window (actually a window into the 68k's address space: you can access the PSG through it IIRC). */
		/* TODO: Apparently the Z80 can access the IO ports and send a bus request to itself. */
		const cc_u32f m68k_address = ((cc_u32f)clownmdemu->state->z80.bank * 0x8000) + (address & 0x7FFE);

		SyncM68k(clownmdemu, callback_user_data, target_cycle);

		if ((address & 1) != 0)
			M68kWriteCallbackWithCycle(user_data, m68k_address / 2, cc_false, cc_true, value, target_cycle);
		else
			M68kWriteCallbackWithCycle(user_data, m68k_address / 2, cc_true, cc_false, value << 8, target_cycle);
	}
	else
	{
		PrintError("Attempted to write invalid Z80 address 0x%" CC_PRIXFAST16 " at 0x%" CC_PRIXLEAST16, address, clownmdemu->state->z80.state.program_counter);
	}
}

static void Z80WriteCallback(const void *user_data, cc_u16f address, cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	Z80WriteCallbackWithCycle(user_data, address, value, callback_user_data->z80_current_cycle);
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

/* MCD 68k (SUB-CPU) memory access callbacks */

static cc_u16f MCDM68kReadCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, const cc_u32f target_cycle)
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
		value = ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq1_enabled << 1)) |
		        ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq2_enabled << 2)) |
		        ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq3_enabled << 3)) |
		        ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq4_enabled << 4)) |
		        ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq5_enabled << 5)) |
		        ((cc_u16f)(clownmdemu->state->mega_cd.irq.irq6_enabled << 6));
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

static cc_u16f MCDM68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	return MCDM68kReadCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, callback_user_data->mcd_m68k_current_cycle);
}

static void MCDM68kWriteCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value, const cc_u32f target_cycle)
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
			clownmdemu->state->mega_cd.irq.irq1_enabled = (value & (1 << 1)) != 0;
			clownmdemu->state->mega_cd.irq.irq2_enabled = (value & (1 << 2)) != 0;
			clownmdemu->state->mega_cd.irq.irq3_enabled = (value & (1 << 3)) != 0;
			clownmdemu->state->mega_cd.irq.irq4_enabled = (value & (1 << 4)) != 0;
			clownmdemu->state->mega_cd.irq.irq5_enabled = (value & (1 << 5)) != 0;
			clownmdemu->state->mega_cd.irq.irq6_enabled = (value & (1 << 6)) != 0;

			if (!clownmdemu->state->mega_cd.irq.irq1_enabled)
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
		if (clownmdemu->state->mega_cd.irq.irq1_enabled)
			clownmdemu->state->mega_cd.irq.irq1_pending = cc_true;
	}
	else
	{
		PrintError("Attempted to write invalid MCD 68k address 0x%" CC_PRIXFAST32 " at 0x%" CC_PRIXLEAST32, address * 2, clownmdemu->mcd_m68k->program_counter);
	}
}

static void MCDM68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	MCDM68kWriteCallbackWithCycle(user_data, address, do_high_byte, do_low_byte, value, callback_user_data->mcd_m68k_current_cycle);
}

void ClownMDEmu_Constant_Initialise(ClownMDEmu_Constant *constant)
{
	Z80_Constant_Initialise(&constant->z80);
	VDP_Constant_Initialise(&constant->vdp);
	FM_Constant_Initialise(&constant->fm);
	PSG_Constant_Initialise(&constant->psg);
}

static cc_bool FrontendControllerCallbackCommon(void* const user_data, const Controller_Button button, const cc_u8f joypad_index)
{
	ClownMDEmu_Button frontend_button;

	const ClownMDEmu_Callbacks *frontend_callbacks = (const ClownMDEmu_Callbacks*)user_data;

	switch (button)
	{
		case CONTROLLER_BUTTON_UP:
			frontend_button = CLOWNMDEMU_BUTTON_UP;
			break;

		case CONTROLLER_BUTTON_DOWN:
			frontend_button = CLOWNMDEMU_BUTTON_DOWN;
			break;

		case CONTROLLER_BUTTON_LEFT:
			frontend_button = CLOWNMDEMU_BUTTON_LEFT;
			break;

		case CONTROLLER_BUTTON_RIGHT:
			frontend_button = CLOWNMDEMU_BUTTON_RIGHT;
			break;

		case CONTROLLER_BUTTON_A:
			frontend_button = CLOWNMDEMU_BUTTON_A;
			break;

		case CONTROLLER_BUTTON_B:
			frontend_button = CLOWNMDEMU_BUTTON_B;
			break;

		case CONTROLLER_BUTTON_C:
			frontend_button = CLOWNMDEMU_BUTTON_C;
			break;

		case CONTROLLER_BUTTON_START:
			frontend_button = CLOWNMDEMU_BUTTON_START;
			break;

		/* TODO: These buttons. */
		case CONTROLLER_BUTTON_X:
		case CONTROLLER_BUTTON_Y:
		case CONTROLLER_BUTTON_Z:
		case CONTROLLER_BUTTON_MODE:
			return cc_false;
	}

	return frontend_callbacks->input_requested((void*)frontend_callbacks->user_data, joypad_index, frontend_button);
}

static cc_bool FrontendController1Callback(void* const user_data, const Controller_Button button)
{
	return FrontendControllerCallbackCommon(user_data, button, 0);
}

static cc_bool FrontendController2Callback(void* const user_data, const Controller_Button button)
{
	return FrontendControllerCallbackCommon(user_data, button, 1);
}

static cc_u8f IOPortToController_ReadCallback(void* const user_data, const cc_u16f cycles)
{
	const IOPortToController_Parameters *parameters = (const IOPortToController_Parameters*)user_data;

	return Controller_Read(parameters->controller, cycles, parameters->frontend_callbacks);
}

static void IOPortToController_WriteCallback(void* const user_data, const cc_u8f value, const cc_u16f cycles)
{
	const IOPortToController_Parameters *parameters = (const IOPortToController_Parameters*)user_data;

	Controller_Write(parameters->controller, value, cycles);
}

void ClownMDEmu_State_Initialise(ClownMDEmu_State *state)
{
	cc_u16f i;

	/* M68K */
	memset(state->m68k.ram, 0, sizeof(state->m68k.ram));
	state->m68k.cycle_countdown = 0;

	/* Z80 */
	Z80_State_Initialise(&state->z80.state);
	memset(state->z80.ram, 0, sizeof(state->z80.ram));
	state->z80.cycle_countdown = 0;
	state->z80.bank = 0;
	state->z80.m68k_has_bus = cc_true;
	state->z80.reset_held = cc_true;

	VDP_State_Initialise(&state->vdp);
	FM_State_Initialise(&state->fm);
	PSG_State_Initialise(&state->psg);

	for (i = 0; i < CC_COUNT_OF(state->io_ports); ++i)
	{
		/* The standard Sega SDK bootcode uses this to detect soft-resets. */
		IOPort_Initialise(&state->io_ports[i]);
	}

	IOPort_SetCallbacks(&state->io_ports[0], IOPortToController_ReadCallback, IOPortToController_WriteCallback);
	IOPort_SetCallbacks(&state->io_ports[1], IOPortToController_ReadCallback, IOPortToController_WriteCallback);

	Controller_Initialise(&state->controllers[0], FrontendController1Callback);
	Controller_Initialise(&state->controllers[1], FrontendController2Callback);

	/* Mega CD */
	state->mega_cd.m68k.cycle_countdown = 0;
	state->mega_cd.m68k.bus_requested = cc_true;
	state->mega_cd.m68k.reset_held = cc_true;

	memset(state->mega_cd.prg_ram.buffer, 0, sizeof(state->mega_cd.prg_ram.buffer));
	state->mega_cd.prg_ram.bank = 0;

	memset(state->mega_cd.word_ram.buffer, 0, sizeof(state->mega_cd.word_ram.buffer));
	state->mega_cd.word_ram.in_1m_mode = cc_true; /* Confirmed by my Visual Sound Test homebrew. */
	/* Page 24 of MEGA-CD HARDWARE MANUAL confirms this. */
	state->mega_cd.word_ram.dmna = cc_false;
	state->mega_cd.word_ram.ret = cc_true;

	state->mega_cd.communication.flag = 0;

	for (i = 0; i < CC_COUNT_OF(state->mega_cd.communication.command); ++i)
		state->mega_cd.communication.command[i] = 0;

	for (i = 0; i < CC_COUNT_OF(state->mega_cd.communication.status); ++i)
		state->mega_cd.communication.status[i] = 0;

	state->mega_cd.cd.current_sector = 0;
	state->mega_cd.cd.total_buffered_sectors = 0;
	state->mega_cd.cd.cdc_ready = cc_false;
	
	state->mega_cd.irq.irq1_enabled = cc_false;
	state->mega_cd.irq.irq2_enabled = cc_false;
	state->mega_cd.irq.irq3_enabled = cc_false;
	state->mega_cd.irq.irq4_enabled = cc_false;
	state->mega_cd.irq.irq5_enabled = cc_false;
	state->mega_cd.irq.irq6_enabled = cc_false;
	state->mega_cd.irq.irq1_pending = cc_false;

	state->mega_cd.boot_from_cd = cc_false;

	state->mega_cd.hblank_address = 0xFFFF;
}

void ClownMDEmu_Parameters_Initialise(ClownMDEmu *clownmdemu, const ClownMDEmu_Configuration *configuration, const ClownMDEmu_Constant *constant, ClownMDEmu_State *state)
{
	clownmdemu->configuration = configuration;
	clownmdemu->constant = constant;
	clownmdemu->state = state;

	clownmdemu->m68k = &state->m68k.state;

	clownmdemu->z80.constant = &constant->z80;
	clownmdemu->z80.state = &state->z80.state;

	clownmdemu->mcd_m68k = &state->mega_cd.m68k.state;

	clownmdemu->vdp.configuration = &configuration->vdp;
	clownmdemu->vdp.constant = &constant->vdp;
	clownmdemu->vdp.state = &state->vdp;

	FM_Parameters_Initialise(&clownmdemu->fm, &configuration->fm, &constant->fm, &state->fm);

	clownmdemu->psg.configuration = &configuration->psg;
	clownmdemu->psg.constant = &constant->psg;
	clownmdemu->psg.state = &state->psg;
}

void ClownMDEmu_Iterate(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks, mcd_m68k_read_write_callbacks;
	CPUCallbackUserData cpu_callback_user_data;
	cc_u8f h_int_counter;

	const cc_u16f television_vertical_resolution = clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? 312 : 262; /* PAL and NTSC, respectively */
	const cc_u16f console_vertical_resolution = (clownmdemu->state->vdp.v30_enabled ? 30 : 28) * 8; /* 240 and 224 */
	const cc_u16f cycles_per_frame = clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_PAL) : CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_NTSC);
	const cc_u16f cycles_per_scanline = cycles_per_frame / television_vertical_resolution;

	cpu_callback_user_data.data_and_callbacks.data = clownmdemu;
	cpu_callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;
	cpu_callback_user_data.m68k_current_cycle = 0;
	cpu_callback_user_data.z80_current_cycle = 0;
	cpu_callback_user_data.mcd_m68k_current_cycle = 0;
	cpu_callback_user_data.fm_current_cycle = 0;
	cpu_callback_user_data.psg_current_cycle = 0;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &cpu_callback_user_data;

	mcd_m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	mcd_m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	mcd_m68k_read_write_callbacks.user_data = &cpu_callback_user_data;

	/* Reload H-Int counter at the top of the screen, just like real hardware does */
	h_int_counter = clownmdemu->state->vdp.h_int_interval;

	clownmdemu->state->vdp.currently_in_vblank = cc_false;

	for (clownmdemu->state->current_scanline = 0; clownmdemu->state->current_scanline < television_vertical_resolution; ++clownmdemu->state->current_scanline)
	{
		const cc_u16f scanline = clownmdemu->state->current_scanline;
		const cc_u32f current_cycle = cycles_per_scanline * (1 + scanline);

		/* Sync the 68k, since it's the one thing that can influence the VDP */
		SyncM68k(clownmdemu, &cpu_callback_user_data, current_cycle);

		/* Only render scanlines and generate H-Ints for scanlines that the console outputs to */
		if (scanline < console_vertical_resolution)
		{
			if (clownmdemu->state->vdp.double_resolution_enabled)
			{
				/* TODO: I'm pretty sure that these scanlines are meant to be a scanline apart. */
				VDP_RenderScanline(&clownmdemu->vdp, scanline * 2, callbacks->scanline_rendered, callbacks->user_data);
				VDP_RenderScanline(&clownmdemu->vdp, scanline * 2 + 1, callbacks->scanline_rendered, callbacks->user_data);
			}
			else
			{
				VDP_RenderScanline(&clownmdemu->vdp, scanline, callbacks->scanline_rendered, callbacks->user_data);
			}

			/* Fire a H-Int if we've reached the requested line */
			if (h_int_counter-- == 0)
			{
				h_int_counter = clownmdemu->state->vdp.h_int_interval;

				/* Do H-Int */
				if (clownmdemu->state->vdp.h_int_enabled)
					Clown68000_Interrupt(clownmdemu->m68k, &m68k_read_write_callbacks, 4);
			}
		}
		else if (scanline == console_vertical_resolution) /* Check if we have reached the end of the console-output scanlines */
		{
			/* Do V-Int */
			if (clownmdemu->state->vdp.v_int_enabled)
			{
				Clown68000_Interrupt(clownmdemu->m68k, &m68k_read_write_callbacks, 6);

				SyncZ80(clownmdemu, &cpu_callback_user_data, current_cycle);
				Z80_Interrupt(&clownmdemu->z80);
			}

			/* Flag that we have entered the V-blank region */
			clownmdemu->state->vdp.currently_in_vblank = cc_true;
		}
	}

	/* Update everything else for the rest of the frame. */
	SyncZ80(clownmdemu, &cpu_callback_user_data, cycles_per_frame);
	SyncMCDM68k(clownmdemu, &cpu_callback_user_data, cycles_per_frame);
	SyncFM(&cpu_callback_user_data, cycles_per_frame);
	SyncPSG(&cpu_callback_user_data, cycles_per_frame);

	/* Fire IRQ1 if needed. */
	/* TODO: This is a hack. Look into when this interrupt should actually be done. */
	if (clownmdemu->state->mega_cd.irq.irq1_pending)
	{
		clownmdemu->state->mega_cd.irq.irq1_pending = cc_false;
		Clown68000_Interrupt(clownmdemu->mcd_m68k, &mcd_m68k_read_write_callbacks, 1);
	}
}

void ClownMDEmu_Reset(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks, const cc_bool cd_boot)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	CPUCallbackUserData callback_user_data;

	clownmdemu->state->mega_cd.boot_from_cd = cd_boot;

	if (cd_boot)
	{
		/* Boot from CD ("Mode 2"). */
		cc_u8f i;
		const cc_u8l *sector_bytes;
		cc_u32f ip_start, ip_length, sp_start, sp_length;
		cc_u8l region;

		/* Read first sector. */
		callbacks->cd_seeked((void*)callbacks->user_data, 0);
		sector_bytes = callbacks->cd_sector_read((void*)callbacks->user_data);
		ip_start = ReadU32BE(&sector_bytes[0x30]);
		ip_length = ReadU32BE(&sector_bytes[0x34]);
		sp_start = ReadU32BE(&sector_bytes[0x40]);
		sp_length = ReadU32BE(&sector_bytes[0x44]);
		region = sector_bytes[0x1F0];

		/* Don't allow overflowing the PRG-RAM array. */
		sp_length = CC_MIN(CC_COUNT_OF(clownmdemu->state->mega_cd.prg_ram.buffer) * 2 - 0x6000, sp_length);

		/* Read Initial Program. */
		BytesTo68kRAM(clownmdemu->state->mega_cd.word_ram.buffer, &sector_bytes[0x200], 0x600);

		/* Load additional Initial Program data if necessary. */
		if (ip_start != 0x200 || ip_length != 0x600)
			CDSectorsTo68kRAM(callbacks, &clownmdemu->state->mega_cd.word_ram.buffer[0x600 / 2], 0x800, 32 * 0x800);

		/* This is what the Mega CD's BIOS does. */
		memcpy(clownmdemu->state->m68k.ram, clownmdemu->state->mega_cd.word_ram.buffer, 0x8000);

		/* Read Sub Program. */
		CDSectorsTo68kRAM(callbacks, &clownmdemu->state->mega_cd.prg_ram.buffer[0x6000 / 2], sp_start, sp_length);

		/* Give WORD-RAM to the SUB-CPU. */
		clownmdemu->state->mega_cd.word_ram.dmna = cc_true;
		clownmdemu->state->mega_cd.word_ram.ret = cc_false;
	}

	callback_user_data.data_and_callbacks.data = clownmdemu;
	callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;

	m68k_read_write_callbacks.user_data = &callback_user_data;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	Clown68000_Reset(clownmdemu->m68k, &m68k_read_write_callbacks);

	Z80_Reset(&clownmdemu->z80);

	m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	Clown68000_Reset(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);
}

void ClownMDEmu_SetErrorCallback(void (*error_callback)(void *user_data, const char *format, va_list arg), const void* const user_data)
{
	SetErrorCallback(error_callback, user_data);
	Clown68000_SetErrorCallback(error_callback, user_data);
}
