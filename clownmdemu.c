#include "clownmdemu.h"

#include <stddef.h>
#include <string.h>

#include "clowncommon.h"

#include "error.h"
/*#include "fm.h"*/
#include "m68k.h"
/*#include "psg.h"*/
#include "vdp.h"
/*#include "z80.h"*/

#define MASTER_CLOCK_NTSC 53693175
#define MASTER_CLOCK_PAL  53203424

/* TODO - alternate vertical resolutions */
#define VERTICAL_RESOLUTION 224

#define ROM_BUFFER_SIZE (1024 * 1024 * 4) /* 4MiB */

typedef struct ClownMDEmu_State
{
	cc_bool pal;
	cc_bool japanese;
	struct
	{
		unsigned char buffer[ROM_BUFFER_SIZE];
		size_t size;
		cc_bool writeable;
	} rom;

	M68k_State m68k;
	unsigned char m68k_ram[0x10000];
	unsigned char z80_ram[0x2000];
	VDP_State vdp;
} ClownMDEmu_State;

/* VDP memory access callback */

static unsigned short VDPReadCallback(void *user_data, unsigned long address)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;
	unsigned short value = 0;

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
		PrintError("VDP attempted to read invalid memory at 0x%X", address);
	}

	return value;
}

/* 68k memory access callbacks */

static unsigned short M68kReadCallback(void *user_data, unsigned long address, cc_bool high_byte, cc_bool low_byte)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;
	unsigned short value = 0;

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		if (high_byte)
			value |= state->rom.buffer[address + 0] << 8;
		if (low_byte)
			value |= state->rom.buffer[address + 1] << 0;
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		if (high_byte && low_byte)
		{
			PrintError("68k attempted to perform word-sized read of Z80 memory");
		}
		else
		{
			address -= 0xA00000;

			if (high_byte)
				value |= state->z80_ram[address + 0] << 8;
			else if (low_byte)
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
	else if (address == 0xC00010)
	{
		/* TODO - PSG */
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		if (high_byte)
			value |= state->m68k_ram[(address + 0) & 0xFFFF] << 8;
		if (low_byte)
			value |= state->m68k_ram[(address + 1) & 0xFFFF] << 0;
	}
	else
	{
		PrintError("68k attempted to read invalid memory at 0x%X", address);
	}

	return value;
}

static void M68kWriteCallback(void *user_data, unsigned long address, cc_bool high_byte, cc_bool low_byte, unsigned short value)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		/*
		if (high_byte)
			state->rom.buffer[address + 0] = (unsigned char)(value >> 8) & 0xFF;
		if (low_byte)
			state->rom.buffer[address + 1] = (unsigned char)(value >> 0) & 0xFF;
		*/

		PrintError("68k attempted to write to ROM at 0x%X", address);
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		if (high_byte && low_byte)
		{
			PrintError("68k attempted to perform word-sized write of Z80 memory");
		}
		else
		{
			address -= 0xA00000;

			if (high_byte)
				state->z80_ram[address + 0] = (unsigned char)(value >> 8) & 0xFF;
			else if (low_byte)
				state->z80_ram[address + 1] = (unsigned char)(value >> 0) & 0xFF;
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
		VDP_WriteData(&state->vdp, value);
	}
	else if (address == 0xC00004 || address == 0xC00006)
	{
		VDP_WriteControl(&state->vdp, value, VDPReadCallback, state);
	}
	else if (address == 0xC00008)
	{
		/* TODO - H/V COUNTER */
	}
	else if (address == 0xC00010)
	{
		/* TODO - PSG */
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		if (high_byte)
			state->m68k_ram[(address + 0) & 0xFFFF] = (unsigned char)(value >> 8) & 0xFF;
		if (low_byte)
			state->m68k_ram[(address + 1) & 0xFFFF] = (unsigned char)(value >> 0) & 0xFF;
	}
	else
	{
		PrintError("68k attempted to write invalid memory at 0x%X", address);
	}
}

void ClownMDEmu_Init(void *state_void)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	VDP_Init(&state->vdp);
}

void ClownMDEmu_Deinit(void *state_void)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	/* No idea */
	(void)state;
}

void ClownMDEmu_Iterate(void *state_void, void (*scanline_rendered_callback)(unsigned short scanline, void *pixels, unsigned short screen_width, unsigned short screen_height))
{
	/* TODO - user callbacks for reading input and showing video */

	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;
	size_t i, j;
	M68k_ReadWriteCallbacks m68k_read_write_callbacks;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = state;

	/*ReadInput(state);*/

	for (i = 0; i < VERTICAL_RESOLUTION; ++i)
	{
		for (j = 0; j < (state->pal ? MASTER_CLOCK_PAL : MASTER_CLOCK_NTSC) / (state->pal ? 50 : 60) / VERTICAL_RESOLUTION / 7 / 2; ++j)
		{
			/* TODO - Not completely accurate: the 68k is MASTER_CLOCK/7, but the Z80 is MASTER_CLOCK/15.
			   I'll need to find a common multiple to make this accurate. */
			M68k_DoCycle(&state->m68k, &m68k_read_write_callbacks);
			M68k_DoCycle(&state->m68k, &m68k_read_write_callbacks);
			/*DoZ80Cycle(state);*/
		}

		VDP_RenderScanline(&state->vdp, i, scanline_rendered_callback);
		/* Do H-Int */
		M68k_Interrupt(&state->m68k, &m68k_read_write_callbacks, 4);
	}

	/* Do V-Int */
	M68k_Interrupt(&state->m68k, &m68k_read_write_callbacks, 6);
	/*UpdateFM(state);*/
	/*UpdatePSG(state);*/
}

/* TODO - Replace this with a function that retrieves a pointer to the internal buffer, to avoid a needless memcpy */
void ClownMDEmu_UpdateROM(void *state_void, const unsigned char *rom_buffer, size_t rom_size)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	if (rom_size > ROM_BUFFER_SIZE)
	{
		PrintError("Provided ROM was too big for the internal buffer");
	}
	else
	{
		memcpy(state->rom.buffer, rom_buffer, rom_size);
		state->rom.size = rom_size;
	}
}

void ClownMDEmu_SetROMWriteable(void *state_void, unsigned char rom_writeable)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	state->rom.writeable = !!rom_writeable; /* Convert to boolean */
}

void ClownMDEmu_Reset(void *state_void)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;
	M68k_ReadWriteCallbacks m68k_read_write_callbacks;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = state;

	M68k_Reset(&state->m68k, &m68k_read_write_callbacks);
}

void ClownMDEmu_SetPAL(void *state_void, unsigned char pal)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	state->pal = !!pal;
}

void ClownMDEmu_SetJapanese(void *state_void, unsigned char japanese)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	state->japanese = !!japanese;
}

size_t ClownMDEmu_GetStateSize(void)
{
	return sizeof(ClownMDEmu_State);
}
