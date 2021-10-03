#ifndef VDP_H
#define VDP_H

#include <stddef.h>

#include "clowncommon.h"

#define VDP_MAX_SCANLINE_WIDTH 320
#define VDP_MAX_SCANLINES 240
#define VDP_INDEXED_PIXEL_VARIATION (1 << (1 + 2 + 4))

typedef struct VDP_State
{
	struct
	{
		cc_bool write_pending;
		unsigned short cached_write;

		cc_bool read_mode;
		unsigned short *selected_buffer; /* TODO - Pointers in the state are BAD: remove it! */
		unsigned short selected_buffer_size_mask;
		unsigned short index;
		unsigned short increment;
	} access;

	struct
	{
		cc_bool enabled;
		cc_bool pending;
		enum
		{
			VDP_DMA_MODE_MEMORY_TO_VRAM,
			VDP_DMA_MODE_FILL,
			VDP_DMA_MODE_COPY
		} mode;
		unsigned char source_address_high;
		unsigned short source_address_low;
		unsigned short length;
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

	cc_bool display_enabled; /* TODO - Actually use this */
	cc_bool v_int_enabled;
	cc_bool h_int_enabled;
	cc_bool h40_enabled;
	cc_bool v30_enabled;
	cc_bool shadow_highlight_enabled; /* TODO - This too */
	cc_bool interlace_mode_2_enabled; /* TODO - And this */

	unsigned char background_colour;
	unsigned char h_int_interval;
	cc_bool currently_in_vblank;

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
	/* http://gendev.spritesmind.net/forum/viewtopic.php?p=36727#p36727 */
	/* According to Mask of Destiny on SpritesMind, later models of Mega Drive (MD2 VA4 and later) have 64 words
	   of VSRAM, instead of the 40 words that earlier models have. This is convenient for me because 64 is a power
	   of 2, while 40 is not, which allows me to use bitmasks instead of slow modulos. */
	unsigned short vsram[64];

	struct
	{
		cc_bool needs_updating;
		struct VDP_SpriteCacheRow
		{
			unsigned char total;

			struct VDP_SpriteCacheEntry
			{
				unsigned char table_index;
				unsigned char y_in_sprite;
				unsigned char width;
				unsigned char height;
			} sprites[20];
		} rows[VDP_MAX_SCANLINES];
	} sprite_cache;

	unsigned char blit_lookup[VDP_INDEXED_PIXEL_VARIATION][VDP_INDEXED_PIXEL_VARIATION];
} VDP_State;

void VDP_Init(VDP_State *state);
void VDP_RenderScanline(VDP_State *state, unsigned short scanline, void (*scanline_rendered_callback)(unsigned short scanline, void *pixels, unsigned short screen_width, unsigned short screen_height));

unsigned short VDP_ReadData(VDP_State *state);
unsigned short VDP_ReadControl(VDP_State *state);
void VDP_WriteData(VDP_State *state, unsigned short value);
void VDP_WriteControl(VDP_State *state, unsigned short value, unsigned short (*read_callback)(void *user_data, unsigned long address), void *user_data);

#endif /* VDP_H */
