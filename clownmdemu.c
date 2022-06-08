#include "clownmdemu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon.h"

#include "error.h"
#include "fm.h"
#include "m68k.h"
#include "psg.h"
#include "vdp.h"
#include "z80.h"

#define MAX_ROM_SIZE (1024 * 1024 * 4) /* 4MiB */

typedef struct DataAndCallbacks
{
	ClownMDEmu *data;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} DataAndCallbacks;

typedef struct CPUCallbackUserData
{
	DataAndCallbacks data_and_callbacks;
	unsigned int current_cycle;
	unsigned int fm_previous_cycle;
	unsigned int psg_previous_cycle;
} CPUCallbackUserData;

static void GenerateFMAudio(ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_frames)
{
	const FM fm = {&clownmdemu->constant->fm, &clownmdemu->state->fm};

	FM_Update(&fm, sample_buffer, total_frames);
}

static void GenerateAndPlayFMSamples(CPUCallbackUserData *m68k_callback_user_data)
{
	const unsigned int fm_current_cycle = m68k_callback_user_data->current_cycle / (CLOWNMDEMU_M68K_CLOCK_DIVIDER * CLOWNMDEMU_FM_SAMPLE_RATE_DIVIDER);

	const size_t samples_to_generate = fm_current_cycle - m68k_callback_user_data->fm_previous_cycle;

	if (samples_to_generate != 0)
	{
		m68k_callback_user_data->data_and_callbacks.frontend_callbacks->fm_audio_to_be_generated(m68k_callback_user_data->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GenerateFMAudio);

		m68k_callback_user_data->fm_previous_cycle = fm_current_cycle;
	}
}

static void GeneratePSGAudio(ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_samples)
{
	const PSG psg = {&clownmdemu->constant->psg, &clownmdemu->state->psg};

	PSG_Update(&psg, sample_buffer, total_samples);
}

static void GenerateAndPlayPSGSamples(CPUCallbackUserData *m68k_callback_user_data)
{
	const unsigned int psg_current_cycle = m68k_callback_user_data->current_cycle / (CLOWNMDEMU_Z80_CLOCK_DIVIDER * CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER);

	const size_t samples_to_generate = psg_current_cycle - m68k_callback_user_data->psg_previous_cycle;

	if (samples_to_generate != 0)
	{
		m68k_callback_user_data->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated(m68k_callback_user_data->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GeneratePSGAudio);

		m68k_callback_user_data->psg_previous_cycle = psg_current_cycle;
	}
}

/* VDP memory access callback */

static unsigned int VDPReadCallback(void *user_data, unsigned long address)
{
	DataAndCallbacks *data_and_callbacks = (DataAndCallbacks*)user_data;
	ClownMDEmu_State *state = data_and_callbacks->data->state;
	const ClownMDEmu_Callbacks *frontend_callbacks = data_and_callbacks->frontend_callbacks;
	unsigned int value = 0;

	if (/*address >= 0 &&*/ address < MAX_ROM_SIZE)
	{
		value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 0) << 8;
		value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 1) << 0;
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		value |= state->m68k_ram[(address + 0) & 0xFFFF] << 8;
		value |= state->m68k_ram[(address + 1) & 0xFFFF] << 0;
	}
	else
	{
		PrintError("VDP attempted to read invalid memory 0x%lX", address);
	}

	return value;
}

/* 68k memory access callbacks */

static unsigned int Z80ReadCallback(void *user_data, unsigned int address);
static void Z80WriteCallback(void *user_data, unsigned int address, unsigned int value);

static unsigned int M68kReadCallback(void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte)
{
	CPUCallbackUserData *callback_user_data = (CPUCallbackUserData*)user_data;
	ClownMDEmu *clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks *frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;
	unsigned int value = 0;

	if (/*address >= 0 &&*/ address < MAX_ROM_SIZE)
	{
		/* Cartridge */
		if (do_high_byte)
			value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 0) << 8;
		if (do_low_byte)
			value |= frontend_callbacks->cartridge_read(frontend_callbacks->user_data, address + 1) << 0;
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		/* Z80 RAM */
		if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized read of Z80 memory at 0x%lX at 0x%lX", address, clownmdemu->state->m68k.program_counter);
		}
		else
		{
			address -= 0xA00000;

			if (do_high_byte)
				value |= clownmdemu->state->z80_ram[address + 0] << 8;
			else if (do_low_byte)
				value |= clownmdemu->state->z80_ram[address + 1] << 0;
		}
	}
	else if (address == 0xA04000)
	{
		/* YM2612 A0 + D0 */
		/* TODO */
	}
	else if (address == 0xA04002)
	{
		/* YM2612 A1 + D1 */
		/* TODO */
	}
	else if (address >= 0xA10000 && address <= 0xA1001F)
	{
		/* I/O AREA */
		/* TODO */
		switch (address)
		{
			case 0xA10000:
				if (do_low_byte)
					value |= ((clownmdemu->configuration->general.region == CLOWNMDEMU_REGION_OVERSEAS) << 7) | ((clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL) << 6) | (1 << 5);	/* Bit 5 set = no Mega CD attached */

				break;

			case 0xA10002:
			case 0xA10004:
				if (do_low_byte)
				{
					const unsigned int joypad_index = (address - 0xA10002) / 2;

					value |= clownmdemu->state->joypads[joypad_index].data;

					if (value & 0x40)
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
					const unsigned int joypad_index = (address - 0xA10008) / 2;

					value = clownmdemu->state->joypads[joypad_index].control;
				}

				break;
		}
	}
	else if (address == 0xA11000)
	{
		/* MEMORY MODE */
		/* TODO */
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
	else if (address == 0xC00000 || address == 0xC00002 || address == 0xC00004 || address == 0xC00006)
	{
		const VDP vdp = {&clownmdemu->configuration->vdp, &clownmdemu->constant->vdp, &clownmdemu->state->vdp};

		if (address == 0xC00000 || address == 0xC00002)
		{
			/* VDP data port */
			/* TODO - Reading from the data port causes real Mega Drives to crash (if the VDP isn't in read mode) */
			value = VDP_ReadData(&vdp);
		}
		else /*if (address == 0xC00004 || address == 0xC00006)*/
		{
			/* VDP control port */
			value = VDP_ReadControl(&vdp);
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
		PrintError("Attempted to read invalid 68k address 0x%lX", address);
	}

	return value;
}

static void M68kWriteCallback(void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte, unsigned int value)
{
	CPUCallbackUserData *callback_user_data = (CPUCallbackUserData*)user_data;
	ClownMDEmu *clownmdemu = callback_user_data->data_and_callbacks.data;
	const ClownMDEmu_Callbacks *frontend_callbacks = callback_user_data->data_and_callbacks.frontend_callbacks;

	const unsigned char high_byte = (unsigned char)((value >> 8) & 0xFF);
	const unsigned char low_byte = (unsigned char)((value >> 0) & 0xFF);

	if (/*address >= 0 &&*/ address < MAX_ROM_SIZE)
	{
		/* Cartridge */
		if (do_high_byte)
			frontend_callbacks->cartridge_written(frontend_callbacks->user_data, address + 0, high_byte);
		if (do_low_byte)
			frontend_callbacks->cartridge_written(frontend_callbacks->user_data, address + 1, low_byte);

		/* TODO - This is temporary, just to catch possible bugs in the 68k emulator */
		PrintError("Attempted to write to ROM address 0x%lX", address);
	}
	else if ((address >= 0xA00000 && address <= 0xA01FFF) || (address == 0xA04000 || address == 0xA04002))
	{
		/* Z80 RAM and YM2612 */
		if (!clownmdemu->state->m68k_has_z80_bus)
		{
			PrintError("68k attempted to access Z80 memory/YM2612 ports without Z80 bus at 0x%lX", clownmdemu->state->m68k.program_counter);
		}
		else if (clownmdemu->state->z80_reset)
		{
			PrintError("68k attempted to access Z80 memory/YM2612 ports while Z80 reset request was active at 0x%lX", clownmdemu->state->m68k.program_counter);
		}
		else if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized write of Z80 memory/YM2612 ports at 0x%lX", clownmdemu->state->m68k.program_counter);
		}
		else
		{
			if (do_high_byte)
				Z80WriteCallback(user_data, (address & 0xFFFE) + 0, high_byte);
			else /*if (do_low_byte)*/
				Z80WriteCallback(user_data, (address & 0xFFFE) + 1, low_byte);
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
					const unsigned int joypad_index = (address - 0xA10002) / 2;

					clownmdemu->state->joypads[joypad_index].data = low_byte & clownmdemu->state->joypads[joypad_index].control;
				}

				break;

			case 0xA10008:
			case 0xA1000A:
			case 0xA1000C:
				if (do_low_byte)
				{
					const unsigned int joypad_index = (address - 0xA10008) / 2;

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
			clownmdemu->state->m68k_has_z80_bus = (high_byte & 1) != 0;
	}
	else if (address == 0xA11200)
	{
		/* Z80 RESET */
		if (do_high_byte)
		{
			const cc_bool new_z80_reset = (high_byte & 1) == 0;

			if (clownmdemu->state->z80_reset && !new_z80_reset)
			{
				const Z80 z80 = {&clownmdemu->constant->z80, &clownmdemu->state->z80};

				Z80_Reset(&z80);
			}

			clownmdemu->state->z80_reset = new_z80_reset;
		}
	}
	else if (address == 0xC00000 || address == 0xC00002 || address == 0xC00004 || address == 0xC00006)
	{
		const VDP vdp = {&clownmdemu->configuration->vdp, &clownmdemu->constant->vdp, &clownmdemu->state->vdp};

		if (address == 0xC00000 || address == 0xC00002)
		{
			/* VDP data port */
			VDP_WriteData(&vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data);
		}
		else /*if (address == 0xC00004 || address == 0xC00006)*/
		{
			/* VDP control port */
			VDP_WriteControl(&vdp, value, frontend_callbacks->colour_updated, frontend_callbacks->user_data, VDPReadCallback, &callback_user_data->data_and_callbacks);
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
			const PSG psg = {&clownmdemu->constant->psg, &clownmdemu->state->psg};

			/* Update the PSG up until this point in time */
			GenerateAndPlayPSGSamples(callback_user_data);

			/* Alter the PSG's state */
			PSG_DoCommand(&psg, low_byte);
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
		PrintError("Attempted to write invalid 68k address 0x%lX", address);
	}
}

/* Z80 memory access callbacks */

static unsigned int Z80ReadCallback(void *user_data, unsigned int address)
{
	const CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	/* I suppose, on read hardware, in an open-bus situation, this would actually
	   be a static variable that retains its value from the last call. */
	unsigned int value;

	value = 0;

	if (address < 0x2000)
	{
		value = clownmdemu->state->z80_ram[address];
	}
	else if (address >= 0x4000 && address <= 0x4003)
	{
		/* YM2612 */
		/* TODO */
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
		const unsigned long m68k_address = ((unsigned long)clownmdemu->state->z80_bank * 0x8000) + (address & 0x7FFE);

		if ((address & 1) != 0)
			value = M68kReadCallback(user_data, m68k_address, cc_false, cc_true);
		else
			value = M68kReadCallback(user_data, m68k_address, cc_true, cc_false) >> 8;
	}

	return value;
}

static void Z80WriteCallback(void *user_data, unsigned int address, unsigned int value)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;
	ClownMDEmu* const clownmdemu = callback_user_data->data_and_callbacks.data;

	if (address < 0x2000)
	{
		clownmdemu->state->z80_ram[address] = value;
	}
	else if (address >= 0x4000 && address <= 0x4003)
	{
		/* YM2612 */
		const FM fm = {&clownmdemu->constant->fm, &clownmdemu->state->fm};
		const unsigned int port = (address & 2) != 0 ? 1 : 0;

		/* Update the FM up until this point in time. */
		GenerateAndPlayFMSamples(callback_user_data);

		if ((address & 1) == 0)
			FM_DoAddress(&fm, port, value);
		else
			FM_DoData(&fm, value);
	}
	else if (address == 0x6000 || address == 0x6001)
	{
		clownmdemu->state->z80_bank >>= 1;
		clownmdemu->state->z80_bank |= (value & 1) != 0 ? 0x100 : 0;
	}
	else if (address == 0x7F11)
	{
		/* PSG (accessed through the 68k's bus). */
		M68kWriteCallback(user_data, 0xC00010, cc_false, cc_true, value);
	}
	else if (address >= 0x8000)
	{
		/* 68k ROM window (actually a window into the 68k's address space: you can access the PSG through it IIRC). */
		const unsigned long m68k_address = ((unsigned long)clownmdemu->state->z80_bank * 0x8000) + (address & 0x7FFE);

		if ((address & 1) != 0)
			M68kWriteCallback(user_data, m68k_address, cc_false, cc_true, value);
		else
			M68kWriteCallback(user_data, m68k_address, cc_true, cc_false, value << 8);
	}
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
	unsigned int i;

	state->countdowns.m68k = 0;
	state->countdowns.z80 = 0;

	state->m68k_has_z80_bus = cc_true;
	state->z80_reset = cc_true;

	/* The standard Sega SDK bootcode uses this to detect soft-resets */
	for (i = 0; i < CC_COUNT_OF(state->joypads); ++i)
		state->joypads[i].control = 0;

	Z80_State_Initialise(&state->z80);
	VDP_State_Initialise(&state->vdp);
	FM_State_Initialise(&state->fm);
	PSG_State_Initialise(&state->psg);
}

void ClownMDEmu_Iterate(ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks)
{
	unsigned int scanline, i;
	M68k_ReadWriteCallbacks m68k_read_write_callbacks;
	Z80_ReadAndWriteCallbacks z80_read_write_callbacks;
	CPUCallbackUserData cpu_callback_user_data;

	const unsigned int television_vertical_resolution = clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? 312 : 262; /* PAL and NTSC, respectively */
	const unsigned int console_vertical_resolution = (clownmdemu->state->vdp.v30_enabled ? 30 : 28) * 8; /* 240 and 224 */
	const unsigned int cycles_per_scanline = (clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_PAL) : CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_NTSC)) / television_vertical_resolution;
	const Z80 z80 = {&clownmdemu->constant->z80, &clownmdemu->state->z80};

	cpu_callback_user_data.data_and_callbacks.data = clownmdemu;
	cpu_callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;
	cpu_callback_user_data.current_cycle = 0;
	cpu_callback_user_data.fm_previous_cycle = 0;
	cpu_callback_user_data.psg_previous_cycle = 0;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &cpu_callback_user_data;

	z80_read_write_callbacks.read.callback = Z80ReadCallback;
	z80_read_write_callbacks.read.user_data = &cpu_callback_user_data;
	z80_read_write_callbacks.write.callback = Z80WriteCallback;
	z80_read_write_callbacks.write.user_data = &cpu_callback_user_data;

	/*ReadInput(state);*/

	/* Reload H-Int counter at the top of the screen, just like real hardware does */
	clownmdemu->state->h_int_counter = clownmdemu->state->vdp.h_int_interval;

	clownmdemu->state->vdp.currently_in_vblank = cc_false;

	for (scanline = 0; scanline < television_vertical_resolution; ++scanline)
	{
		/* Run the 68k and Z80 for a scanline's worth of cycles */
		for (i = 0; i < cycles_per_scanline; ++i)
		{
			/* 68k */
			if (clownmdemu->state->countdowns.m68k == 0)
			{
				clownmdemu->state->countdowns.m68k = CLOWNMDEMU_M68K_CLOCK_DIVIDER * 10; /* TODO - The x10 is a temporary hack to get the 68k to run roughly at the correct speed until instruction cycle durations are added */

				M68k_DoCycle(&clownmdemu->state->m68k, &m68k_read_write_callbacks);
			}

			--clownmdemu->state->countdowns.m68k;

			/* Z80 */
			if (clownmdemu->state->countdowns.z80 == 0)
			{
				clownmdemu->state->countdowns.z80 = CLOWNMDEMU_Z80_CLOCK_DIVIDER * 8; /* TODO: A similar temporary hack. */

				if (!clownmdemu->state->m68k_has_z80_bus)
					Z80_DoCycle(&z80, &z80_read_write_callbacks);
			}

			--clownmdemu->state->countdowns.z80;

			++cpu_callback_user_data.current_cycle;
		}

		/* Only render scanlines and generate H-Ints for scanlines that the console outputs to */
		if (scanline < console_vertical_resolution)
		{
			const VDP vdp = {&clownmdemu->configuration->vdp, &clownmdemu->constant->vdp, &clownmdemu->state->vdp};

			if (clownmdemu->state->vdp.double_resolution_enabled)
			{
				VDP_RenderScanline(&vdp, scanline * 2, callbacks->scanline_rendered, callbacks->user_data);
				VDP_RenderScanline(&vdp, scanline * 2 + 1, callbacks->scanline_rendered, callbacks->user_data);
			}
			else
			{
				VDP_RenderScanline(&vdp, scanline, callbacks->scanline_rendered, callbacks->user_data);
			}

			/* Fire a H-Int if we've reached the requested line */
			if (clownmdemu->state->h_int_counter-- == 0)
			{
				clownmdemu->state->h_int_counter = clownmdemu->state->vdp.h_int_interval;

				/* Do H-Int */
				if (clownmdemu->state->vdp.h_int_enabled)
					M68k_Interrupt(&clownmdemu->state->m68k, &m68k_read_write_callbacks, 4);
			}
		}

		/* Check if we have reached the end of the console-output scanlines */
		if (scanline == console_vertical_resolution)
		{
			/* Do V-Int */
			if (clownmdemu->state->vdp.v_int_enabled)
			{
				M68k_Interrupt(&clownmdemu->state->m68k, &m68k_read_write_callbacks, 6);
				Z80_Interrupt(&z80, &z80_read_write_callbacks);
			}

			/* Flag that we have entered the V-blank region */
			clownmdemu->state->vdp.currently_in_vblank = cc_true;
		}
	}

	/* Update the FM for the rest of this frame */
	GenerateAndPlayFMSamples(&cpu_callback_user_data);

	/* Update the PSG for the rest of this frame */
	GenerateAndPlayPSGSamples(&cpu_callback_user_data);
}

void ClownMDEmu_Reset(ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks)
{
	const Z80 z80 = {&clownmdemu->constant->z80, &clownmdemu->state->z80};

	M68k_ReadWriteCallbacks m68k_read_write_callbacks;
	CPUCallbackUserData callback_user_data;

	callback_user_data.data_and_callbacks.data = clownmdemu;
	callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &callback_user_data;

	M68k_Reset(&clownmdemu->state->m68k, &m68k_read_write_callbacks);
	Z80_Reset(&z80);
}

void ClownMDEmu_SetErrorCallback(void (*error_callback)(const char *format, va_list arg))
{
	SetErrorCallback(error_callback);
}
