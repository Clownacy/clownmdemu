#ifndef CLOWNMDEMU_H
#define CLOWNMDEMU_H

#include <stddef.h>

#include "clowncommon.h"

/*#include "fm.h"*/
#include "m68k.h"
/*#include "psg.h"*/
#include "vdp.h"
/*#include "z80.h"*/

/* TODO - Documentation */

#define CLOWNMDEMU_ROM_BUFFER_SIZE (1024 * 1024 * 4) /* 4MiB */

enum
{
	CLOWNMDEMU_BUTTON_UP,
	CLOWNMDEMU_BUTTON_DOWN,
	CLOWNMDEMU_BUTTON_LEFT,
	CLOWNMDEMU_BUTTON_RIGHT,
	CLOWNMDEMU_BUTTON_A,
	CLOWNMDEMU_BUTTON_B,
	CLOWNMDEMU_BUTTON_C,
	CLOWNMDEMU_BUTTON_START
};

typedef struct ClownMDEmu_State
{
	cc_bool pal;
	cc_bool japanese;
	struct
	{
		unsigned char buffer[CLOWNMDEMU_ROM_BUFFER_SIZE];
		size_t size;
		cc_bool writeable;
	} rom;

	M68k_State m68k;
	unsigned char m68k_ram[0x10000];
	unsigned char z80_ram[0x2000];
	VDP_State vdp;
	unsigned char h_int_counter;
	struct
	{
		unsigned char control;
		unsigned char data;
	} joypads[3];
} ClownMDEmu_State;

void ClownMDEmu_Init(ClownMDEmu_State *state);
void ClownMDEmu_Deinit(ClownMDEmu_State *state);
void ClownMDEmu_Iterate(ClownMDEmu_State *state, void (*colour_updated_callback)(unsigned int index, unsigned int colour), void (*scanline_rendered_callback)(unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height), unsigned char (*read_input_callback)(unsigned int player_id, unsigned int button_id));
void ClownMDEmu_UpdateROM(ClownMDEmu_State *state, const unsigned char *rom_buffer, size_t rom_size);
void ClownMDEmu_SetROMWriteable(ClownMDEmu_State *state, cc_bool rom_writeable);
void ClownMDEmu_Reset(ClownMDEmu_State *state);
void ClownMDEmu_SetPAL(ClownMDEmu_State *state, cc_bool pal);
void ClownMDEmu_SetJapanese(ClownMDEmu_State *state, cc_bool japanese);

#endif /* CLOWNMDEMU_H */
