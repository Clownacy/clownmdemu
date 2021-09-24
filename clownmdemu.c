#include "clownmdemu.h"

#include <stddef.h>
#include <string.h>

#include "clowncommon.h"

#include "error.h"
/*#include "fm.h"*/
#include "m68k.h"
/*#include "psg.h"*/
/*#include "vdp.h"*/
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
} ClownMDEmu_State;

static unsigned char M68kReadCallback(void *user_data, unsigned long address)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;

	address &= 0xFFFFFF; /* The 68k's address bus is 24-bit */

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		return state->rom.buffer[address];
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		return state->z80_ram[address & 0x1FFF];
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		return state->m68k_ram[address & 0xFFFF];
	}
	else
	{
		PrintError("68k attempted to read invalid memory at 0x%X", address);
		return 0;
	}
}

static void M68kWriteCallback(void *user_data, unsigned long address, unsigned char value)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)user_data;

	address &= 0xFFFFFF; /* The 68k's address bus is 24-bit */

	if (/*address >= 0 &&*/ address < state->rom.size)
	{
		/*state->rom.buffer[address] = value;*/
		PrintError("68k attempted to write to ROM at 0x%X", address);
	}
	else if (address >= 0xA00000 && address <= 0xA01FFF)
	{
		state->z80_ram[address & 0x1FFF] = value;
	}
	else if (address >= 0xE00000 && address <= 0xFFFFFF)
	{
		state->m68k_ram[address & 0xFFFF] = value;
	}
	else
	{
		PrintError("68k attempted to write invalid memory at 0x%X", address);
	}
}

void ClownMDEmu_Init(void *state_void)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	/* Initialise VDP lookup table */
	(void)state;/*InitVDP(state);*/
}

void ClownMDEmu_Deinit(void *state_void)
{
	ClownMDEmu_State *state = (ClownMDEmu_State*)state_void;

	/* No idea */
	(void)state;
}

void ClownMDEmu_Iterate(void *state_void, void (*video_callback)(void *pixels, size_t width, size_t height))
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

		/*DoHInt(state);*/
		/*RenderScanline(state);*/
	}

	/*DoVInt(state);*/
	/*UpdateFM(state);*/
	/*UpdatePSG(state);*/
	(void)video_callback;/*video_callback();*/
	/*PresentFinalImageToUser(state);*/
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
