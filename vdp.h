#ifndef VDP_H
#define VDP_H

#include <stddef.h>

#include "clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VDP_MAX_SCANLINE_WIDTH 320
#define VDP_MAX_SCANLINES (240 * 2) /* V30 in interlace mode 2 */

typedef struct VDP_Configuration
{
	cc_bool sprites_disabled;
	cc_bool planes_disabled[2];
} VDP_Configuration;

typedef struct VDP_Constant
{
	unsigned char blit_lookup[1 << (1 + 1 + 2 + 4)][1 << (1 + 2 + 4)];
	unsigned char blit_lookup_shadow_highlight[1 << (1 + 1 + 2 + 4)][1 << (1 + 2 + 4)];
} VDP_Constant;

typedef struct VDP_State
{
	struct
	{
		cc_bool write_pending;
		unsigned short cached_write;

		enum
		{
			VDP_ACCESS_VRAM,
			VDP_ACCESS_CRAM,
			VDP_ACCESS_VSRAM
		} selected_buffer;

		cc_bool read_mode;
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

	cc_bool display_enabled;
	cc_bool v_int_enabled;
	cc_bool h_int_enabled;
	cc_bool h40_enabled;
	cc_bool v30_enabled;
	cc_bool shadow_highlight_enabled;
	cc_bool double_resolution_enabled;

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
	   of VSRAM, instead of the 40 words that earlier models have. */
	unsigned short vsram[64];

	unsigned short sprite_table_cache[80][2];

	struct
	{
		cc_bool needs_updating;
		struct VDP_SpriteRowCacheRow
		{
			unsigned char total;

			struct VDP_SpriteRowCacheEntry
			{
				unsigned char table_index;
				unsigned char y_in_sprite;
				unsigned char width;
				unsigned char height;
			} sprites[20];
		} rows[VDP_MAX_SCANLINES];
	} sprite_row_cache;
} VDP_State;

typedef struct VDP
{
	const VDP_Configuration *configuration;
	const VDP_Constant *constant;
	VDP_State *state;
} VDP;

void VDP_Constant_Initialise(VDP_Constant *constant);
void VDP_State_Initialise(VDP_State *state);
void VDP_RenderScanline(const VDP *vdp, unsigned int scanline, void (*scanline_rendered_callback)(void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height), void *scanline_rendered_callback_user_data);

unsigned int VDP_ReadData(const VDP *vdp);
unsigned int VDP_ReadControl(const VDP *vdp);
void VDP_WriteData(const VDP *vdp, unsigned int value, void (*colour_updated_callback)(void *user_data, unsigned int index, unsigned int colour), void *colour_updated_callback_user_data);
void VDP_WriteControl(const VDP *vdp, unsigned int value, void (*colour_updated_callback)(void *user_data, unsigned int index, unsigned int colour), void *colour_updated_callback_user_data, unsigned int (*read_callback)(void *user_data, unsigned long address), void *read_callback_user_data);

#ifdef __cplusplus
}
#endif

#endif /* VDP_H */
