#include "clownmdemu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon/clowncommon.h"

#include "error.h"
#include "fm.h"
#include "clown68000/clown68000.h"
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

/* This is the 'bios.kos' file that can be found in the 'SUB-CPU BIOS' directory. */
/* TODO: Please, anything but this... */
static const cc_u16l subcpu_bios[] = {
	0x0F63, 0x0000, 0x5000, 0xFF01,
	0x0AFC, 0x0063, 0xB2FC, 0xF85A,
	0x08FC, 0x34F8, 0xFCF8, 0x1200,
	0xC3FF, 0xFCF8, 0x7B4E, 0x71FE,
	0x6000, 0xFFFA, 0x4E73, 0x41FA,
	0x5EFF, 0xFFF4, 0x0C90, 0x4D41,
	0x494E, 0x66F4, 0xD1E8, 0x0018,
	0x3010, 0x4EF0, 0x98B0, 0xDE46,
	0xFC27, 0x00E6, 0xDA7F, 0x6AEE,
	0x2800, 0x024E, 0xF000, 0xF0CA,
	0xD6FD, 0x0C13, 0x08E8, 0xFD04,
	0xD4BC, 0x00F0, 0x0000
};

static cc_u16f M68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte);
static void M68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value);
static cc_u16f Z80ReadCallback(const void *user_data, cc_u16f address);
static void Z80WriteCallback(const void *user_data, cc_u16f address, cc_u16f value);
static cc_u16f MCDM68kReadCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte);
static void MCDM68kWriteCallback(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, cc_u16f value);

static void SyncM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	cc_u16f m68k_countdown;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = other_state;

	/* Store this in a local variable to make the upcoming code faster. */
	m68k_countdown = clownmdemu->state->countdowns.m68k;

	while (other_state->m68k_current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(m68k_countdown, target_cycle - other_state->m68k_current_cycle);

		m68k_countdown -= cycles_to_do;

		if (m68k_countdown == 0)
		{
			Clown68000_DoCycle(clownmdemu->m68k, &m68k_read_write_callbacks);
			m68k_countdown = CLOWNMDEMU_M68K_CLOCK_DIVIDER * 10; /* TODO: The '* 10' is a temporary hack until 68000 instruction durations are added. */
		}

		other_state->m68k_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->countdowns.m68k = m68k_countdown;
}

static void SyncZ80(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Z80_ReadAndWriteCallbacks z80_read_write_callbacks;
	cc_u16f z80_countdown;

	z80_read_write_callbacks.read = Z80ReadCallback;
	z80_read_write_callbacks.write = Z80WriteCallback;
	z80_read_write_callbacks.user_data = other_state;

	/* Store this in a local variables to make the upcoming code faster. */
	z80_countdown = clownmdemu->state->countdowns.z80;

	while (other_state->z80_current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(z80_countdown, target_cycle - other_state->z80_current_cycle);

		z80_countdown -= cycles_to_do;

		if (z80_countdown == 0)
			z80_countdown = CLOWNMDEMU_Z80_CLOCK_DIVIDER * (clownmdemu->state->m68k_has_z80_bus ? 1 : Z80_DoCycle(&clownmdemu->z80, &z80_read_write_callbacks));

		other_state->z80_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->countdowns.z80 = z80_countdown;
}

static void SyncMCDM68k(const ClownMDEmu* const clownmdemu, CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	cc_u16f m68k_countdown;

	m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	m68k_read_write_callbacks.user_data = other_state;

	/* Store this in a local variable to make the upcoming code faster. */
	m68k_countdown = clownmdemu->state->countdowns.mcd_m68k;

	while (other_state->mcd_m68k_current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(m68k_countdown, target_cycle - other_state->mcd_m68k_current_cycle);

		m68k_countdown -= cycles_to_do;

		if (m68k_countdown == 0)
		{
			if (!clownmdemu->state->m68k_has_mcd_m68k_bus)
				Clown68000_DoCycle(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);

			m68k_countdown = CLOWNMDEMU_MCD_M68K_CLOCK_DIVIDER * 10; /* TODO: The '* 10' is a temporary hack until 68000 instruction durations are added. */
			/* TODO: Handle the MCD's master clock! */
		}

		other_state->mcd_m68k_current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	clownmdemu->state->countdowns.mcd_m68k = m68k_countdown;
}

static void FMCallbackWrapper(const ClownMDEmu *clownmdemu, cc_s16l *sample_buffer, size_t total_frames)
{
	FM_OutputSamples(&clownmdemu->fm, sample_buffer, total_frames);
}

static void GenerateFMAudio(const void *user_data, cc_u32f total_frames)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	callback_user_data->data_and_callbacks.frontend_callbacks->fm_audio_to_be_generated(callback_user_data->data_and_callbacks.frontend_callbacks->user_data, total_frames, FMCallbackWrapper);
}

static cc_u8f SyncFM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f fm_target_cycle = target_cycle / CLOWNMDEMU_M68K_CLOCK_DIVIDER;

	const cc_u32f cycles_to_do = fm_target_cycle - other_state->fm_current_cycle;

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

	if (samples_to_generate != 0)
	{
		other_state->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated(other_state->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GeneratePSGAudio);

		other_state->psg_current_cycle = psg_target_cycle;
	}
}

/* VDP memory access callback */

static cc_u16f VDPReadCallback(const void *user_data, cc_u32f address)
{
	DataAndCallbacks *data_and_callbacks = (DataAndCallbacks*)user_data;
	ClownMDEmu_State *state = data_and_callbacks->data->state;
	const ClownMDEmu_Callbacks *frontend_callbacks = data_and_callbacks->frontend_callbacks;
	cc_u16f value = 0;

	if (/*address >= 0 &&*/ address < MAX_ROM_SIZE)
	{
		/* Cartridge. */
		value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 0) << 8;
		value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 1) << 0;
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		/* 68k RAM. */
		value |= state->m68k_ram[(address + 0) & 0xFFFF] << 8;
		value |= state->m68k_ram[(address + 1) & 0xFFFF] << 0;
	}
	else
	{
		PrintError("VDP attempted to read invalid memory 0x%" CC_PRIXFAST32, address);
	}

	return value;
}

/* 68k memory access callbacks */

static cc_u16f Z80ReadCallbackWithCycle(const void *user_data, cc_u16f address, const cc_u32f target_cycle);
static void Z80WriteCallbackWithCycle(const void *user_data, cc_u16f address, cc_u16f value, const cc_u32f target_cycle);

static cc_u16f M68kReadCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;
	cc_u16f value = 0;

	if (address < 0x800000)
	{
		if ((address & 0x400000) == 0)
		{
			/* Cartridge */
			if (do_high_byte)
				value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, (address + 0) & 0x3FFFFF) << 8;
			if (do_low_byte)
				value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, (address + 1) & 0x3FFFFF) << 0;
		}
		else
		{
			if ((address & 0x200000) != 0)
			{
				/* WORD-RAM */
				if (clownmdemu->state->mcd_has_word_ram)
				{
					PrintError("MAIN-CPU attempted to read from WORD-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
				}
				else
				{
					if (do_high_byte)
						value |= clownmdemu->state->word_ram[(address + 0) & 0x3FFFF] << 8;
					if (do_low_byte)
						value |= clownmdemu->state->word_ram[(address + 1) & 0x3FFFF] << 0;
				}
			}
			else if ((address & 0x20000) == 0)
			{
				/* Mega CD BIOS */
				const cc_u16f local_address = address & 0x1FFFF;

				if (local_address >= 0x16000 && local_address < 0x16000 + CC_COUNT_OF(subcpu_bios) * 2)
				{
					/* Kosinski-compressed SUB-CPU payload. */
					value = subcpu_bios[(local_address - 0x16000) / 2];
				}
				else if (local_address == 0x1606E)
				{
					/* SUB-CPU payload magic number (used by ROM-hacks that use 'Mode 1'). */
					value = ('E' << 8) | ('G' << 0);
				}
			}
			else
			{
				/* PRG-RAM */
				if (!clownmdemu->state->m68k_has_mcd_m68k_bus)
				{
					PrintError("MAIN-CPU attempted to read from PRG-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
				}
				else
				{
					if (do_high_byte)
						value |= clownmdemu->state->prg_ram[0x20000 * clownmdemu->state->prg_ram_bank + (address + 0) & 0x1FFFF] << 8;
					if (do_low_byte)
						value |= clownmdemu->state->prg_ram[0x20000 * clownmdemu->state->prg_ram_bank + (address + 1) & 0x1FFFF] << 0;
				}
			}
		}
	}
	else if ((address >= 0xA00000 && address <= 0xA01FFF) || address == 0xA04000 || address == 0xA04002)
	{
		/* Z80 RAM and YM2612 */
		if (!clownmdemu->state->m68k_has_z80_bus)
		{
			PrintError("68k attempted to read Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else if (clownmdemu->state->z80_reset)
		{
			PrintError("68k attempted to read Z80 memory/YM2612 ports while Z80 reset request was active at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized read of Z80 memory/YM2612 ports at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				value = Z80ReadCallbackWithCycle(user_data, (address & 0xFFFF) + 0, target_cycle) << 8;
			else /*if (do_low_byte)*/
				value = Z80ReadCallbackWithCycle(user_data, (address & 0xFFFF) + 1, target_cycle) << 0;
		}
	}
	else if (address >= 0xA10000 && address <= 0xA1001F)
	{
		/* I/O AREA */
		/* TODO */
		switch (address)
		{
			case 0xA10000:
				if (do_low_byte)
					value |= ((clownmdemu->configuration->general.region == CLOWNMDEMU_REGION_OVERSEAS) << 7) | ((clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL) << 6) | (0 << 5);	/* Bit 5 clear = Mega CD attached */

				break;

			case 0xA10002:
			case 0xA10004:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = (address - 0xA10002) / 2;

					value |= clownmdemu->state->joypads[joypad_index].data;

					if ((value & 0x40) != 0)
					{
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_C) << 5;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_B) << 4;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_RIGHT) << 3;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_LEFT) << 2;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_DOWN) << 1;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_UP) << 0;
					}
					else
					{
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_START) << 5;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_A) << 4;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_DOWN) << 1;
						value |= !frontend_callbacks->input_requested(frontend_callbacks->user_data, joypad_index, CLOWNMDEMU_BUTTON_UP) << 0;
					}
				}

				break;

			case 0xA10006:
				value = 0xFF;
				break;

			case 0xA10008:
			case 0xA1000A:
			case 0xA1000C:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = (address - 0xA10008) / 2;

					value = clownmdemu->state->joypads[joypad_index].control;
				}

				break;
		}
	}
	else if (address == 0xA11000)
	{
		/* MEMORY MODE */
		/* TODO */
		/* https://gendev.spritesmind.net/forum/viewtopic.php?p=28843&sid=65d8f210be331ff257a43b4e3dddb7c3#p28843 */
		/* According to this, this flag is only functional on earlier models, and effectively halves the 68k's speed when running from cartridge. */
	}
	else if (address == 0xA11100)
	{
		/* Z80 BUSREQ */
		/* TODO */
	}
	else if (address == 0xA11200)
	{
		/* Z80 RESET */
		/* TODO */
	}
	else if (address == 0xA12000)
	{
		/* RESET, HALT */
		value = ((cc_u16f)clownmdemu->state->m68k_has_mcd_m68k_bus << 1) | (1 << 0); /* Always show that the reset was successful. */
	}
	else if (address == 0xA12002)
	{
		/* Memory mode / Write protect */
		value = (cc_u16f)clownmdemu->state->prg_ram_bank << 6;
	}
	else if (address == 0xA1200E)
	{
		/* Communication flag */
		value = clownmdemu->state->mcd_communication_flag;
	}
	else if (address >= 0xA12010 && address < 0xA12020)
	{
		/* Communication command */
		value = clownmdemu->state->mcd_communication_command[(address - 0xA12010) / 2];
	}
	else if (address >= 0xA12020 && address < 0xA12030)
	{
		/* Communication status */
		value = clownmdemu->state->mcd_communication_status[(address - 0xA12020) / 2];
	}
	else if (address == 0xC00000 || address == 0xC00002 || address == 0xC00004 || address == 0xC00006)
	{
		if (address == 0xC00000 || address == 0xC00002)
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
	else if (address == 0xC00008)
	{
		/* H/V COUNTER */
		/* TODO */
	}
	else if (address >= 0xC00010 && address <= 0xC00016)
	{
		/* PSG */
		/* TODO - What's supposed to happen here, if you read from the PSG? */
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		/* 68k RAM */
		if (do_high_byte)
			value |= clownmdemu->state->m68k_ram[(address + 0) & 0xFFFF] << 8;
		if (do_low_byte)
			value |= clownmdemu->state->m68k_ram[(address + 1) & 0xFFFF] << 0;
	}
	else
	{
		PrintError("Attempted to read invalid 68k address 0x%" CC_PRIXFAST32, address);
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

	if (address < 0x800000)
	{
		if ((address & 0x400000) == 0)
		{
			/* Cartridge */
			if (do_high_byte)
				frontend_callbacks->cartridge_written(frontend_callbacks->user_data, address + 0, high_byte);
			if (do_low_byte)
				frontend_callbacks->cartridge_written(frontend_callbacks->user_data, address + 1, low_byte);

			/* TODO - This is temporary, just to catch possible bugs in the 68k emulator */
			PrintError("Attempted to write to ROM address 0x%" CC_PRIXFAST32, address);
		}
		else if ((address & 0x20000) == 0)
		{
			/* Mega CD BIOS */
			PrintError("MAIN-CPU attempted to write to BIOS (0x%" CC_PRIXFAST32 ") at 0x%" CC_PRIXLEAST32, address, clownmdemu->state->m68k.program_counter);
		}
		else
		{
			/* PRG-RAM */
			if (!clownmdemu->state->m68k_has_mcd_m68k_bus)
			{
				PrintError("MAIN-CPU attempted to write to PRG-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
			}
			else
			{
				if ((address & 0x1FFFF) == 2)
					address = 0xFF420002;
				if (do_high_byte)
					clownmdemu->state->prg_ram[0x20000 * clownmdemu->state->prg_ram_bank + ((address + 0) & 0x1FFFF)] = high_byte;
				if (do_low_byte)
					clownmdemu->state->prg_ram[0x20000 * clownmdemu->state->prg_ram_bank + ((address + 1) & 0x1FFFF)] = low_byte;
			}
		}
	}
	else if (address < 0x80000)
	{
		/* WORD-RAM */
		if (clownmdemu->state->mcd_has_word_ram)
		{
			PrintError("MAIN-CPU attempted to write to WORD-RAM while SUB-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				clownmdemu->state->word_ram[(address + 0) & 0x3FFFF] = high_byte;
			if (do_low_byte)
				clownmdemu->state->word_ram[(address + 1) & 0x3FFFF] = low_byte;
		}
	}
	else if ((address >= 0xA00000 && address <= 0xA01FFF) || address == 0xA04000 || address == 0xA04002)
	{
		/* Z80 RAM and YM2612 */
		if (!clownmdemu->state->m68k_has_z80_bus)
		{
			PrintError("68k attempted to write Z80 memory/YM2612 ports without Z80 bus at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else if (clownmdemu->state->z80_reset)
		{
			PrintError("68k attempted to write Z80 memory/YM2612 ports while Z80 reset request was active at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized write of Z80 memory/YM2612 ports at 0x%" CC_PRIXLEAST32, clownmdemu->state->m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				Z80WriteCallbackWithCycle(user_data, (address & 0xFFFF) + 0, high_byte, target_cycle);
			else /*if (do_low_byte)*/
				Z80WriteCallbackWithCycle(user_data, (address & 0xFFFF) + 1, low_byte, target_cycle);
		}
	}
	else if (address >= 0xA10000 && address <= 0xA1001F)
	{
		/* I/O AREA */
		/* TODO */
		switch (address)
		{
			case 0xA10002:
			case 0xA10004:
			case 0xA10006:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = (address - 0xA10002) / 2;

					clownmdemu->state->joypads[joypad_index].data = low_byte & clownmdemu->state->joypads[joypad_index].control;
				}

				break;

			case 0xA10008:
			case 0xA1000A:
			case 0xA1000C:
				if (do_low_byte)
				{
					const cc_u16f joypad_index = (address - 0xA10008) / 2;

					clownmdemu->state->joypads[joypad_index].control = low_byte;
				}

				break;
		}
	}
	else if (address == 0xA11000)
	{
		/* MEMORY MODE */
		/* TODO: Make setting this to DRAM mode make the cartridge writeable. */
	}
	else if (address == 0xA11100)
	{
		/* Z80 BUSREQ */
		if (do_high_byte)
		{
			const cc_bool bus_request = (high_byte & 1) != 0;

			SyncZ80(clownmdemu, callback_user_data, target_cycle);

			clownmdemu->state->m68k_has_z80_bus = bus_request;
		}
	}
	else if (address == 0xA11200)
	{
		/* Z80 RESET */
		if (do_high_byte)
		{
			const cc_bool new_z80_reset = (high_byte & 1) == 0;

			if (clownmdemu->state->z80_reset && !new_z80_reset)
			{
				SyncZ80(clownmdemu, callback_user_data, target_cycle);
				Z80_Reset(&clownmdemu->z80);
				FM_State_Initialise(&clownmdemu->state->fm);
			}

			clownmdemu->state->z80_reset = new_z80_reset;
		}
	}
	else if (address == 0xA12000)
	{
		/* RESET, HALT */
		if (do_low_byte)
		{
			Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;

			const cc_bool interrupt = (high_byte & (1 << 0)) != 0;
			const cc_bool bus_request = (low_byte & (1 << 1)) != 0;
			const cc_bool reset = (low_byte & (1 << 0)) != 0;

			m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
			m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
			m68k_read_write_callbacks.user_data = callback_user_data;

			if (!clownmdemu->state->m68k_has_mcd_m68k_bus != bus_request)
				SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);

			if (!clownmdemu->state->mcd_m68k_reset && reset)
			{
				SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
				Clown68000_Reset(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);
			}

			if (interrupt)
			{
				SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
				Clown68000_Interrupt(clownmdemu->mcd_m68k, &m68k_read_write_callbacks, 2);
			}

			clownmdemu->state->m68k_has_mcd_m68k_bus = bus_request;
			clownmdemu->state->mcd_m68k_reset = reset;
		}
	}
	else if (address == 0xA12002)
	{
		/* Memory mode / Write protect */
		if (do_low_byte)
		{
			if ((low_byte & (1 << 1)) != 0)
			{
				SyncMCDM68k(clownmdemu, callback_user_data, target_cycle);
				clownmdemu->state->mcd_has_word_ram = cc_true;
			}

			clownmdemu->state->prg_ram_bank = (low_byte >> 6) & 3;
		}
	}
	else if (address == 0xA1200E)
	{
		/* Communication flag */
		if (do_high_byte)
			clownmdemu->state->mcd_communication_flag = (clownmdemu->state->mcd_communication_flag & 0x00FF) | (value & 0xFF00);

		if (do_low_byte)
			PrintError("MAIN-CPU attempted to write to SUB-CPU's communication flag at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address >= 0xA12010 && address < 0xA12020)
	{
		/* Communication command */
		cc_u16f mask;

		if (do_high_byte)
			mask |= 0xFF00;
		if (do_low_byte)
			mask |= 0x00FF;

		clownmdemu->state->mcd_communication_command[(address - 0xA12010) / 2] &= ~mask;
		clownmdemu->state->mcd_communication_command[(address - 0xA12010) / 2] |= value & mask;
	}
	else if (address >= 0xA12020 && address < 0xA12030)
	{
		/* Communication status */
		PrintError("MAIN-CPU attempted to write to SUB-CPU's communication status at 0x%" CC_PRIXLEAST32, clownmdemu->m68k->program_counter);
	}
	else if (address == 0xC00000 || address == 0xC00002 || address == 0xC00004 || address == 0xC00006)
	{
		if (address == 0xC00000 || address == 0xC00002)
		{
			/* VDP data port */
			VDP_WriteData(&clownmdemu->vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data);
		}
		else /*if (address == 0xC00004 || address == 0xC00006)*/
		{
			/* VDP control port */
			VDP_WriteControl(&clownmdemu->vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data, VDPReadCallback, &callback_user_data->data_and_callbacks);
		}
	}
	else if (address == 0xC00008)
	{
		/* H/V COUNTER */
		/* TODO */
	}
	else if (address >= 0xC00010 && address <= 0xC00016)
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
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		/* 68k RAM */
		if (do_high_byte)
			clownmdemu->state->m68k_ram[(address + 0) & 0xFFFF] = high_byte;
		if (do_low_byte)
			clownmdemu->state->m68k_ram[(address + 1) & 0xFFFF] = low_byte;
	}
	else
	{
		PrintError("Attempted to write invalid 68k address 0x%" CC_PRIXFAST32, address);
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
		value = clownmdemu->state->z80_ram[address];
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
		const cc_u32f m68k_address = ((cc_u32f)clownmdemu->state->z80_bank * 0x8000) + (address & 0x7FFE);

		SyncM68k(clownmdemu, callback_user_data, target_cycle);

		if ((address & 1) != 0)
			value = M68kReadCallbackWithCycle(user_data, m68k_address, cc_false, cc_true, target_cycle);
		else
			value = M68kReadCallbackWithCycle(user_data, m68k_address, cc_true, cc_false, target_cycle) >> 8;
	}
	else
	{
		PrintError("Attempted to read invalid Z80 address 0x%" CC_PRIXFAST16, address);
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
		clownmdemu->state->z80_ram[address] = value;
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
		clownmdemu->state->z80_bank >>= 1;
		clownmdemu->state->z80_bank |= (value & 1) != 0 ? 0x100 : 0;
	}
	else if (address == 0x7F11)
	{
		/* PSG (accessed through the 68k's bus). */
		SyncM68k(clownmdemu, callback_user_data, target_cycle);
		M68kWriteCallbackWithCycle(user_data, 0xC00010, cc_false, cc_true, value, target_cycle);
	}
	else if (address >= 0x8000)
	{
		/* TODO: Apparently Mamono Hunter Youko needs the Z80 to be able to write to 68k RAM in order to boot?
		   777 Casino also does weird stuff like this.
		   http://gendev.spritesmind.net/forum/viewtopic.php?f=24&t=347&start=30 */

		/* 68k ROM window (actually a window into the 68k's address space: you can access the PSG through it IIRC). */
		/* TODO: Apparently the Z80 can access the IO ports and send a bus request to itself. */
		const cc_u32f m68k_address = ((cc_u32f)clownmdemu->state->z80_bank * 0x8000) + (address & 0x7FFE);

		SyncM68k(clownmdemu, callback_user_data, target_cycle);

		if ((address & 1) != 0)
			M68kWriteCallbackWithCycle(user_data, m68k_address, cc_false, cc_true, value, target_cycle);
		else
			M68kWriteCallbackWithCycle(user_data, m68k_address, cc_true, cc_false, value << 8, target_cycle);
	}
	else
	{
		PrintError("Attempted to write invalid Z80 address 0x%" CC_PRIXFAST16, address);
	}
}

static void Z80WriteCallback(const void *user_data, cc_u16f address, cc_u16f value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	Z80WriteCallbackWithCycle(user_data, address, value, callback_user_data->z80_current_cycle);
}

/* MCD 68k (SUB-CPU) memory access callbacks */

static cc_u16f MCDM68kReadCallbackWithCycle(const void *user_data, cc_u32f address, cc_bool do_high_byte, cc_bool do_low_byte, const cc_u32f target_cycle)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	const ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;
	cc_u16f value = 0;

	if (/*address >= 0 &&*/ address < 0x80000)
	{
		/* PRG-RAM */
		if (address == 0x5F22)
		{
			PrintError("BIOS CALL DETECTED");
			value = 0x4E75;
		}
		else
		{
			if (do_high_byte)
				value |= clownmdemu->state->prg_ram[address + 0] << 8;
			if (do_low_byte)
				value |= clownmdemu->state->prg_ram[address + 1] << 0;
		}
	}
	else if (address < 0xC0000)
	{
		/* WORD-RAM */
		if (!clownmdemu->state->mcd_has_word_ram)
		{
			PrintError("SUB-CPU attempted to read from WORD-RAM while MAIN-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->mcd_m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				value |= clownmdemu->state->word_ram[(address + 0) & 0x3FFFF] << 8;
			if (do_low_byte)
				value |= clownmdemu->state->word_ram[(address + 1) & 0x3FFFF] << 0;
		}
	}
	else if (address == 0xFF800E)
	{
		/* Communication flag */
		value = clownmdemu->state->mcd_communication_flag;
	}
	else if (address >= 0xFF8010 && address < 0xFF8020)
	{
		/* Communication command */
		value = clownmdemu->state->mcd_communication_command[(address - 0xFF8010) / 2];
	}
	else if (address >= 0xFF8020 && address < 0xFF8030)
	{
		/* Communication status */
		value = clownmdemu->state->mcd_communication_status[(address - 0xFF8020) / 2];
	}
	else
	{
		PrintError("Attempted to read invalid MCD 68k address 0x%" CC_PRIXFAST32, address);
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
	const ClownMDEmu_Callbacks* const frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;

	const cc_u16f high_byte = (value >> 8) & 0xFF;
	const cc_u16f low_byte = (value >> 0) & 0xFF;

	if (/*address >= 0 &&*/ address < 0x80000)
	{
		/* PRG-RAM */
		if (do_high_byte)
			clownmdemu->state->prg_ram[address + 0] = high_byte;
		if (do_low_byte)
			clownmdemu->state->prg_ram[address + 1] = low_byte;
	}
	else if (address < 0xC0000)
	{
		/* WORD-RAM */
		if (!clownmdemu->state->mcd_has_word_ram)
		{
			PrintError("SUB-CPU attempted to write to WORD-RAM while MAIN-CPU has it at 0x%" CC_PRIXLEAST32, clownmdemu->state->mcd_m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				clownmdemu->state->word_ram[(address + 0) & 0x3FFFF] = high_byte;
			if (do_low_byte)
				clownmdemu->state->word_ram[(address + 1) & 0x3FFFF] = low_byte;
		}
	}
	else if (address == 0xFF8002)
	{
		/* Memory mode / Write protect */
		if (do_low_byte && (low_byte & (1 << 0)) != 0)
			clownmdemu->state->mcd_has_word_ram = cc_false;
	}
	else if (address == 0xFF800E)
	{
		/* Communication flag */
		if (do_high_byte)
			PrintError("SUB-CPU attempted to write to MAIN-CPU's communication flag at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);

		if (do_low_byte)
			clownmdemu->state->mcd_communication_flag = (clownmdemu->state->mcd_communication_flag & 0xFF00) | (value & 0x00FF);
	}
	else if (address >= 0xFF8010 && address < 0xFF8020)
	{
		/* Communication command */
		PrintError("SUB-CPU attempted to write to MAIN-CPU's communication command at 0x%" CC_PRIXLEAST32, clownmdemu->mcd_m68k->program_counter);
	}
	else if (address >= 0xFF8020 && address < 0xFF8030)
	{
		/* Communication status */
		cc_u16f mask;

		if (do_high_byte)
			mask |= 0xFF00;
		if (do_low_byte)
			mask |= 0x00FF;

		clownmdemu->state->mcd_communication_status[(address - 0xFF8020) / 2] &= ~mask;
		clownmdemu->state->mcd_communication_status[(address - 0xFF8020) / 2] |= value & mask;
	}
	else
	{
		PrintError("Attempted to write invalid MCD 68k address 0x%" CC_PRIXFAST32, address);
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

void ClownMDEmu_State_Initialise(ClownMDEmu_State *state)
{
	cc_u16f i;

	state->countdowns.m68k = 0;
	state->countdowns.z80 = 0;
	state->countdowns.mcd_m68k = 0;

	state->m68k_has_z80_bus = cc_true;
	state->z80_reset = cc_true;

	/* The standard Sega SDK bootcode uses this to detect soft-resets */
	for (i = 0; i < CC_COUNT_OF(state->joypads); ++i)
		state->joypads[i].control = 0;

	Z80_State_Initialise(&state->z80);
	VDP_State_Initialise(&state->vdp);
	FM_State_Initialise(&state->fm);
	PSG_State_Initialise(&state->psg);

	state->m68k_has_mcd_m68k_bus = cc_true;
	state->mcd_m68k_reset = cc_false;
	state->prg_ram_bank = 0;
	state->mcd_has_word_ram = cc_true;
}

void ClownMDEmu_Parameters_Initialise(ClownMDEmu *clownmdemu, const ClownMDEmu_Configuration *configuration, const ClownMDEmu_Constant *constant, ClownMDEmu_State *state)
{
	clownmdemu->configuration = configuration;
	clownmdemu->constant = constant;
	clownmdemu->state = state;

	clownmdemu->m68k = &state->m68k;

	clownmdemu->z80.constant = &constant->z80;
	clownmdemu->z80.state = &state->z80;

	clownmdemu->mcd_m68k = &state->mcd_m68k;

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
	cc_u16f scanline;
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
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

	/* Reload H-Int counter at the top of the screen, just like real hardware does */
	h_int_counter = clownmdemu->state->vdp.h_int_interval;

	clownmdemu->state->vdp.currently_in_vblank = cc_false;

	for (scanline = 0; scanline < television_vertical_resolution; ++scanline)
	{
		const cc_u32f current_cycle = cycles_per_scanline * (1 + scanline);

		/* Sync the 68k, since it's the one thing that can influence the VDP */
		SyncM68k(clownmdemu, &cpu_callback_user_data, current_cycle);

		/* Only render scanlines and generate H-Ints for scanlines that the console outputs to */
		if (scanline < console_vertical_resolution)
		{
			if (clownmdemu->state->vdp.double_resolution_enabled)
			{
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
}

void ClownMDEmu_Reset(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	CPUCallbackUserData callback_user_data;

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

void ClownMDEmu_SetErrorCallback(void (*error_callback)(const char *format, va_list arg))
{
	SetErrorCallback(error_callback);
	Clown68000_SetErrorCallback(error_callback);
}
