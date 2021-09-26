#ifndef VDP_H
#define VDP_H

#include <stddef.h>

#include "clowncommon.h"

#define MAX_SCANLINE_WIDTH 320

typedef struct VDP_State
{
	struct
	{
		cc_bool write_pending;
		unsigned short cached_write;

		cc_bool read_mode;
		unsigned short *selected_buffer;
		unsigned short selected_buffer_size_mask;
		unsigned short index;
		unsigned short increment;
	} access;

	struct
	{
		enum
		{
			VDP_DMA_MODE_MEMORY_TO_VRAM,
			VDP_DMA_MODE_FILL,
			VDP_DMA_MODE_COPY
		} mode;
		unsigned long source_address;
		unsigned short length;
		cc_bool awaiting_destination_address;
	} dma;

	unsigned short plane_a_address;
	unsigned short plane_b_address;
	unsigned short window_address;
	unsigned short sprite_table_address;
	unsigned short hscroll_address;

	unsigned short plane_width;
	unsigned short plane_height;
	unsigned short plane_width_bitmask;
	unsigned short plane_height_bitmask;

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
void VDP_WriteControl(VDP_State *state, unsigned short value, unsigned short (*read_callback)(void *user_data, unsigned long address), void *user_data);

#endif /* VDP_H */
