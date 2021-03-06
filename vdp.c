#include "vdp.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon.h"

#include "error.h"

enum
{
	SHADOW_HIGHLIGHT_NORMAL = 0 << 6,
	SHADOW_HIGHLIGHT_SHADOW = 1 << 6,
	SHADOW_HIGHLIGHT_HIGHLIGHT = 2 << 6
};

/* Some of the logic here is based on research done by Nemesis:
   https://gendev.spritesmind.net/forum/viewtopic.php?p=21016#p21016 */

static void WriteAndIncrement(VDP_State *state, unsigned int value, void (*colour_updated_callback)(const void *user_data, unsigned int index, unsigned int colour), const void *colour_updated_callback_user_data)
{
	const unsigned int index = state->access.index / 2;

	switch (state->access.selected_buffer)
	{
		default:
			/* Should never happen. */
			assert(0);
			break;

		case VDP_ACCESS_VRAM:
		{
			/* Update sprite cache if we're writing to the sprite table */
			const unsigned int sprite_table_index = index - state->sprite_table_address;

			if (sprite_table_index < (state->h40_enabled ? 80u : 64u) * 4u && (sprite_table_index & 2) == 0)
			{
				state->sprite_table_cache[sprite_table_index / 4][sprite_table_index & 1] = (unsigned short)value;
				state->sprite_row_cache.needs_updating = cc_true;
			}

			state->vram[index % CC_COUNT_OF(state->vram)] = (unsigned short)value;

			break;
		}

		case VDP_ACCESS_CRAM:
		{
			/* Remove garbage bits */
			const unsigned int colour = value & 0xEEE;

			/* Fit index to within CRAM */
			const unsigned int index_wrapped = index % CC_COUNT_OF(state->cram);

			/* Store regular Mega Drive-format colour (with garbage bits intact) */
			state->cram[index_wrapped] = (unsigned short)value;

			/* Now let's precompute the shadow/normal/highlight colours in
			   RGB444 (so we don't have to calculate them during blitting)
			   and send them to the frontend for further optimisation */

			/* Create normal colour */
			/* (repeat the upper bit in the lower bit so that the full 4-bit colour range is covered) */
			colour_updated_callback(colour_updated_callback_user_data, SHADOW_HIGHLIGHT_NORMAL + index_wrapped, colour | ((colour & 0x888) >> 3));

			/* Create shadow colour */
			/* (divide by two and leave in lower half of colour range) */
			colour_updated_callback(colour_updated_callback_user_data, SHADOW_HIGHLIGHT_SHADOW + index_wrapped, colour >> 1);

			/* Create highlight colour */
			/* (divide by two and move to upper half of colour range) */
			colour_updated_callback(colour_updated_callback_user_data, SHADOW_HIGHLIGHT_HIGHLIGHT + index_wrapped, 0x888 + (colour >> 1));

			break;
		}

		case VDP_ACCESS_VSRAM:
			state->vsram[index % CC_COUNT_OF(state->vsram)] = (unsigned short)value;
			break;
	}

	state->access.index += state->access.increment;
}

static unsigned int ReadAndIncrement(VDP_State *state)
{
	unsigned int value;

	const unsigned int index = state->access.index / 2;

	switch (state->access.selected_buffer)
	{
		default:
			/* Should never happen. */
			assert(0);
			/* Fallthrough */
		case VDP_ACCESS_VRAM:
			value = state->vram[index % CC_COUNT_OF(state->vram)];
			break;

		case VDP_ACCESS_CRAM:
			value = state->cram[index % CC_COUNT_OF(state->cram)];
			break;

		case VDP_ACCESS_VSRAM:
			value = state->vsram[index % CC_COUNT_OF(state->vsram)];
			break;
	}

	state->access.index += state->access.increment;

	return value;
}

void VDP_Constant_Initialise(VDP_Constant *constant)
{
	/* This essentially pre-computes the VDP's depth-test and alpha-test,
	   generating a lookup table to eliminate the need to perform these
	   every time a pixel is blitted. This provides a *massive* speed boost. */
	const unsigned int palette_line_index_mask = 0xF;
	const unsigned int colour_index_mask = 0x3F;
	const unsigned int priority_mask = 0x40;
	const unsigned int not_shadowed_mask = 0x80;

	unsigned int old_pixel, new_pixel;

	for (old_pixel = 0; old_pixel < CC_COUNT_OF(constant->blit_lookup); ++old_pixel)
	{
		for (new_pixel = 0; new_pixel < CC_COUNT_OF(constant->blit_lookup[0]); ++new_pixel)
		{
			const unsigned int old_palette_line_index = old_pixel & palette_line_index_mask;
			const unsigned int old_colour_index = old_pixel & colour_index_mask;
			const cc_bool old_priority = (old_pixel & priority_mask) != 0;
			const cc_bool old_not_shadowed = (old_pixel & not_shadowed_mask) != 0;

			const unsigned int new_palette_line_index = new_pixel & palette_line_index_mask;
			const unsigned int new_colour_index = new_pixel & colour_index_mask;
			const cc_bool new_priority = (new_pixel & priority_mask) != 0;
			const cc_bool new_not_shadowed = new_priority;

			const cc_bool draw_new_pixel = new_palette_line_index != 0 && (old_palette_line_index == 0 || !old_priority || new_priority);

			unsigned int output;

			/* First, generate the table for regular blitting */
			output = draw_new_pixel ? new_pixel : old_pixel;

			output |= old_not_shadowed || new_not_shadowed ? not_shadowed_mask : 0;

			constant->blit_lookup[old_pixel][new_pixel] = (unsigned char)output;

			/* Now, generate the table for shadow/highlight blitting */
			if (draw_new_pixel)
			{
				/* Sprite goes on top of plane */
				if (new_colour_index == 0x3E)
				{
					/* Transparent highlight pixel */
					output = old_colour_index | (old_not_shadowed ? SHADOW_HIGHLIGHT_HIGHLIGHT : SHADOW_HIGHLIGHT_NORMAL);
				}
				else if (new_colour_index == 0x3F)
				{
					/* Transparent shadow pixel */
					output = old_colour_index | SHADOW_HIGHLIGHT_SHADOW;
				}
				else if (new_palette_line_index == 0xE)
				{
					/* Always-normal pixel */
					output = new_colour_index | SHADOW_HIGHLIGHT_NORMAL;
				}
				else
				{
					/* Regular sprite pixel */
					output = new_colour_index | (new_not_shadowed || old_not_shadowed ? SHADOW_HIGHLIGHT_NORMAL : SHADOW_HIGHLIGHT_SHADOW);
				}
			}
			else
			{
				/* Plane goes on top of sprite */
				output = old_colour_index | (old_not_shadowed ? SHADOW_HIGHLIGHT_NORMAL : SHADOW_HIGHLIGHT_SHADOW);
			}

			constant->blit_lookup_shadow_highlight[old_pixel][new_pixel] = (unsigned char)output;
		}
	}
}

void VDP_State_Initialise(VDP_State *state)
{
	state->access.write_pending = cc_false;

	state->access.read_mode = cc_false;
	state->access.selected_buffer = VDP_ACCESS_VRAM;
	state->access.index = 0;
	state->access.increment = 0;

	state->dma.enabled = cc_false;
	state->dma.pending = cc_false;
	state->dma.source_address_high = 0;
	state->dma.source_address_low = 0;

	state->plane_a_address = 0;
	state->plane_b_address = 0;
	state->window_address = 0;
	state->sprite_table_address = 0;
	state->hscroll_address = 0;

	state->plane_width = 32;
	state->plane_height = 32;
	state->plane_width_bitmask = state->plane_width - 1;
	state->plane_height_bitmask = state->plane_height - 1;

	state->display_enabled = cc_false;
	state->v_int_enabled = cc_false;
	state->h_int_enabled = cc_false;
	state->h40_enabled = cc_false;
	state->v30_enabled = cc_false;
	state->shadow_highlight_enabled = cc_false;
	state->double_resolution_enabled = cc_false;

	state->background_colour = 0;
	state->h_int_interval = 0;
	state->currently_in_vblank = 0;

	state->hscroll_mode = VDP_HSCROLL_MODE_FULL;
	state->vscroll_mode = VDP_VSCROLL_MODE_FULL;

	state->sprite_row_cache.needs_updating = cc_true;
}

void VDP_RenderScanline(const VDP *vdp, unsigned int scanline, void (*scanline_rendered_callback)(const void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height), const void *scanline_rendered_callback_user_data)
{
	unsigned int i;

	/* The original hardware has a bug where if you use V-scroll and H-scroll at the same time,
	   the partially off-screen leftmost column will use an invalid V-scroll value.
	   To simulate this, 16 extra pixels are rendered at the left size of the scanline.
	   Depending on the H-scroll value, these pixels may appear on the left side of the screen.
	   This is done by offsetting where the pixels start being written to the buffer.
	   This is why the buffer has an extra 16 bytes at the beginning, and an extra 15 bytes at
	   the end.
	   Also of note is that this function renders a tile's entire width, regardless of whether
	   it's fully on-screen or not. This is why VDP_MAX_SCANLINE_WIDTH is rounded up to 8.
	   In both cases, these extra bytes exist to catch the 'overflow' values that are written
	   outside the visible portion of the buffer. */
	unsigned char plane_metapixels[16 + (VDP_MAX_SCANLINE_WIDTH + (8 - 1)) / 8 * 8 + (16 - 1)];

	const unsigned int tile_height_power = vdp->state->double_resolution_enabled ? 4 : 3;

	assert(scanline < VDP_MAX_SCANLINES);

	/* Fill the scanline buffer with the background colour */
	memset(plane_metapixels, vdp->state->background_colour, sizeof(plane_metapixels));

	if (vdp->state->display_enabled)
	{
		/* ***** *
		 * Setup *
		 * ***** */

		#define MAX_SPRITE_WIDTH (8 * 4)

		const unsigned int tile_height_mask = (1 << tile_height_power) - 1;
		const unsigned int tile_size = (8 << tile_height_power) / 4;

		unsigned int sprite_limit = vdp->state->h40_enabled ? 20 : 16;
		unsigned int pixel_limit = vdp->state->h40_enabled ? 320 : 256;

		/* The padding bytes of the left and right are for allowing sprites to overdraw at the
		   edges of the screen. */
		unsigned char sprite_metapixels[(MAX_SPRITE_WIDTH - 1) + VDP_MAX_SCANLINE_WIDTH + (MAX_SPRITE_WIDTH - 1)];


		/* *********** */
		/* Draw planes */
		/* *********** */
		for (i = 2 ; i-- > 0; )
		{
			if (!vdp->configuration->planes_disabled[i])
			{
				/* The extra two tiles on the left of the scanline */
				const unsigned int EXTRA_TILES = 2;

				unsigned int j;
				unsigned char *metapixels_pointer;
				unsigned int hscroll;
				unsigned int hscroll_scroll_offset;
				unsigned int plane_x_offset;

				/* Get the horizontal scroll value */
				switch (vdp->state->hscroll_mode)
				{
					default:
						/* Should never happen. */
						assert(0);
						/* Fallthrough */
					case VDP_HSCROLL_MODE_FULL:
						hscroll = vdp->state->vram[vdp->state->hscroll_address + i];
						break;

					case VDP_HSCROLL_MODE_1CELL:
						hscroll = vdp->state->vram[vdp->state->hscroll_address + (scanline >> tile_height_power) * 2 + i];
						break;

					case VDP_HSCROLL_MODE_1LINE:
						hscroll = vdp->state->vram[vdp->state->hscroll_address + (scanline >> vdp->state->double_resolution_enabled) * 2 + i];
						break;
				}

				/* Get the value used to offset the writes to the metapixel buffer */
				hscroll_scroll_offset = hscroll % 16;

				/* Get the value used to offset the reads from the plane map */
				plane_x_offset = 0 - EXTRA_TILES - ((hscroll - hscroll_scroll_offset) / 8);

				/* Obtain the pointer used to write metapixels to the buffer */
				metapixels_pointer = plane_metapixels + hscroll_scroll_offset;

				/* Render tiles */
				for (j = 0; j < (VDP_MAX_SCANLINE_WIDTH + (8 - 1)) / 8 + EXTRA_TILES; ++j)
				{
					unsigned int k;

					unsigned int vscroll;

					unsigned int pixel_y_in_plane;
					unsigned int tile_x;
					unsigned int tile_y;
					unsigned int pixel_y_in_tile;

					unsigned int tile_metadata;
					cc_bool tile_priority;
					unsigned int tile_palette_line;
					cc_bool tile_y_flip;
					cc_bool tile_x_flip;
					unsigned int tile_index;

					/* Get the vertical scroll value */
					switch (vdp->state->vscroll_mode)
					{
						default:
							/* Should never happen. */
							assert(0);
							/* Fallthrough */
						case VDP_VSCROLL_MODE_FULL:
							vscroll = vdp->state->vsram[i];
							break;

						case VDP_VSCROLL_MODE_2CELL:
							vscroll = vdp->state->vsram[(((0 - EXTRA_TILES + j) / 2) * 2 + i) % CC_COUNT_OF(vdp->state->vsram)];
							break;
					}

					/* Get the Y coordinate of the pixel in the plane */
					pixel_y_in_plane = vscroll + scanline;

					/* Get the coordinates of the tile in the plane */
					tile_x = (plane_x_offset + j) & vdp->state->plane_width_bitmask;
					tile_y = (pixel_y_in_plane >> tile_height_power) & vdp->state->plane_height_bitmask;

					/* Obtain and decode tile metadata */
					tile_metadata = vdp->state->vram[(i ? vdp->state->plane_b_address : vdp->state->plane_a_address) + tile_y * vdp->state->plane_width + tile_x];
					tile_priority = (tile_metadata & 0x8000) != 0;
					tile_palette_line = (tile_metadata >> 13) & 3;
					tile_y_flip = (tile_metadata & 0x1000) != 0;
					tile_x_flip = (tile_metadata & 0x0800) != 0;
					tile_index = tile_metadata & 0x7FF;

					/* Get the Y coordinate of the pixel in the tile */
					pixel_y_in_tile = (pixel_y_in_plane & tile_height_mask) ^ (tile_y_flip ? tile_height_mask : 0);

					/* TODO - Unroll this loop? */
					for (k = 0; k < 8; ++k)
					{
						/* Get the X coordinate of the pixel in the tile */
						const unsigned int pixel_x_in_tile = k ^ (tile_x_flip ? 7 : 0);

						/* Get raw tile data that contains the desired metapixel */
						const unsigned int tile_data = vdp->state->vram[tile_index * tile_size + pixel_y_in_tile * 2 + pixel_x_in_tile / 4];

						/* Obtain the index into the palette line */
						const unsigned int palette_line_index = (tile_data >> (4 * ((pixel_x_in_tile & 3) ^ 3))) & 0xF;

						/* Merge the priority, palette line, and palette line index into the complete indexed pixel */
						const unsigned int metapixel = (tile_priority << 6) | (tile_palette_line << 4) | palette_line_index;

						*metapixels_pointer = vdp->constant->blit_lookup[*metapixels_pointer][metapixel];
						++metapixels_pointer;
					}
				}
			}
		}

		/* ************ *
		 * Draw sprites *
		 * ************ */

		/* Update the sprite row cache if needed */
		if (vdp->state->sprite_row_cache.needs_updating)
		{
			/* Caching and preprocessing some of the sprite table allows the renderer to avoid
				scanning the entire sprite table every time it renders a scanline. The VDP actually
				partially caches its sprite data too, though I don't know if it's for the same purpose. */
			const unsigned int max_sprites = vdp->state->h40_enabled ? 80 : 64;

			unsigned int sprite_index;
			unsigned int sprites_remaining = max_sprites;

			vdp->state->sprite_row_cache.needs_updating = cc_false;

			/* Make it so we write to the start of the rows */
			for (i = 0; i < CC_COUNT_OF(vdp->state->sprite_row_cache.rows); ++i)
				vdp->state->sprite_row_cache.rows[i].total = 0;

			sprite_index = 0;

			do
			{
				/* Decode sprite data */
				const unsigned short *cached_sprite = vdp->state->sprite_table_cache[sprite_index];
				const unsigned int y = cached_sprite[0] & 0x3FF;
				const unsigned int width = ((cached_sprite[1] >> 10) & 3) + 1;
				const unsigned int height = ((cached_sprite[1] >> 8) & 3) + 1;
				const unsigned int link = cached_sprite[1] & 0x7F;

				const unsigned int blank_lines = 128 << vdp->state->double_resolution_enabled;

				/* This loop only processes rows that are on-screen, and haven't been drawn yet */
				for (i = CC_MAX(blank_lines + scanline, y); i < CC_MIN(blank_lines + ((vdp->state->v30_enabled ? 30 : 28) << tile_height_power), y + (height << tile_height_power)); ++i)
				{
					struct VDP_SpriteRowCacheRow *row = &vdp->state->sprite_row_cache.rows[i - blank_lines];

					/* Don't write more sprites than are allowed to be drawn on this line */
					if (row->total != (vdp->state->h40_enabled ? 20 : 16))
					{
						struct VDP_SpriteRowCacheEntry *sprite_row_cache_entry = &row->sprites[row->total++];

						sprite_row_cache_entry->table_index = (unsigned char)sprite_index;
						sprite_row_cache_entry->width = (unsigned char)width;
						sprite_row_cache_entry->height = (unsigned char)height;
						sprite_row_cache_entry->y_in_sprite = (unsigned char)(i - y);
					}
				}

				if (link >= max_sprites)
				{
					/* Invalid link - bail before it can cause a crash.
						According to Nemesis, this is actually what real hardware does too:
						http://gendev.spritesmind.net/forum/viewtopic.php?p=8364#p8364 */
					break;
				}

				sprite_index = link;
			}
			while (sprite_index != 0 && --sprites_remaining != 0);
		}

		/* Clear the scanline buffer, so that the sprite blitter
			knows which pixels haven't been drawn yet. */
		memset(sprite_metapixels, 0, sizeof(sprite_metapixels));

		if (!vdp->configuration->sprites_disabled)
		{
			/* Render sprites */
			for (i = 0; i < vdp->state->sprite_row_cache.rows[scanline].total; ++i)
			{
				struct VDP_SpriteRowCacheEntry *sprite_row_cache_entry = &vdp->state->sprite_row_cache.rows[scanline].sprites[i];

				/* Decode sprite data */
				const unsigned short *sprite = &vdp->state->vram[vdp->state->sprite_table_address + sprite_row_cache_entry->table_index * 4];
				const unsigned int width = sprite_row_cache_entry->width;
				const unsigned int height = sprite_row_cache_entry->height;
				const unsigned int tile_metadata = sprite[2];
				const unsigned int x = sprite[3] & 0x1FF;

				/* Decode tile metadata */
				const cc_bool tile_priority = (tile_metadata & 0x8000) != 0;
				const unsigned int tile_palette_line = (tile_metadata >> 13) & 3;
				const cc_bool tile_y_flip = (tile_metadata & 0x1000) != 0;
				const cc_bool tile_x_flip = (tile_metadata & 0x0800) != 0;
				const unsigned int tile_index_base = tile_metadata & 0x7FF;

				unsigned int y_in_sprite = sprite_row_cache_entry->y_in_sprite;

				/* TODO - sprites only mask if another sprite rendered before them on the
				   current scanline, or if the previous scanline reached its pixel limit */
				/* This is a masking sprite: prevent all remaining sprites from being drawn */
				if (x == 0)
					break;

				if (x + width * 8 > 128 && x < 128u + (vdp->state->h40_enabled ? 40 : 32) * 8 - 1)
				{
					unsigned int j;

					unsigned char *metapixels_pointer = sprite_metapixels + (MAX_SPRITE_WIDTH - 1) + x - 128;

					y_in_sprite = tile_y_flip ? (height << tile_height_power) - y_in_sprite - 1 : y_in_sprite;

					for (j = 0; j < width; ++j)
					{
						unsigned int k;

						const unsigned int x_in_sprite = tile_x_flip ? width - j - 1 : j;
						const unsigned int tile_index = tile_index_base + (y_in_sprite >> tile_height_power) + x_in_sprite * height;
						const unsigned int pixel_y_in_tile = y_in_sprite & tile_height_mask;

						for (k = 0; k < 8; ++k)
						{
							/* Get the X coordinate of the pixel in the tile */
							const unsigned int pixel_x_in_tile = k ^ (tile_x_flip ? 7 : 0);

							/* Get raw tile data that contains the desired metapixel */
							const unsigned int tile_data = vdp->state->vram[tile_index * tile_size + pixel_y_in_tile * 2 + pixel_x_in_tile / 4];

							/* Obtain the index into the palette line */
							const unsigned int palette_line_index = (tile_data >> (4 * ((pixel_x_in_tile & 3) ^ 3))) & 0xF;

							/* Merge the priority, palette line, and palette line index into the complete indexed pixel */
							const unsigned int metapixel = (tile_priority << 6) | (tile_palette_line << 4) | palette_line_index;

							*metapixels_pointer |= metapixel & (0 - (unsigned int)(*metapixels_pointer == 0)) & (0 - (unsigned int)(palette_line_index != 0));
							++metapixels_pointer;

							if (--pixel_limit == 0)
								goto DoneWithSprites;
						}
					}
				}
				else
				{
					if (pixel_limit <= width * 8)
						break;

					pixel_limit -= width * 8;
				}

				if (--sprite_limit == 0)
					break;
			}

		DoneWithSprites:;
		}

		/* ************************************ *
		 * Blit sprite pixels onto plane pixels *
		 * ************************************ */

		if (vdp->state->shadow_highlight_enabled)
		{
			for (i = 0; i < VDP_MAX_SCANLINE_WIDTH; ++i)
				plane_metapixels[16 + i] = vdp->constant->blit_lookup_shadow_highlight[plane_metapixels[16 + i]][sprite_metapixels[(MAX_SPRITE_WIDTH - 1) + i]];
		}
		else
		{
			for (i = 0; i < VDP_MAX_SCANLINE_WIDTH; ++i)
				plane_metapixels[16 + i] = vdp->constant->blit_lookup[plane_metapixels[16 + i]][sprite_metapixels[(MAX_SPRITE_WIDTH - 1) + i]] & 0x3F;
		}

		#undef MAX_SPRITE_WIDTH
	}

	/* Send pixels to the frontend to be displayed */
	scanline_rendered_callback(scanline_rendered_callback_user_data, scanline, plane_metapixels + 16, (vdp->state->h40_enabled ? 40 : 32) * 8, (vdp->state->v30_enabled ? 30 : 28) << tile_height_power);
}

unsigned int VDP_ReadData(const VDP *vdp)
{
	unsigned int value = 0;

	if (!vdp->state->access.read_mode)
	{
		/* According to GENESIS SOFTWARE DEVELOPMENT MANUAL (COMPLEMENT) section 4.1,
		   this should cause the 68k to hang */
		/* TODO */
		PrintError("Data was read from the VDP data port while the VDP was in write mode");
	}
	else
	{
		value = ReadAndIncrement(vdp->state);
	}

	return value;
}

unsigned int VDP_ReadControl(const VDP *vdp)
{
	/* TODO */

	/* Reading from the control port aborts the VDP waiting for a second command word to be written.
	   This doesn't appear to be documented in the official SDK manuals we have, but the official
	   boot code makes use of this feature. */
	vdp->state->access.write_pending = cc_false;

	/* Output the 'V-blanking' and 'H-blanking' bits */
	return (vdp->state->currently_in_vblank << 3) | (1 << 2); /* The H-blank bit is forced for now so Sonic 2's two-player mode works */
}

void VDP_WriteData(const VDP *vdp, unsigned int value, void (*colour_updated_callback)(const void *user_data, unsigned int index, unsigned int colour), const void *colour_updated_callback_user_data)
{
	if (vdp->state->access.read_mode)
	{
		/* Invalid input, but defined behaviour */
		PrintError("Data was written to the VDP data port while the VDP was in read mode");

		/* According to GENESIS SOFTWARE DEVELOPMENT MANUAL (COMPLEMENT) section 4.1,
		   data should not be written, but the address should be incremented */
		vdp->state->access.index += vdp->state->access.increment;
	}
	else if (vdp->state->dma.pending)
	{
		/* Perform DMA fill */
		vdp->state->dma.pending = cc_false;

		do
		{
			WriteAndIncrement(vdp->state, value, colour_updated_callback, colour_updated_callback_user_data);
		}
		while (--vdp->state->dma.length, vdp->state->dma.length &= 0xFFFF, vdp->state->dma.length != 0);
	}
	else
	{
		/* Write the value to memory */
		WriteAndIncrement(vdp->state, value, colour_updated_callback, colour_updated_callback_user_data);
	}
}

/* TODO - Retention of partial commands */
void VDP_WriteControl(const VDP *vdp, unsigned int value, void (*colour_updated_callback)(const void *user_data, unsigned int index, unsigned int colour), const void *colour_updated_callback_user_data, unsigned int (*read_callback)(const void *user_data, unsigned long address), const void *read_callback_user_data)
{
	if (vdp->state->access.write_pending)
	{
		/* This command is setting up for a memory access (part 2) */
		const unsigned int destination_address = (vdp->state->access.cached_write & 0x3FFF) | ((value & 3) << 14);
		const unsigned int access_mode = ((vdp->state->access.cached_write >> 14) & 3) | ((value >> 2) & 0x3C);

		vdp->state->access.write_pending = cc_false;

		vdp->state->access.index = (unsigned short)destination_address;
		vdp->state->access.read_mode = (access_mode & 1) == 0;

		if (vdp->state->dma.enabled)
			vdp->state->dma.pending = (access_mode & 0x20) != 0;

		switch ((access_mode >> 1) & 7)
		{
			case 0: /* VRAM */
				vdp->state->access.selected_buffer = VDP_ACCESS_VRAM;
				break;

			case 4: /* CRAM (read) */
			case 1: /* CRAM (write) */
				vdp->state->access.selected_buffer = VDP_ACCESS_CRAM;
				break;

			case 2: /* VSRAM */
				vdp->state->access.selected_buffer = VDP_ACCESS_VSRAM;
				break;

			default: /* Invalid */
				PrintError("Invalid VDP access mode specified (0x%X)", access_mode);
				break;
		}

		if (vdp->state->dma.pending && vdp->state->dma.mode != VDP_DMA_MODE_FILL)
		{
			/* Firing DMA */
			vdp->state->dma.pending = cc_false;

			if (vdp->state->dma.mode == VDP_DMA_MODE_MEMORY_TO_VRAM)
			{
				do
				{
					WriteAndIncrement(vdp->state, read_callback(read_callback_user_data, ((unsigned long)vdp->state->dma.source_address_high << 17) | ((unsigned long)vdp->state->dma.source_address_low << 1)), colour_updated_callback, colour_updated_callback_user_data);

					/* Emulate the 128KiB DMA wrap-around bug */
					++vdp->state->dma.source_address_low;
					vdp->state->dma.source_address_low &= 0xFFFF;
				}
				while (--vdp->state->dma.length, vdp->state->dma.length &= 0xFFFF, vdp->state->dma.length != 0);
			}
			else /*if (state->dma.mode == VDP_DMA_MODE_COPY)*/
			{
				/* TODO */
				PrintError("DMA copy attempted, but not currently supported!");
			}
		}
	}
	else if ((value & 0xC000) != 0x8000)
	{
		/* This command is setting up for a memory access (part 1) */
		vdp->state->access.write_pending = cc_true;
		vdp->state->access.cached_write = (unsigned short)value;
	}
	else
	{
		const unsigned int reg = (value >> 8) & 0x3F;
		const unsigned int data = value & 0xFF;

		/* This command is setting a register */
		switch (reg)
		{
			case 0:
				/* MODE SET REGISTER NO.1 */
				vdp->state->h_int_enabled = (data & (1 << 4)) != 0;
				/* TODO */
				break;

			case 1:
				/* MODE SET REGISTER NO.2 */
				vdp->state->display_enabled = (data & (1 << 6)) != 0;
				vdp->state->v_int_enabled = (data & (1 << 5)) != 0;
				vdp->state->dma.enabled = (data & (1 << 4)) != 0;
				vdp->state->v30_enabled = (data & (1 << 3)) != 0;
				/* TODO */
				break;

			case 2:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL A */
				vdp->state->plane_a_address = (data & 0x38) << (10 - 1);
				break;

			case 3:
				/* PATTERN NAME TABLE BASE ADDRESS FOR WINDOW */
				vdp->state->window_address = (data & 0x3E) << (10 - 1);
				break;

			case 4:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL B */
				vdp->state->plane_b_address = (data & 7) << (13 - 1);
				break;

			case 5:
				/* SPRITE ATTRIBUTE TABLE BASE ADDRESS */
				vdp->state->sprite_table_address = (data & 0x7F) << (9 - 1);

				/* Real VDPs partially cache the sprite table, and forget to update it
				   when the sprite table base address is changed. Replicating this
				   behaviour may be needed in order to properly emulate certain effects
				   that involve manipulating the sprite table during rendering. */
				/*state->sprite_cache.needs_updating = cc_true;*/ /* Oops */

				break;

			case 7:
				/* BACKGROUND COLOR */
				vdp->state->background_colour = data & 0x3F;
				break;

			case 10:
				/* H INTERRUPT REGISTER */
				vdp->state->h_int_interval = (unsigned char)data;
				break;

			case 11:
				/* MODE SET REGISTER NO.3 */

				/* TODO - External interrupt */

				vdp->state->vscroll_mode = data & 4 ? VDP_VSCROLL_MODE_2CELL : VDP_VSCROLL_MODE_FULL;

				switch (data & 3)
				{
					case 0:
						vdp->state->hscroll_mode = VDP_HSCROLL_MODE_FULL;
						break;

					case 1:
						PrintError("Prohibitied H-scroll mode selected");
						break;

					case 2:
						vdp->state->hscroll_mode = VDP_HSCROLL_MODE_1CELL;
						break;

					case 3:
						vdp->state->hscroll_mode = VDP_HSCROLL_MODE_1LINE;
						break;
				}

				break;

			case 12:
				/* MODE SET REGISTER NO.4 */
				/* TODO */
				vdp->state->h40_enabled = (data & ((1 << 7) | (1 << 0))) != 0;
				vdp->state->shadow_highlight_enabled = (data & (1 << 3)) != 0;

				/* Process the interlacing bits.
				   To fully understand this, one has to understand how PAL and NTSC televisions display:
				   NTSC televisions display 480 lines, however, they are divided into two 'fields' of
				   240 lines. On one frame the cathode ray draws the even-numbered lines, then on the
				   next frame it draw the odd-numbered lines. */
				switch ((data >> 1) & 3)
				{
					case 0:
						/* Regular '240p' mode: the 'Genesis Software Manual' seems to suggest that this
						   mode only outputs the even-numbered lines, leaving the odd-numbered ones black. */
						vdp->state->double_resolution_enabled = cc_false;
						break;

					case 1:
						/* This mode renders the odd-numbered lines as well, but they are merely
						   duplicates of the even lines. The 'Genesis Software Manual' warns that this
						   can create severe vertical blurring. */
						vdp->state->double_resolution_enabled = cc_false;
						break;

					case 2:
						/* The 'Genesis Software Manual' warns that this is prohibited. Some unlicensed
						   EA games use this. Apparently it creates an image that is slightly broken in
						   some way. */
						vdp->state->double_resolution_enabled = cc_false;
						break;

					case 3:
						/* Double resolution mode: in this mode, the odd and even lines display different
						   graphics, effectively turning the tiles from 8x8 to 8x16. */
						vdp->state->double_resolution_enabled = cc_true;
						break;
				}

				break;

			case 13:
				/* H SCROLL DATA TABLE BASE ADDRESS */
				vdp->state->hscroll_address = (data & 0x3F) << (10 - 1);
				break;

			case 15:
				/* AUTO INCREMENT DATA */
				vdp->state->access.increment = (unsigned short)data;
				break;

			case 16:
			{
				/* SCROLL SIZE */
				const unsigned int width_index  = (data >> 0) & 3;
				const unsigned int height_index = (data >> 4) & 3;

				if ((width_index == 3 && height_index != 0) || (height_index == 3 && width_index != 0))
				{
					PrintError("Selected plane size exceeds 64x64/32x128/128x32");
				}
				else
				{
					switch (width_index)
					{
						case 0:
							vdp->state->plane_width = 32;
							break;

						case 1:
							vdp->state->plane_width = 64;
							break;

						case 2:
							PrintError("Prohibited plane width selected");
							break;

						case 3:
							vdp->state->plane_width = 128;
							break;
					}

					switch (height_index)
					{
						case 0:
							vdp->state->plane_height = 32;
							break;

						case 1:
							vdp->state->plane_height = 64;
							break;

						case 2:
							PrintError("Prohibited plane height selected");
							break;

						case 3:
							vdp->state->plane_height = 128;
							break;
					}

					vdp->state->plane_width_bitmask = vdp->state->plane_width - 1;
					vdp->state->plane_height_bitmask = vdp->state->plane_height - 1;
				}

				break;
			}

			case 17:
				/* WINDOW H POSITION */
				/* TODO */
				break;

			case 18:
				/* WINDOW V POSITION */
				/* TODO */
				break;

			case 19:
				/* DMA LENGTH COUNTER LOW */
				vdp->state->dma.length &= ~(0xFFu << 0);
				vdp->state->dma.length |= data << 0;
				break;

			case 20:
				/* DMA LENGTH COUNTER HIGH */
				vdp->state->dma.length &= ~(0xFFu << 8);
				vdp->state->dma.length |= data << 8;
				break;

			case 21:
				/* DMA SOURCE ADDRESS LOW */
				vdp->state->dma.source_address_low &= ~(0xFFu << 0);
				vdp->state->dma.source_address_low |= data << 0;
				break;

			case 22:
				/* DMA SOURCE ADDRESS MID. */
				vdp->state->dma.source_address_low &= ~(0xFFu << 8);
				vdp->state->dma.source_address_low |= data << 8;
				break;

			case 23:
				/* DMA SOURCE ADDRESS HIGH */
				if (data & 0x80)
				{
					vdp->state->dma.source_address_high = data & 0x3F;
					vdp->state->dma.mode = data & 0x40 ? VDP_DMA_MODE_COPY : VDP_DMA_MODE_FILL;
				}
				else
				{
					vdp->state->dma.source_address_high = data & 0x7F;
					vdp->state->dma.mode = VDP_DMA_MODE_MEMORY_TO_VRAM;
				}

				break;

			case 6:
			case 8:
			case 9:
			case 14:
				/* Unused legacy register */
				break;

			default:
				/* Invalid */
				PrintError("Attempted to set invalid VDP register (0x%X)", reg);
				break;
		}
	}
}
