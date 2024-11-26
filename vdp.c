/* TODO: https://gendev.spritesmind.net/forum/viewtopic.php?p=21016#p21016 */
/* TODO: HV counter details - https://wiki.megadrive.org/index.php?title=VDP_Ports */

#include "vdp.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon/clowncommon.h"

#include "log.h"

enum
{
	SHADOW_HIGHLIGHT_NORMAL = 0 << 6,
	SHADOW_HIGHLIGHT_SHADOW = 1 << 6,
	SHADOW_HIGHLIGHT_HIGHLIGHT = 2 << 6
};

/* Some of the logic here is based on research done by Nemesis:
   https://gendev.spritesmind.net/forum/viewtopic.php?p=21016#p21016 */

static cc_bool IsDMAPending(const VDP_State* const state)
{
	return (state->access.code_register & 0x20) != 0;
}

static void ClearDMAPending(VDP_State* const state)
{
	state->access.code_register &= ~0x20;
}

static cc_bool IsInReadMode(const VDP_State* const state)
{
	return (state->access.code_register & 1) == 0;
}

static void WriteVRAM(VDP_State* const state, const cc_u16f index, const cc_u8f value)
{
	/* Update sprite cache if we're writing to the sprite table */
	/* TODO: Do DMA fills and copies do this? */
	const cc_u16f index_wrapped = index % CC_COUNT_OF(state->vram);
	const cc_u16f sprite_table_index = index_wrapped - state->sprite_table_address;

	if (sprite_table_index < (state->h40_enabled ? 80u : 64u) * 8u && (sprite_table_index & 4) == 0)
	{
		cc_u8l* const cache_bytes = state->sprite_table_cache[sprite_table_index / 8];

		cache_bytes[sprite_table_index & 3] = value;

		state->sprite_row_cache.needs_updating = cc_true;
	}

	state->vram[index_wrapped] = value;
}

static void WriteAndIncrement(VDP_State* const state, const cc_u16f value, const VDP_ColourUpdatedCallback colour_updated_callback, const void* const colour_updated_callback_user_data)
{
	switch (state->access.selected_buffer)
	{
		case VDP_ACCESS_VRAM:
			WriteVRAM(state, state->access.address_register ^ 0, (cc_u8f)(value >> 8));
			WriteVRAM(state, state->access.address_register ^ 1, (cc_u8f)(value & 0xFF));
			break;

		case VDP_ACCESS_CRAM:
		{
			/* Remove garbage bits */
			const cc_u16f colour = value & 0xEEE;

			/* Fit index to within CRAM */
			const cc_u16f index_wrapped = (state->access.address_register / 2) % CC_COUNT_OF(state->cram);

			/* Store regular Mega Drive-format colour (with garbage bits intact) */
			state->cram[index_wrapped] = colour;

			/* Now let's precompute the shadow/normal/highlight colours in
			   RGB444 (so we don't have to calculate them during blitting)
			   and send them to the frontend for further optimisation */

			/* Create normal colour */
			/* (repeat the upper bit in the lower bit so that the full 4-bit colour range is covered) */
			colour_updated_callback((void*)colour_updated_callback_user_data, SHADOW_HIGHLIGHT_NORMAL + index_wrapped, colour | ((colour & 0x888) >> 3));

			/* Create shadow colour */
			/* (divide by two and leave in lower half of colour range) */
			colour_updated_callback((void*)colour_updated_callback_user_data, SHADOW_HIGHLIGHT_SHADOW + index_wrapped, colour >> 1);

			/* Create highlight colour */
			/* (divide by two and move to upper half of colour range) */
			colour_updated_callback((void*)colour_updated_callback_user_data, SHADOW_HIGHLIGHT_HIGHLIGHT + index_wrapped, 0x888 + (colour >> 1));

			break;
		}

		case VDP_ACCESS_VSRAM:
			state->vsram[(state->access.address_register / 2) % CC_COUNT_OF(state->vsram)] = (cc_u16l)(value & 0x7FF);
			break;

		default:
			/* Should never happen. */
			assert(0);
			/* Fallthrough */
		case VDP_ACCESS_INVALID:
			LogMessage("VDP write attempted with invalid access mode specified (0x%" CC_PRIXFAST16 ")", state->access.code_register);
			break;
	}

	state->access.address_register += state->access.increment;
}

static cc_u16f ReadAndIncrement(VDP_State* const state)
{
	cc_u16f value;

	switch (state->access.selected_buffer)
	{
		case VDP_ACCESS_VRAM:
			value = VDP_ReadVRAMWord(state, state->access.address_register % CC_COUNT_OF(state->vram));
			break;

		case VDP_ACCESS_CRAM:
			value = state->cram[(state->access.address_register / 2) % CC_COUNT_OF(state->cram)];
			break;

		case VDP_ACCESS_VSRAM:
			value = state->vsram[(state->access.address_register / 2) % CC_COUNT_OF(state->vsram)];
			break;

		default:
			/* Should never happen. */
			assert(0);
			/* Fallthrough */
		case VDP_ACCESS_INVALID:
			value = 0;
			LogMessage("VDP read attempted with invalid access mode specified (0x%" CC_PRIXFAST16 ")", state->access.code_register);
			break;
	}

	state->access.address_register += state->access.increment;

	return value;
}

void VDP_Constant_Initialise(VDP_Constant* const constant)
{
	/* This essentially pre-computes the VDP's depth-test and alpha-test,
	   generating a lookup table to eliminate the need to perform these
	   every time a pixel is blitted. This provides a *massive* speed boost. */
	cc_u16f new_pixel_high;

	for (new_pixel_high = 0; new_pixel_high < CC_COUNT_OF(constant->blit_lookup); ++new_pixel_high)
	{
		cc_u16f old_pixel;

		for (old_pixel = 0; old_pixel < CC_COUNT_OF(constant->blit_lookup[0]); ++old_pixel)
		{
			cc_u16f new_pixel_low;

			for (new_pixel_low = 0; new_pixel_low < CC_COUNT_OF(constant->blit_lookup[0][0]); ++new_pixel_low)
			{
				const cc_u16f palette_line_index_mask = 0xF;
				const cc_u16f colour_index_mask = 0x3F;
				const cc_u16f priority_mask = 0x40;
				const cc_u16f not_shadowed_mask = 0x80;

				const cc_u16f old_palette_line_index = old_pixel & palette_line_index_mask;
				const cc_u16f old_colour_index = old_pixel & colour_index_mask;
				const cc_bool old_priority = (old_pixel & priority_mask) != 0;
				const cc_bool old_not_shadowed = (old_pixel & not_shadowed_mask) != 0;

				const cc_u16f new_pixel = (new_pixel_high << 4) | new_pixel_low;

				const cc_u16f new_palette_line_index = new_pixel & palette_line_index_mask;
				const cc_u16f new_colour_index = new_pixel & colour_index_mask;
				const cc_bool new_priority = (new_pixel & priority_mask) != 0;
				const cc_bool new_not_shadowed = new_priority;

				const cc_bool draw_new_pixel = new_palette_line_index != 0 && (old_palette_line_index == 0 || !old_priority || new_priority);

				cc_u16f output;

				/* First, generate the table for regular blitting */
				output = draw_new_pixel ? new_pixel : old_pixel;

				output |= old_not_shadowed || new_not_shadowed ? not_shadowed_mask : 0;

				constant->blit_lookup[new_pixel_high][old_pixel][new_pixel_low] = (cc_u8l)output;

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

				constant->blit_lookup_shadow_highlight[new_pixel_high][old_pixel][new_pixel_low] = (cc_u8l)output;
			}
		}
	}
}

void VDP_State_Initialise(VDP_State* const state)
{
	state->access.write_pending = cc_false;
	state->access.address_register = 0;
	state->access.code_register = 0;
	state->access.selected_buffer = VDP_ACCESS_VRAM;
	state->access.increment = 0;

	state->dma.enabled = cc_false;
	state->dma.mode = VDP_DMA_MODE_MEMORY_TO_VRAM;
	state->dma.source_address_high = 0;
	state->dma.source_address_low = 0;
	state->dma.length = 0;

	state->plane_a_address = 0;
	state->plane_b_address = 0;
	state->window_address = 0;
	state->sprite_table_address = 0;
	state->hscroll_address = 0;

	state->window.aligned_right = cc_false;
	state->window.aligned_bottom = cc_false;
	state->window.horizontal_boundary = 0;
	state->window.vertical_boundary = 0;

	state->plane_width = 32;
	state->plane_height = 32;

	state->display_enabled = cc_false;
	state->v_int_enabled = cc_false;
	state->h_int_enabled = cc_false;
	state->h40_enabled = cc_false;
	state->v30_enabled = cc_false;
	state->shadow_highlight_enabled = cc_false;
	state->double_resolution_enabled = cc_false;

	state->background_colour = 0;
	state->h_int_interval = 0;
	state->currently_in_vblank = cc_false;
	state->allow_sprite_masking = cc_false;

	state->hscroll_mode = VDP_HSCROLL_MODE_FULL;
	state->vscroll_mode = VDP_VSCROLL_MODE_FULL;

	memset(state->vram, 0, sizeof(state->vram));
	memset(state->cram, 0, sizeof(state->cram));
	memset(state->vsram, 0, sizeof(state->vsram));
	memset(state->sprite_table_cache, 0, sizeof(state->sprite_table_cache));

	state->sprite_row_cache.needs_updating = cc_true;
	memset(state->sprite_row_cache.rows, 0, sizeof(state->sprite_row_cache.rows));

	state->kdebug_buffer_index = 0;
	/* This byte never gets overwritten, so we can set it ahead of time. */
	state->kdebug_buffer[CC_COUNT_OF(state->kdebug_buffer) - 1] = '\0';
}

static void RenderTile(const VDP* const vdp, const cc_u16f pixel_y_in_plane, const cc_u16f tile_x, const cc_u16f tile_y, const cc_u16f plane_address, const cc_u16f tile_height_mask, const cc_u16f tile_size, cc_u8l** const metapixels_pointer)
{
	const VDP_TileMetadata tile = VDP_DecomposeTileMetadata(VDP_ReadVRAMWord(vdp->state, plane_address + (tile_y * vdp->state->plane_width + tile_x) * 2));

	/* Get the Y coordinate of the pixel in the tile */
	const cc_u16f pixel_y_in_tile = (pixel_y_in_plane & tile_height_mask) ^ (tile.y_flip ? tile_height_mask : 0);

	/* Get raw tile data that contains the desired metapixel */
	const cc_u8l* const tile_data = &vdp->state->vram[(tile.tile_index * tile_size + pixel_y_in_tile * 4) % CC_COUNT_OF(vdp->state->vram)];

	const cc_u8f byte_index_xor = tile.x_flip ? 7 : 0;
	const cc_u8f metapixel_upper_bits = (tile.priority << 2) | tile.palette_line;

	const cc_u8l (* const blit_lookup)[1 << 4] = vdp->constant->blit_lookup[metapixel_upper_bits];

	cc_u16f i;

	/* TODO - Unroll this loop? */
	for (i = 0; i < 8; ++i)
	{
		/* Get the X coordinate of the pixel in the tile */
		const cc_u8f pixel_x_in_tile = i ^ byte_index_xor;

		/* Obtain the index into the palette line */
		const cc_u8f nibble_shift = (~pixel_x_in_tile & 1) << 2;
		const cc_u8f palette_line_index = (tile_data[pixel_x_in_tile / 2] >> nibble_shift) & 0xF;

		**metapixels_pointer = blit_lookup[**metapixels_pointer][palette_line_index];
		++*metapixels_pointer;
	}
}

void VDP_RenderScanline(const VDP* const vdp, const cc_u16f scanline, const VDP_ScanlineRenderedCallback scanline_rendered_callback, const void* const scanline_rendered_callback_user_data)
{
	const VDP_Constant* const constant = vdp->constant;
	VDP_State* const state = vdp->state;
	const cc_u16f tile_height_power = state->double_resolution_enabled ? 4 : 3;

	cc_u16f i;

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
	cc_u8l plane_metapixels[16 + CC_DIVIDE_CEILING(VDP_MAX_SCANLINE_WIDTH, 8) * 8 + (16 - 1)];

	assert(scanline < VDP_MAX_SCANLINES);

	/* Fill the scanline buffer with the background colour */
	for (i = 0; i < CC_COUNT_OF(plane_metapixels); ++i)
		plane_metapixels[i] = state->background_colour;

	if (state->display_enabled)
	{
		/* ***** *
		 * Setup *
		 * ***** */

		#define MAX_SPRITE_WIDTH (8 * 4)

		const cc_u16f tile_height_mask = (1 << tile_height_power) - 1;
		const cc_u16f tile_size = (8 << tile_height_power) / 2;

		/* Back these two up before we overwrite them. */
		const cc_u16l plane_width_copy = state->plane_width;
		const cc_u16l plane_height_copy = state->plane_height;

		const cc_u16l window_plane_width = state->h40_enabled ? 64 : 32;
		const cc_u16l window_plane_height = 32;

		cc_u16f sprite_limit = state->h40_enabled ? 20 : 16;
		cc_u16f pixel_limit = state->h40_enabled ? 320 : 256;

		/* The padding bytes of the left and right are for allowing sprites to overdraw at the
		   edges of the screen. */
		cc_u8l sprite_metapixels[(MAX_SPRITE_WIDTH - 1) + VDP_MAX_SCANLINE_WIDTH + (MAX_SPRITE_WIDTH - 1)][2];


		/* *********** */
		/* Draw planes */
		/* *********** */
		for (i = 2; i-- > 0; )
		{
			/* Notably, we allow Plane A to render in the Window Plane's place when the latter is disabled. */
			/* TODO: Test if this works properly in Interlace Mode 2. */
			const cc_bool rendering_window_plane = i == 0 && (scanline < state->window.vertical_boundary) != state->window.aligned_bottom && !vdp->configuration->window_disabled;

			/* The window plane has a hardcoded size, unlike the other planes. */
			/* Sonic 3's 'Data Select' menu background relies on this. */
			state->plane_width = rendering_window_plane ? window_plane_width : plane_width_copy;
			state->plane_height = rendering_window_plane ? window_plane_height : plane_height_copy;

			if (rendering_window_plane || !vdp->configuration->planes_disabled[i])
			{
				cc_u16f hscroll;

				/* Get the horizontal scroll value */
				if (rendering_window_plane)
				{
					hscroll = 0;
				}
				else
				{
					switch (state->hscroll_mode)
					{
						default:
							/* Should never happen. */
							assert(0);
							/* Fallthrough */
						case VDP_HSCROLL_MODE_FULL:
							hscroll = VDP_ReadVRAMWord(state, state->hscroll_address + i * 2);
							break;

						case VDP_HSCROLL_MODE_1CELL:
							hscroll = VDP_ReadVRAMWord(state, state->hscroll_address + (scanline >> tile_height_power << tile_height_power) * 4 + i * 2);
							break;

						case VDP_HSCROLL_MODE_1LINE:
							hscroll = VDP_ReadVRAMWord(state, state->hscroll_address + (scanline >> state->double_resolution_enabled) * 4 + i * 2);
							break;
					}
				}

				{
					const cc_u16l plane_width_bitmask = state->plane_width - 1;
					const cc_u16l plane_height_bitmask = state->plane_height - 1;

					const cc_u16f plane_address = i == 0 ? (rendering_window_plane ? state->window_address : state->plane_a_address) : state->plane_b_address;

					/* The extra two tiles on the left of the scanline */
					const cc_u16f EXTRA_TILES = 2;

					/* Get the value used to offset the writes to the metapixel buffer */
					const cc_u16f hscroll_scroll_offset = hscroll % 16;

					/* Get the value used to offset the reads from the plane map */
					const cc_u16f plane_x_offset = 0 - EXTRA_TILES - ((hscroll - hscroll_scroll_offset) / 8);

					/* Obtain the pointer used to write metapixels to the buffer */
					cc_u8l *metapixels_pointer = plane_metapixels + hscroll_scroll_offset;

					cc_u16f j;

					/* Render tiles */
					for (j = 0; j < CC_DIVIDE_CEILING(VDP_MAX_SCANLINE_WIDTH, 8) + EXTRA_TILES; ++j)
					{
						cc_u16f vscroll;

						/* Get the vertical scroll value */
						if (rendering_window_plane)
						{
							vscroll = 0;
						}
						else
						{
							switch (state->vscroll_mode)
							{
								default:
									/* Should never happen. */
									assert(0);
									/* Fallthrough */
								case VDP_VSCROLL_MODE_FULL:
									vscroll = state->vsram[i];
									break;

								case VDP_VSCROLL_MODE_2CELL:
									vscroll = state->vsram[(((0 - EXTRA_TILES + j) / 2) * 2 + i) % CC_COUNT_OF(state->vsram)];
									break;
							}
						}

						{
							/* Get the Y coordinate of the pixel in the plane */
							const cc_u16f pixel_y_in_plane = vscroll + scanline;

							/* Get the coordinates of the tile in the plane */
							const cc_u16f tile_x = (plane_x_offset + j) & plane_width_bitmask;
							const cc_u16f tile_y = (pixel_y_in_plane >> tile_height_power) & plane_height_bitmask;

							RenderTile(vdp, pixel_y_in_plane, tile_x, tile_y, plane_address, tile_height_mask, tile_size, &metapixels_pointer);
						}
					}
				}
			}
		}

		/* ************************************ *
		 * Draw horizontal part of window plane *
		 * ************************************ */
		if (!vdp->configuration->window_disabled)
		{
			/* TODO: Emulate the bug where the tiles to the right of the window plane distort
			   if Plane A is scrolled horizontally by an amount that is not a multiple of 16. */
			const cc_u16f start = state->window.aligned_right ? state->window.horizontal_boundary : 0;
			const cc_u16f end = state->window.aligned_right ? CC_DIVIDE_CEILING(VDP_MAX_SCANLINE_WIDTH, 8) : state->window.horizontal_boundary;

			/* Obtain the pointer used to write metapixels to the buffer */
			cc_u8l *metapixels_pointer = &plane_metapixels[16 + start * 8];

			/* The window plane has a hardcoded size, unlike the other planes. */
			state->plane_width = window_plane_width;
			state->plane_height = window_plane_height;

			/* Render tiles */
			for (i = start; i < end; ++i)
				RenderTile(vdp, scanline, i, scanline >> tile_height_power, state->window_address, tile_height_mask, tile_size, &metapixels_pointer);
		}

		/* Restore the plane size, since we meddled with it earlier for the window plane. */
		state->plane_width = plane_width_copy;
		state->plane_height = plane_height_copy;

		/* ************ *
		 * Draw sprites *
		 * ************ */

		/* Update the sprite row cache if needed */
		if (state->sprite_row_cache.needs_updating)
		{
			/* Caching and preprocessing some of the sprite table allows the renderer to avoid
			   scanning the entire sprite table every time it renders a scanline. The VDP actually
			   partially caches its sprite data too, though I don't know if it's for the same purpose. */
			const cc_u16f max_sprites = state->h40_enabled ? 80 : 64;

			cc_u16f sprite_index;
			cc_u16f sprites_remaining = max_sprites;

			state->sprite_row_cache.needs_updating = cc_false;

			/* Make it so we write to the start of the rows */
			for (i = 0; i < CC_COUNT_OF(state->sprite_row_cache.rows); ++i)
				state->sprite_row_cache.rows[i].total = 0;

			sprite_index = 0;

			do
			{
				const VDP_CachedSprite cached_sprite = VDP_GetCachedSprite(state, sprite_index);
				const cc_u16f blank_lines = 128 << state->double_resolution_enabled;

				/* This loop only processes rows that are on-screen. */
				for (i = CC_MAX(blank_lines, cached_sprite.y); i < CC_MIN(blank_lines + ((state->v30_enabled ? 30 : 28) << tile_height_power), cached_sprite.y + (cached_sprite.height << tile_height_power)); ++i)
				{
					struct VDP_SpriteRowCacheRow* const row = &state->sprite_row_cache.rows[i - blank_lines];

					/* Don't write more sprites than are allowed to be drawn on this line */
					if (row->total != (state->h40_enabled ? 20 : 16))
					{
						struct VDP_SpriteRowCacheEntry* const sprite_row_cache_entry = &row->sprites[row->total++];

						sprite_row_cache_entry->table_index = (cc_u8l)sprite_index;
						sprite_row_cache_entry->width = (cc_u8l)cached_sprite.width;
						sprite_row_cache_entry->height = (cc_u8l)cached_sprite.height;
						sprite_row_cache_entry->y_in_sprite = (cc_u8l)(i - cached_sprite.y);
					}
				}

				if (cached_sprite.link >= max_sprites)
				{
					/* Invalid link - bail before it can cause a crash.
						According to Nemesis, this is actually what real hardware does too:
						http://gendev.spritesmind.net/forum/viewtopic.php?p=8364#p8364 */
					break;
				}

				sprite_index = cached_sprite.link;
			}
			while (sprite_index != 0 && --sprites_remaining != 0);
		}

		/* Clear the scanline buffer, so that the sprite blitter
			knows which pixels haven't been drawn yet. */
		memset(sprite_metapixels, 0, sizeof(sprite_metapixels));

		if (!vdp->configuration->sprites_disabled)
		{
			cc_bool masked = cc_false;

			/* Render sprites */
			/* This has been verified with Nemesis's sprite masking and overflow test ROM:
			   https://segaretro.org/Sprite_Masking_and_Overflow_Test_ROM */
			for (i = 0; i < state->sprite_row_cache.rows[scanline].total; ++i)
			{
				struct VDP_SpriteRowCacheEntry* const sprite_row_cache_entry = &state->sprite_row_cache.rows[scanline].sprites[i];

				/* Decode sprite data */
				const cc_u16f sprite_index = state->sprite_table_address + sprite_row_cache_entry->table_index * 8;
				const cc_u16f width = sprite_row_cache_entry->width;
				const cc_u16f height = sprite_row_cache_entry->height;
				const VDP_TileMetadata tile = VDP_DecomposeTileMetadata(VDP_ReadVRAMWord(state, sprite_index + 4));
				const cc_u16f x = VDP_ReadVRAMWord(state, sprite_index + 6) & 0x1FF;

				const cc_u8f metapixel_high_bits = (tile.priority << 2) | tile.palette_line;

				cc_u16f y_in_sprite = sprite_row_cache_entry->y_in_sprite;

				/* This is a masking sprite: prevent all remaining sprites from being drawn */
				if (x == 0)
					masked = state->allow_sprite_masking;
				else
					/* Enable sprite masking after successfully drawing a sprite. */
					state->allow_sprite_masking = cc_true;

				/* Skip rendering when possible or required. */
				if (masked || x + width * 8 <= 0x80u || x >= 0x80u + (state->h40_enabled ? 40 : 32) * 8)
				{
					if (pixel_limit <= width * 8)
						goto PixelLimitReached;

					pixel_limit -= width * 8;
				}
				else
				{
					cc_u8l *metapixels_pointer = sprite_metapixels[(MAX_SPRITE_WIDTH - 1) + x - 0x80];

					cc_u16f j;

					y_in_sprite = tile.y_flip ? (height << tile_height_power) - y_in_sprite - 1 : y_in_sprite;

					for (j = 0; j < width; ++j)
					{
						const cc_u16f x_in_sprite = tile.x_flip ? width - j - 1 : j;
						const cc_u16f tile_index = tile.tile_index + (y_in_sprite >> tile_height_power) + x_in_sprite * height;
						const cc_u16f pixel_y_in_tile = y_in_sprite & tile_height_mask;

						/* Get raw tile data that contains the desired metapixel */
						const cc_u8l* const tile_data = &state->vram[(tile_index * tile_size + pixel_y_in_tile * 4) % CC_COUNT_OF(state->vram)];

						cc_u16f k;

						for (k = 0; k < 8; ++k)
						{
							/* Get the X coordinate of the pixel in the tile */
							const cc_u8f pixel_x_in_tile = k ^ (tile.x_flip ? 7 : 0);

							/* Obtain the index into the palette line */
							const cc_u8f nibble_shift = (~pixel_x_in_tile & 1) << 2;
							const cc_u8f palette_line_index = (tile_data[pixel_x_in_tile / 2] >> nibble_shift) & 0xF;

							const cc_u8f mask = 0 - (cc_u8f)((metapixels_pointer[1] == 0) & (palette_line_index != 0));

							*metapixels_pointer++ |= metapixel_high_bits & mask;
							*metapixels_pointer++ |= palette_line_index & mask;

							if (--pixel_limit == 0)
								goto PixelLimitReached;
						}
					}
				}

				if (--sprite_limit == 0)
					break;
			}

			/* Prevent sprite masking when ending a scanline without reaching the pixel limit. */
			state->allow_sprite_masking = cc_false;

		PixelLimitReached:;
		}

		/* ************************************ *
		 * Blit sprite pixels onto plane pixels *
		 * ************************************ */

		{
			const cc_u8l *sprite_metapixels_pointer = sprite_metapixels[MAX_SPRITE_WIDTH - 1];
			cc_u8l *plane_metapixels_pointer = &plane_metapixels[16];

			if (state->shadow_highlight_enabled)
			{
				for (i = 0; i < VDP_MAX_SCANLINE_WIDTH; ++i)
				{
					*plane_metapixels_pointer = constant->blit_lookup_shadow_highlight[sprite_metapixels_pointer[0]][*plane_metapixels_pointer][sprite_metapixels_pointer[1]];
					++plane_metapixels_pointer;
					sprite_metapixels_pointer += 2;
				}
			}
			else
			{
				for (i = 0; i < VDP_MAX_SCANLINE_WIDTH; ++i)
				{
					*plane_metapixels_pointer = constant->blit_lookup[sprite_metapixels_pointer[0]][*plane_metapixels_pointer][sprite_metapixels_pointer[1]] & 0x3F;
					++plane_metapixels_pointer;
					sprite_metapixels_pointer += 2;
				}
			}
		}

		#undef MAX_SPRITE_WIDTH
	}

	/* Send pixels to the frontend to be displayed */
	scanline_rendered_callback((void*)scanline_rendered_callback_user_data, scanline, plane_metapixels + 16, (state->h40_enabled ? 40 : 32) * 8, (state->v30_enabled ? 30 : 28) << tile_height_power);
}

cc_u16f VDP_ReadData(const VDP* const vdp)
{
	cc_u16f value = 0;

	vdp->state->access.write_pending = cc_false;

	if (!IsInReadMode(vdp->state))
	{
		/* According to GENESIS SOFTWARE DEVELOPMENT MANUAL (COMPLEMENT) section 4.1,
		   this should cause the 68k to hang */
		/* TODO */
		LogMessage("Data was read from the VDP data port while the VDP was in write mode");
	}
	else
	{
		value = ReadAndIncrement(vdp->state);
	}

	return value;
}

cc_u16f VDP_ReadControl(const VDP* const vdp)
{
	/* TODO */
	const cc_bool currently_in_hblank = cc_true;
	const cc_bool fifo_empty = cc_true;

	/* Reading from the control port aborts the VDP waiting for a second command word to be written.
	   This doesn't appear to be documented in the official SDK manuals we have, but the official
	   boot code makes use of this feature. */
	vdp->state->access.write_pending = cc_false;

	return 0x3400 | (fifo_empty << 9) | (vdp->state->currently_in_vblank << 3) | (currently_in_hblank << 2); /* The H-blank bit is forced for now so Sonic 2's two-player mode works */
}

void VDP_WriteData(const VDP* const vdp, const cc_u16f value, const VDP_ColourUpdatedCallback colour_updated_callback, const void* const colour_updated_callback_user_data)
{
	vdp->state->access.write_pending = cc_false;

	if (IsInReadMode(vdp->state))
	{
		/* Invalid input, but defined behaviour */
		LogMessage("Data was written to the VDP data port while the VDP was in read mode");

		/* According to GENESIS SOFTWARE DEVELOPMENT MANUAL (COMPLEMENT) section 4.1,
		   data should not be written, but the address should be incremented */
		vdp->state->access.address_register += vdp->state->access.increment;
	}
	else
	{
		/* Write the value to memory */
		WriteAndIncrement(vdp->state, value, colour_updated_callback, colour_updated_callback_user_data);

		if (IsDMAPending(vdp->state))
		{
			/* Perform DMA fill */
			/* TODO: https://gendev.spritesmind.net/forum/viewtopic.php?p=31857&sid=34ef0ab3215fa6ceb29e12db824c3427#p31857 */
			ClearDMAPending(vdp->state);

			do
			{
				WriteVRAM(vdp->state, vdp->state->access.address_register ^ 1, (cc_u8f)(value >> 8));
				vdp->state->access.address_register += vdp->state->access.increment;

				/* Yes, even DMA fills do this, according to
				   'https://gendev.spritesmind.net/forum/viewtopic.php?p=21016#p21016'. */
				++vdp->state->dma.source_address_low;
				vdp->state->dma.source_address_low &= 0xFFFF;
			} while (--vdp->state->dma.length, vdp->state->dma.length &= 0xFFFF, vdp->state->dma.length != 0);
		}
	}
}

/* TODO - Retention of partial commands */
void VDP_WriteControl(const VDP* const vdp, const cc_u16f value, const VDP_ColourUpdatedCallback colour_updated_callback, const void* const colour_updated_callback_user_data, const VDP_ReadCallback read_callback, const void* const read_callback_user_data, const VDP_KDebugCallback kdebug_callback, const void* const kdebug_callback_user_data)
{
	if (vdp->state->access.write_pending)
	{
		/* This is an "address set" command (part 2). */
		const cc_u16f code_bitmask = vdp->state->dma.enabled ? 0x3C : 0x1C;

		vdp->state->access.write_pending = cc_false;
		vdp->state->access.address_register = (vdp->state->access.address_register & 0x3FFF) | ((value & 3) << 14);
		vdp->state->access.code_register = (vdp->state->access.code_register & ~code_bitmask) | ((value >> 2) & code_bitmask);
	}
	else if ((value & 0xC000) == 0x8000)
	{
		/* This is a "register set" command. */
		const cc_u16f reg = (value >> 8) & 0x1F;
		const cc_u16f data = value & 0xFF;

		/* This is relied upon by Sonic 3D Blast (the opening FMV will have broken colours otherwise). */
		/* TODO: Does the DMA enable bit prevent this from clearing bit 5? */
		vdp->state->access.code_register = 0;

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
				vdp->state->plane_a_address = (data & 0x38) << 10;
				break;

			case 3:
				/* PATTERN NAME TABLE BASE ADDRESS FOR WINDOW */
				/* TODO: The lowest bit is invalid is H40 mode according to the 'Genesis Software Manual'. */
				/* http://techdocs.exodusemulator.com/Console/SegaMegaDrive/Documentation.html#mega-drive-documentation */
				vdp->state->window_address = (data & 0x3E) << 10;
				break;

			case 4:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL B */
				vdp->state->plane_b_address = (data & 7) << 13;
				break;

			case 5:
				/* SPRITE ATTRIBUTE TABLE BASE ADDRESS */
				vdp->state->sprite_table_address = (data & 0x7F) << 9;

				/* Real VDPs partially cache the sprite table, and forget to update it
				   when the sprite table base address is changed. Replicating this
				   behaviour may be needed in order to properly emulate certain effects
				   that involve manipulating the sprite table during rendering. */
				/*vdp->state->sprite_row_cache.needs_updating = cc_true;*/ /* The VDP does not do this! */

				break;

			case 7:
				/* BACKGROUND COLOR */
				vdp->state->background_colour = data & 0x3F;
				break;

			case 10:
				/* H INTERRUPT REGISTER */
				vdp->state->h_int_interval = (cc_u8l)data;
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
						/* TODO: Some unauthorised EA games use this, and it acts as
						   a slightly unstable version of one of the other modes. */
						LogMessage("Prohibitied H-scroll mode selected");
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
				vdp->state->hscroll_address = (data & 0x3F) << 10;
				break;

			case 15:
				/* AUTO INCREMENT DATA */
				vdp->state->access.increment = (cc_u16l)data;
				break;

			case 16:
			{
				/* SCROLL SIZE */
				const cc_u16f width_index  = (data >> 0) & 3;
				const cc_u16f height_index = (data >> 4) & 3;

				if ((width_index == 3 && height_index != 0) || (height_index == 3 && width_index != 0))
				{
					/* TODO: So... what should happen? */
					LogMessage("Selected plane size exceeds 64x64/32x128/128x32");
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
							/* TODO: I swear some dumb Electronic Arts game uses this. */
							LogMessage("Prohibited plane width selected");
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
							/* TODO: I swear some dumb Electronic Arts game uses this. */
							LogMessage("Prohibited plane height selected");
							break;

						case 3:
							vdp->state->plane_height = 128;
							break;
					}
				}

				break;
			}

			case 17:
				/* WINDOW H POSITION */
				vdp->state->window.aligned_right = (data & 0x80) != 0;
				vdp->state->window.horizontal_boundary = (data & 0x1F) * 2; /* Measured in tiles. */
				break;

			case 18:
				/* WINDOW V POSITION */
				vdp->state->window.aligned_bottom = (data & 0x80) != 0;
				vdp->state->window.vertical_boundary = (data & 0x1F) * 8; /* Measured in scanlines. */
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
				if ((data & 0x80) != 0)
				{
					vdp->state->dma.source_address_high = data & 0x3F;
					vdp->state->dma.mode = (data & 0x40) != 0 ? VDP_DMA_MODE_COPY : VDP_DMA_MODE_FILL;
				}
				else
				{
					vdp->state->dma.source_address_high = data & 0x7F;
					vdp->state->dma.mode = VDP_DMA_MODE_MEMORY_TO_VRAM;
				}

				break;

			case 30:
			{
				/* Gens KMod debug register. Does not exist in real Mega Drives, but is a useful emulator feature for debugging. */
				const char character = CC_SIGN_EXTEND(int, 7, (int)data);

				/* This behaviour exactly matches Gens KMod v0.7. */
				if (character < 0x20 && character != '\0')
					break;

				vdp->state->kdebug_buffer[vdp->state->kdebug_buffer_index++] = character;

				/* The last byte of the buffer is always set to 0, so we don't need to do it here. */
				if (character == '\0' || vdp->state->kdebug_buffer_index == CC_COUNT_OF(vdp->state->kdebug_buffer) - 1)
				{
					vdp->state->kdebug_buffer_index = 0;
					kdebug_callback((void*)kdebug_callback_user_data, vdp->state->kdebug_buffer);
				}

				break;
			}

			case 6:
			case 8:
			case 9:
			case 14:
				/* Unused legacy register */
				break;

			default:
				/* Invalid */
				LogMessage("Attempted to set invalid VDP register (0x%" CC_PRIXFAST16 ")", reg);
				break;
		}
	}
	else
	{
		/* This is an "address set" command (part 1). */
		vdp->state->access.write_pending = cc_true;
		vdp->state->access.address_register = (value & 0x3FFF) | (vdp->state->access.address_register & (3 << 14));
		vdp->state->access.code_register = ((value >> 14) & 3) | (vdp->state->access.code_register & 0x3C);
	}

	switch ((vdp->state->access.code_register >> 1) & 7)
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
			vdp->state->access.selected_buffer = VDP_ACCESS_INVALID;
			break;
	}

	if (IsDMAPending(vdp->state) && vdp->state->dma.mode != VDP_DMA_MODE_FILL)
	{
		/* Firing DMA */
		ClearDMAPending(vdp->state);

		do
		{
			if (vdp->state->dma.mode == VDP_DMA_MODE_MEMORY_TO_VRAM)
			{
				WriteAndIncrement(vdp->state, read_callback((void*)read_callback_user_data, ((cc_u32f)vdp->state->dma.source_address_high << 17) | ((cc_u32f)vdp->state->dma.source_address_low << 1)), colour_updated_callback, colour_updated_callback_user_data);
			}
			else /*if (state->dma.mode == VDP_DMA_MODE_COPY)*/
			{
				WriteVRAM(vdp->state, vdp->state->access.address_register ^ 1, vdp->state->vram[vdp->state->dma.source_address_low ^ 1]);
				vdp->state->access.address_register += vdp->state->access.increment;
			}

			/* Emulate the 128KiB DMA wrap-around bug. */
			++vdp->state->dma.source_address_low;
			vdp->state->dma.source_address_low &= 0xFFFF;
		} while (--vdp->state->dma.length, vdp->state->dma.length &= 0xFFFF, vdp->state->dma.length != 0);
	}
}

cc_u16f VDP_ReadVRAMWord(const VDP_State* const state, const cc_u16f address)
{
	assert(address < CC_COUNT_OF(state->vram));
	return (state->vram[address ^ 0] << 8) | state->vram[address ^ 1];
}

VDP_TileMetadata VDP_DecomposeTileMetadata(const cc_u16f packed_tile_metadata)
{
	VDP_TileMetadata tile_metadata;

	tile_metadata.tile_index = packed_tile_metadata & 0x7FF;
	tile_metadata.palette_line = (packed_tile_metadata >> 13) & 3;
	tile_metadata.x_flip = (packed_tile_metadata & 0x800) != 0;
	tile_metadata.y_flip = (packed_tile_metadata & 0x1000) != 0;
	tile_metadata.priority = (packed_tile_metadata & 0x8000) != 0;

	return tile_metadata;
}

VDP_CachedSprite VDP_GetCachedSprite(const VDP_State* const state, const cc_u16f sprite_index)
{
	VDP_CachedSprite cached_sprite;

	const cc_u8l* const bytes = state->sprite_table_cache[sprite_index];

	cached_sprite.y = ((bytes[0] & 3) << 8) | bytes[1];
	cached_sprite.width = ((bytes[2] >> 2) & 3) + 1;
	cached_sprite.height = (bytes[2] & 3) + 1;
	cached_sprite.link = bytes[3] & 0x7F;

	return cached_sprite;
}
