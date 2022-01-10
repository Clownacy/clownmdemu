#include "clownmdemu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon.h"

#include "error.h"
/*#include "fm.h"*/
#include "m68k.h"
#include "psg.h"
#include "vdp.h"
/*#include "z80.h"*/

typedef struct CallbackUserData
{
	ClownMDEmu_State *state;
	void (*colour_updated_callback)(unsigned int index, unsigned int colour);
	unsigned char (*read_input_callback)(unsigned int player_id, unsigned int button_id);
	void (*psg_audio_callback)(short *samples, size_t total_samples);
	unsigned int current_cycle;
	unsigned int psg_previous_cycle;
} CallbackUserData;

static GenerateAndPlayPSGSamples(ClownMDEmu_State *state, void (*psg_audio_callback)(short *samples, size_t total_samples), size_t total_samples)
{
	/* TODO - PAL */
	short buffer[CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_NTSC) / 15 / 16];

	assert(total_samples <= CC_COUNT_OF(buffer));

	memset(buffer, 0, sizeof(buffer));

	PSG_Update(&state->psg, buffer, total_samples);

	psg_audio_callback(buffer, total_samples);
}

/* VDP memory access callback */

static unsigned int VDPReadCallback(void *user_data, unsigned long address)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;
	unsigned int value = 0;

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		value |= state->rom.buffer[address + 0] << 8;
		value |= state->rom.buffer[address + 1] << 0;
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

static unsigned int M68kReadCallback(void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte)
{
	CallbackUserData *callback_user_data = (CallbackUserData*)user_data;
	ClownMDEmu_State *state = callback_user_data->state;
	unsigned int value = 0;

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		if (do_high_byte)
			value |= state->rom.buffer[address + 0] << 8;
		if (do_low_byte)
			value |= state->rom.buffer[address + 1] << 0;
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized read of Z80 memory at 0x%lX at 0x%lX", address, state->m68k.program_counter);
		}
		else
		{
			address -= 0xA00000;

			if (do_high_byte)
				value |= state->z80_ram[address + 0] << 8;
			else if (do_low_byte)
				value |= state->z80_ram[address + 1] << 0;
		}
	}
	else if (address == 0xA04000)
	{
		/* TODO - YM2612 A0 + D0 */
	}
	else if (address == 0xA04002)
	{
		/* TODO - YM2612 A1 + D1 */
	}
	else if (address >= 0xA10000 && address <= 0xA1001F)
	{
		/* TODO - I/O AREA */
		switch (address)
		{
			case 0xA10000:
				if (do_low_byte)
					value |= (!state->japanese << 7) | (state->pal << 6) | (1 << 5);	/* Bit 5 set = no Mega CD attached */

				break;

			case 0xA10002:
			case 0xA10004:
				if (do_low_byte)
				{
					const unsigned int joypad_index = (address - 0xA10002) / 2;

					value |= state->joypads[joypad_index].data;

					if (value & 0x40)
					{
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_C) << 5;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_B) << 4;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_RIGHT) << 3;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_LEFT) << 2;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_DOWN) << 1;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_UP) << 0;
					}
					else
					{
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_START) << 5;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_A) << 4;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_DOWN) << 1;
						value |= !callback_user_data->read_input_callback(joypad_index, CLOWNMDEMU_BUTTON_UP) << 0;
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

					value = state->joypads[joypad_index].control;
				}

				break;
		}
	}
	else if (address == 0xA11000)
	{
		/* TODO - MEMORY MODE */
	}
	else if (address == 0xA11100)
	{
		/* TODO - Z80 BUSREQ */
	}
	else if (address == 0xA11200)
	{
		/* TODO - Z80 RESET */
	}
	else if (address == 0xC00000 || address == 0xC00002)
	{
		/* TODO - Reading from the data port causes real Mega Drives to crash (if the VDP isn't in read mode) */
		value = VDP_ReadData(&state->vdp);
	}
	else if (address == 0xC00004 || address == 0xC00006)
	{
		value = VDP_ReadControl(&state->vdp);
	}
	else if (address == 0xC00008)
	{
		/* TODO - H/V COUNTER */
	}
	else if (address >= 0xC00010 && address <= 0xC00016)
	{
		/* TODO - PSG */
		/* What's supposed to happen here, if you read from the PSG? */
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		if (do_high_byte)
			value |= state->m68k_ram[(address + 0) & 0xFFFF] << 8;
		if (do_low_byte)
			value |= state->m68k_ram[(address + 1) & 0xFFFF] << 0;
	}
	else
	{
		PrintError("68k attempted to read invalid memory 0x%lX at 0x%lX", address, state->m68k.program_counter);
	}

	return value;
}

static void M68kWriteCallback(void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte, unsigned int value)
{
	CallbackUserData *callback_user_data = (CallbackUserData*)user_data;
	ClownMDEmu_State *state = callback_user_data->state;

	const unsigned char high_byte = (unsigned char)((value >> 8) & 0xFF);
	const unsigned char low_byte = (unsigned char)((value >> 0) & 0xFF);

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		/*
		if (do_high_byte)
			state->rom.buffer[address + 0] = high_byte;
		if (do_low_byte)
			state->rom.buffer[address + 1] = low_byte;
		*/

		PrintError("68k attempted to write to ROM 0x%lX at 0x%lX", address, state->m68k.program_counter);
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		if (do_high_byte && do_low_byte)
		{
			PrintError("68k attempted to perform word-sized write of Z80 memory at 0x%lX", state->m68k.program_counter);
		}
		else
		{
			address -= 0xA00000;

			if (do_high_byte)
				state->z80_ram[address + 0] = high_byte;
			else if (do_low_byte)
				state->z80_ram[address + 1] = low_byte;
		}
	}
	else if (address == 0xA04000)
	{
		/* TODO - YM2612 A0 + D0 */
	}
	else if (address == 0xA04002)
	{
		/* TODO - YM2612 A1 + D1 */
	}
	else if (address >= 0xA10000 && address <= 0xA1001F)
	{
		/* TODO - I/O AREA */
		switch (address)
		{
			case 0xA10002:
			case 0xA10004:
			case 0xA10006:
				if (do_low_byte)
				{
					const unsigned int joypad_index = (address - 0xA10002) / 2;

					state->joypads[joypad_index].data = low_byte & state->joypads[joypad_index].control;
				}

				break;

			case 0xA10008:
			case 0xA1000A:
			case 0xA1000C:
				if (do_low_byte)
				{
					const unsigned int joypad_index = (address - 0xA10008) / 2;

					state->joypads[joypad_index].control = low_byte;
				}

				break;
		}
	}
	else if (address == 0xA11000)
	{
		/* TODO - MEMORY MODE */
	}
	else if (address == 0xA11100)
	{
		/* TODO - Z80 BUSREQ */
	}
	else if (address == 0xA11200)
	{
		/* TODO - Z80 RESET */
	}
	else if (address == 0xC00000 || address == 0xC00002)
	{
		VDP_WriteData(&state->vdp, value, callback_user_data->colour_updated_callback);
	}
	else if (address == 0xC00004 || address == 0xC00006)
	{
		VDP_WriteControl(&state->vdp, value, callback_user_data->colour_updated_callback, VDPReadCallback, state);
	}
	else if (address == 0xC00008)
	{
		/* TODO - H/V COUNTER */
	}
	else if (address >= 0xC00010 && address <= 0xC00016)
	{
		/* PSG */

		if (do_low_byte)
		{
			/* Update the PSG up until this point in time */
			GenerateAndPlayPSGSamples(callback_user_data->state, callback_user_data->psg_audio_callback, (callback_user_data->current_cycle - callback_user_data->psg_previous_cycle) / 15 / 16);
			callback_user_data->psg_previous_cycle = callback_user_data->current_cycle;

			/* Alter the PSG's state */
			PSG_DoCommand(&state->psg, low_byte);
		}
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		if (do_high_byte)
			state->m68k_ram[(address + 0) & 0xFFFF] = high_byte;
		if (do_low_byte)
			state->m68k_ram[(address + 1) & 0xFFFF] = low_byte;
	}
	else
	{
		PrintError("68k attempted to write invalid memory 0x%lX at 0x%lX", address, state->m68k.program_counter);
	}
}

void ClownMDEmu_Init(ClownMDEmu_State *state)
{
	unsigned int i;

	state->countdowns.m68k = 0;
	state->countdowns.z80 = 0;

	/* The standard Sega SDK bootcode uses this to detect soft-resets */
	for (i = 0; i < CC_COUNT_OF(state->joypads); ++i)
		state->joypads[i].control = 0;

	VDP_Init(&state->vdp);
	PSG_Init(&state->psg);
}

void ClownMDEmu_Deinit(ClownMDEmu_State *state)
{
	/* No idea */
	(void)state;
}

void ClownMDEmu_Iterate(ClownMDEmu_State *state, void (*colour_updated_callback)(unsigned int index, unsigned int colour), void (*scanline_rendered_callback)(unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height), unsigned char (*read_input_callback)(unsigned int player_id, unsigned int button_id), void (*psg_audio_callback)(short *samples, size_t total_samples))
{
	unsigned int scanline, i;
	M68k_ReadWriteCallbacks m68k_read_write_callbacks;
	CallbackUserData callback_user_data;

	const unsigned int television_vertical_resolution = state->pal ? 312 : 262; /* PAL and NTSC, respectively */
	const unsigned int console_vertical_resolution = (state->vdp.v30_enabled ? 30 : 28) * 8; /* 240 and 224 */
	const unsigned int cycles_per_scanline = (state->pal ? CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_PAL) : CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_NTSC)) / television_vertical_resolution;

	callback_user_data.state = state;
	callback_user_data.colour_updated_callback = colour_updated_callback;
	callback_user_data.read_input_callback = read_input_callback;
	callback_user_data.psg_audio_callback = psg_audio_callback;
	callback_user_data.current_cycle = 0;
	callback_user_data.psg_previous_cycle = 0;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &callback_user_data;

	/*ReadInput(state);*/

	/* Reload H-Int counter at the top of the screen, just like real hardware does */
	state->h_int_counter = state->vdp.h_int_interval;

	state->vdp.currently_in_vblank = cc_false;

	for (scanline = 0; scanline < television_vertical_resolution; ++scanline)
	{
		/* Run the 68k and Z80 for a scanline's worth of cycles */
		for (i = 0; i < cycles_per_scanline; ++i)
		{
			/* 68k */
			if (state->countdowns.m68k == 0)
			{
				state->countdowns.m68k = 7 * 10; /* TODO - The x10 is a temporary hack to get the 68k to run roughly at the correct speed until instruction cycle durations are added */

				M68k_DoCycle(&state->m68k, &m68k_read_write_callbacks);
			}

			--state->countdowns.m68k;

			/* Z80 */
			if (state->countdowns.z80 == 0)
			{
				state->countdowns.z80 = 15;

				/*Z80_DoCycle(&state->z80);*/
			}

			--state->countdowns.z80;

			++callback_user_data.current_cycle;
		}

		/* Only render scanlines and generate H-Ints for scanlines that the console outputs to */
		if (scanline < console_vertical_resolution)
		{
			if (state->vdp.interlace_mode_2_enabled)
			{
				VDP_RenderScanline(&state->vdp, scanline * 2, scanline_rendered_callback);
				VDP_RenderScanline(&state->vdp, scanline * 2 + 1, scanline_rendered_callback);
			}
			else
			{
				VDP_RenderScanline(&state->vdp, scanline, scanline_rendered_callback);
			}

			/* Fire a H-Int if we've reached the requested line */
			if (state->h_int_counter-- == 0)
			{
				state->h_int_counter = state->vdp.h_int_interval;

				/* Do H-Int */
				if (state->vdp.h_int_enabled)
					M68k_Interrupt(&state->m68k, &m68k_read_write_callbacks, 4);
			}
		}

		/* Check if we have reached the end of the console-output scanlines */
		if (scanline == console_vertical_resolution)
		{
			/* Do V-Int */
			if (state->vdp.v_int_enabled)
				M68k_Interrupt(&state->m68k, &m68k_read_write_callbacks, 6);

			/* Flag that we have entered the V-blank region */
			state->vdp.currently_in_vblank = cc_true;
		}
	}

	/*UpdateFM(state);*/

	/* Update the PSG for the rest of this frame */
	GenerateAndPlayPSGSamples(state, psg_audio_callback, (callback_user_data.current_cycle - callback_user_data.psg_previous_cycle) / 15 / 16);
}

/* TODO - Replace this with a system where the emulator queries the frontend for data from the cartridge */
void ClownMDEmu_UpdateROM(ClownMDEmu_State *state, const unsigned char *rom_buffer, size_t rom_size)
{
	if (rom_size > sizeof(state->rom.buffer))
	{
		PrintError("Provided ROM was too big for the internal buffer");
	}
	else
	{
		memcpy(state->rom.buffer, rom_buffer, rom_size);
		state->rom.size = rom_size;
	}
}

void ClownMDEmu_SetROMWriteable(ClownMDEmu_State *state, cc_bool rom_writeable)
{
	state->rom.writeable = !!rom_writeable; /* Convert to boolean */
}

void ClownMDEmu_Reset(ClownMDEmu_State *state)
{
	M68k_ReadWriteCallbacks m68k_read_write_callbacks;
	CallbackUserData callback_user_data;

	/* TODO - Please rethink this */
	callback_user_data.state = state;
	callback_user_data.colour_updated_callback = NULL;
	callback_user_data.read_input_callback = NULL;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &callback_user_data;

	M68k_Reset(&state->m68k, &m68k_read_write_callbacks);
}

void ClownMDEmu_SetPAL(ClownMDEmu_State *state, cc_bool pal)
{
	state->pal = !!pal;
}

void ClownMDEmu_SetJapanese(ClownMDEmu_State *state, cc_bool japanese)
{
	state->japanese = !!japanese;
}
