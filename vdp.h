#ifndef VDP_H
#define VDP_H

#include <stddef.h>

#include "clowncommon.h"

#define MAX_SCANLINE_WIDTH 320

typedef struct VDP_State
{
	struct
	{
		cc_bool read_mode;
		unsigned short *selected_buffer;
		size_t selected_buffer_size;
		size_t index;
		size_t increment;

		cc_bool awaiting_dma_destination_address;

		cc_bool write_pending;
		unsigned short cached_write;
	} access;

	size_t plane_a_address;
	size_t plane_b_address;
	size_t window_address;
	size_t sprite_table_address;
	size_t hscroll_address;

	size_t plane_width;
	size_t plane_height;
	size_t plane_width_bitmask;
	size_t plane_height_bitmask;

	size_t screen_width;
	size_t screen_height;

	enum
	{
		VDP_HSCROLL_MODE_FULL,
		VDP_HSCROLL_MODE_1CELL,
		VDP_HSCROLL_MODE_1LINE
	} hscroll_mode;

	enum
	{
		VDP_VSCROLL_MODE_FULL,
		VDP_VSCROLL_MODE_2CELL
	} vscroll_mode;

	unsigned short vram[0x8000];
	unsigned short cram[4 * 16];
	unsigned short vsram[MAX_SCANLINE_WIDTH / 16];
} VDP_State;

void VDP_Init(VDP_State *state);
void VDP_RenderScanline(VDP_State *state, size_t scanline, void (*scanline_rendered_callback)(size_t scanline, void *pixels, size_t screen_width, size_t screen_height));

unsigned short VDP_ReadData(VDP_State *state);
unsigned short VDP_ReadControl(VDP_State *state);
void VDP_WriteData(VDP_State *state, unsigned short value);
void VDP_WriteControl(VDP_State *state, unsigned short value);

#endif /* VDP_H */
