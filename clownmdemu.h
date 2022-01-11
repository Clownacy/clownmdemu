#ifndef CLOWNMDEMU_H
#define CLOWNMDEMU_H

#include <stddef.h>

#include "clowncommon.h"

/*#include "fm.h"*/
#include "m68k.h"
#include "psg.h"
#include "vdp.h"
/*#include "z80.h"*/

/* TODO - Documentation */

#define CLOWNMDEMU_MASTER_CLOCK_NTSC 53693175
#define CLOWNMDEMU_MASTER_CLOCK_PAL  53203424

/* The NTSC framerate is 59.94FPS (60 divided by 1.001) */
#define CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(x) (((x) / 60) + ((x) / (60 * 1000)))

/* The PAL framerate is 50FPS */
#define CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(x) ((x) / 50)

enum
{
	CLOWNMDEMU_BUTTON_UP,
	CLOWNMDEMU_BUTTON_DOWN,
	CLOWNMDEMU_BUTTON_LEFT,
	CLOWNMDEMU_BUTTON_RIGHT,
	CLOWNMDEMU_BUTTON_A,
	CLOWNMDEMU_BUTTON_B,
	CLOWNMDEMU_BUTTON_C,
	CLOWNMDEMU_BUTTON_START,
	CLOWNMDEMU_BUTTON_MAX
};

typedef struct ClownMDEmu_State
{
	cc_bool pal;
	cc_bool japanese;
	struct
	{
		unsigned int m68k;
		unsigned int z80;
	} countdowns;
	M68k_State m68k;
	unsigned char m68k_ram[0x10000];
	unsigned char z80_ram[0x2000];
	VDP_State vdp;
	unsigned char h_int_counter;
	PSG_State psg;
	struct
	{
		unsigned char control;
		unsigned char data;
	} joypads[3];
} ClownMDEmu_State;

typedef struct ClownMDEmu_Callbacks
{
	void *user_data;

	unsigned int (*cartridge_read)(void *user_data, unsigned long address);
	void (*cartridge_written)(void *user_data, unsigned long address, unsigned int value);
	void (*colour_updated)(void *user_data, unsigned int index, unsigned int colour);
	void (*scanline_rendered)(void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height);
	cc_bool (*input_requested)(void *user_data, unsigned int player_id, unsigned int button_id);
	void (*psg_audio_generated)(void *user_data, short *samples, size_t total_samples);
} ClownMDEmu_Callbacks;

void ClownMDEmu_Init(ClownMDEmu_State *state);
void ClownMDEmu_Deinit(ClownMDEmu_State *state);
void ClownMDEmu_Iterate(ClownMDEmu_State *state, ClownMDEmu_Callbacks *callbacks);
void ClownMDEmu_Reset(ClownMDEmu_State *state, ClownMDEmu_Callbacks *callbacks);
void ClownMDEmu_SetPAL(ClownMDEmu_State *state, cc_bool pal);
void ClownMDEmu_SetJapanese(ClownMDEmu_State *state, cc_bool japanese);

#endif /* CLOWNMDEMU_H */
