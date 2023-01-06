#ifndef CLOWNMDEMU_H
#define CLOWNMDEMU_H

#include <stdarg.h>
#include <stddef.h>

#include "clowncommon.h"

#include "fm.h"
#include "m68k.h"
#include "psg.h"
#include "vdp.h"
#include "z80.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO - Documentation */

#define CLOWNMDEMU_PARAMETERS_INITIALISE(CONFIGURATION, CONSTANT, STATE) { \
		(CONFIGURATION), \
		(CONSTANT), \
		(STATE), \
\
		&(STATE)->m68k, \
\
		{ \
			&(CONSTANT)->z80, \
			&(STATE)->z80 \
		}, \
\
		{ \
			&(CONFIGURATION)->vdp, \
			&(CONSTANT)->vdp, \
			&(STATE)->vdp \
		}, \
\
		FM_PARAMETERS_INITIALISE( \
			&(CONSTANT)->fm, \
			&(STATE)->fm \
		), \
\
		{ \
			&(CONSTANT)->psg, \
			&(STATE)->psg \
		} \
	}

#define CLOWNMDEMU_MASTER_CLOCK_NTSC 53693175
#define CLOWNMDEMU_MASTER_CLOCK_PAL  53203424

#define CLOWNMDEMU_M68K_CLOCK_DIVIDER 7
#define CLOWNMDEMU_M68K_CLOCK_NTSC (CLOWNMDEMU_MASTER_CLOCK_NTSC / CLOWNMDEMU_M68K_CLOCK_DIVIDER)
#define CLOWNMDEMU_M68K_CLOCK_PAL  (CLOWNMDEMU_MASTER_CLOCK_PAL / CLOWNMDEMU_M68K_CLOCK_DIVIDER)

#define CLOWNMDEMU_Z80_CLOCK_DIVIDER 15
#define CLOWNMDEMU_Z80_CLOCK_NTSC (CLOWNMDEMU_MASTER_CLOCK_NTSC / CLOWNMDEMU_Z80_CLOCK_DIVIDER)
#define CLOWNMDEMU_Z80_CLOCK_PAL  (CLOWNMDEMU_MASTER_CLOCK_PAL / CLOWNMDEMU_Z80_CLOCK_DIVIDER)

/* 6 for the hardcoded prescale, 6 for the number of channels, and 4 for the number of operators per channel. */
#define CLOWNMDEMU_FM_SAMPLE_RATE_DIVIDER (6 * 6 * 4)
#define CLOWNMDEMU_FM_SAMPLE_RATE_NTSC (CLOWNMDEMU_M68K_CLOCK_NTSC / CLOWNMDEMU_FM_SAMPLE_RATE_DIVIDER)
#define CLOWNMDEMU_FM_SAMPLE_RATE_PAL  (CLOWNMDEMU_M68K_CLOCK_PAL / CLOWNMDEMU_FM_SAMPLE_RATE_DIVIDER)

#define CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER 16
#define CLOWNMDEMU_PSG_SAMPLE_RATE_NTSC (CLOWNMDEMU_Z80_CLOCK_NTSC / CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER)
#define CLOWNMDEMU_PSG_SAMPLE_RATE_PAL  (CLOWNMDEMU_Z80_CLOCK_PAL / CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER)

/* The NTSC framerate is 59.94FPS (60 divided by 1.001) */
#define CLOWNMDEMU_MULTIPLY_BY_NTSC_FRAMERATE(x) ((x) * (60 * 1000) / 1001)
#define CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(x) (((x) / 60) + ((x) / (60 * 1000)))

/* The PAL framerate is 50FPS */
#define CLOWNMDEMU_MULTIPLY_BY_PAL_FRAMERATE(x) ((x) * 50)
#define CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(x) ((x) / 50)

typedef enum ClownMDEmu_Button
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
} ClownMDEmu_Button;

typedef enum ClownMDEmu_Region
{
	CLOWNMDEMU_REGION_DOMESTIC, /* Japanese */
	CLOWNMDEMU_REGION_OVERSEAS  /* Elsewhere */
} ClownMDEmu_Region;

typedef enum ClownMDEmu_TVStandard
{
	CLOWNMDEMU_TV_STANDARD_NTSC, /* 60Hz */
	CLOWNMDEMU_TV_STANDARD_PAL   /* 50Hz */
} ClownMDEmu_TVStandard;

typedef struct ClownMDEmu_Configuration
{
	struct
	{
		ClownMDEmu_Region region;
		ClownMDEmu_TVStandard tv_standard;
	} general;

	VDP_Configuration vdp;
} ClownMDEmu_Configuration;

typedef struct ClownMDEmu_Constant
{
	Z80_Constant z80;
	VDP_Constant vdp;
	FM_Constant fm;
	PSG_Constant psg;
} ClownMDEmu_Constant;

typedef struct ClownMDEmu_State
{
	struct
	{
		cc_u16f m68k;
		cc_u16f z80;
	} countdowns;
	M68k_State m68k;
	Z80_State z80;
	cc_u8l m68k_ram[0x10000];
	cc_u8l z80_ram[0x2000];
	VDP_State vdp;
	cc_u8l h_int_counter;
	FM_State fm;
	PSG_State psg;
	struct
	{
		cc_u8l control;
		cc_u8l data;
	} joypads[3];
	cc_u16f z80_bank;
	cc_bool m68k_has_z80_bus;
	cc_bool z80_reset;
} ClownMDEmu_State;

typedef struct ClownMDEmu
{
	const ClownMDEmu_Configuration *configuration;
	const ClownMDEmu_Constant *constant;
	ClownMDEmu_State *state;
	M68k_State *m68k;
	Z80 z80;
	VDP vdp;
	FM fm;
	PSG psg;
} ClownMDEmu;

typedef struct ClownMDEmu_Callbacks
{
	const void *user_data;

	cc_u16f (*cartridge_read)(const void *user_data, cc_u32f address);
	void (*cartridge_written)(const void *user_data, cc_u32f address, cc_u16f value);
	void (*colour_updated)(const void *user_data, cc_u16f index, cc_u16f colour);
	void (*scanline_rendered)(const void *user_data, cc_u16f scanline, const cc_u8l *pixels, cc_u16f screen_width, cc_u16f screen_height);
	cc_bool (*input_requested)(const void *user_data, cc_u16f player_id, ClownMDEmu_Button button_id);
	void (*fm_audio_to_be_generated)(const void *user_data, size_t total_frames, void (*generate_fm_audio)(const ClownMDEmu *clownmdemu, cc_s16l *sample_buffer, size_t total_frames));
	void (*psg_audio_to_be_generated)(const void *user_data, size_t total_samples, void (*generate_psg_audio)(const ClownMDEmu *clownmdemu, cc_s16l *sample_buffer, size_t total_samples));
} ClownMDEmu_Callbacks;

void ClownMDEmu_Constant_Initialise(ClownMDEmu_Constant *constant);
void ClownMDEmu_State_Initialise(ClownMDEmu_State *state);
void ClownMDEmu_Parameters_Initialise(ClownMDEmu *clownmdemu, const ClownMDEmu_Configuration *configuration, const ClownMDEmu_Constant *constant, ClownMDEmu_State *state);
void ClownMDEmu_Iterate(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks);
void ClownMDEmu_Reset(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks);
void ClownMDEmu_SetErrorCallback(void (*error_callback)(const char *format, va_list arg));

#ifdef __cplusplus
}
#endif

#endif /* CLOWNMDEMU_H */
