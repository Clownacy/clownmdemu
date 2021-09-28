#include "vdp.h"

#include <stddef.h>

#include "clowncommon.h"

#include "error.h"

/* This essentially pre-computes the VDP's depth-test and alpha-test,
   generating a lookup table to eliminate the need to perform these
   every time a pixel is blitted. This provides a *massive* speed boost. */
static void InitBlitLookupTable(VDP_State *state)
{
	const unsigned int palette_index_mask = 0xF;
	const unsigned int priority_mask = 0x40;

	unsigned int old, new;

	for (old = 0; old < VDP_INDEXED_PIXEL_VARIATION; ++old)
	{
		for (new = 0; new < VDP_INDEXED_PIXEL_VARIATION; ++new)
		{
			unsigned char output;

			if ((new & palette_index_mask) == 0)
				output = old;	/* New pixel failed the alpha-test */
			else if (!(old & priority_mask) || (old & palette_index_mask) == 0)
				output = new;	/* Old pixel had lower priority or it was transparent */
			else if (!(new & priority_mask))
				output = old;	/* New pixel had lower priority */
			else
				output = new;	/* New pixel had higher priority */

			/* Don't allow transparent pixels to have priority bit
			   (think of a low-priority plane A pixel being drawn
			   over a high-priority transparent plane B pixel) */
			if ((output & palette_index_mask) == 0)
				output &= ~priority_mask;

			state->blit_lookup[old][new] = output;
		}
	}
}

static unsigned short* DecodeAndIncrementAccessAddress(VDP_State *state)
{
	unsigned short *address = &state->access.selected_buffer[(state->access.index / 2) & state->access.selected_buffer_size_mask];

	state->access.index += state->access.increment;

	return address;
}

static unsigned char GetPixelFromPlane(VDP_State *state, unsigned short x, unsigned short y, unsigned short *plane_buffer, unsigned short *hscroll_buffer, unsigned short *vscroll_buffer)
{
	unsigned int hscroll, vscroll;
	unsigned int pixel_x_in_plane, pixel_y_in_plane;
	unsigned int pixel_x_in_tile, pixel_y_in_tile;
	unsigned int tile_x, tile_y;

	unsigned int tile_metadata;
	unsigned int tile_index;
	cc_bool tile_priority;
	unsigned int tile_palette_line;
	cc_bool tile_y_flip;
	cc_bool tile_x_flip;

	unsigned int tile_data;
	unsigned int palette_line_index;

	/* Get the horizontal scroll value */
	switch (state->hscroll_mode)
	{
		case VDP_HSCROLL_MODE_FULL:
			hscroll = hscroll_buffer[0];
			break;

		case VDP_HSCROLL_MODE_1CELL:
			hscroll = hscroll_buffer[(y / 8) * 2];
			break;

		case VDP_HSCROLL_MODE_1LINE:
			hscroll = hscroll_buffer[y * 2];
			break;
	}

	/* Get the vertical scroll value */
	switch (state->vscroll_mode)
	{
		case VDP_VSCROLL_MODE_FULL:
			vscroll = vscroll_buffer[0];
			break;

		case VDP_VSCROLL_MODE_2CELL:
			vscroll = vscroll_buffer[(x / 16) * 2];
			break;
	}

	/* Get the coordinates of the pixel to be drawn (in the plane) */
	pixel_x_in_plane = -hscroll + x;
	pixel_y_in_plane = -vscroll + y;

	/* Get the coordinates of the pixel to be drawn (in the tile) */
	pixel_x_in_tile = pixel_x_in_plane & 7;
	pixel_y_in_tile = pixel_y_in_plane & 7;

	/* Get the coordinates of the tile to be drawn */
	tile_x = (pixel_x_in_plane / 8) & state->plane_width_bitmask;
	tile_y = (pixel_y_in_plane / 8) & state->plane_height_bitmask;

	/* Obtain and decode tile metadata */
	tile_metadata = plane_buffer[tile_y * state->plane_width + tile_x];
	tile_index = tile_metadata & 0x7FF;
	tile_priority = !!(tile_metadata & 0x8000);
	tile_palette_line = (tile_metadata >> 13) & 3;
	tile_y_flip = !!(tile_metadata & 0x1000);
	tile_x_flip = !!(tile_metadata & 0x0800);

	/* Perform tile flipping if needed */
	pixel_x_in_tile ^= tile_x_flip ? 7 : 0;
	pixel_y_in_tile ^= tile_y_flip ? 7 : 0;

	/* Read a word of tile data */
	tile_data = state->vram[tile_index * (8 * 8 / 4) + pixel_y_in_tile * 2 + pixel_x_in_tile / 4];

	/* Obtain the index into the palette line */
	palette_line_index = (tile_data >> (4 * ((pixel_x_in_tile & 3) ^ 3))) & 0xF;

	/* Merge the priority, palette line, and palette line index into the final pixel */
	return palette_line_index | (tile_palette_line << 4) | (tile_priority << 6);
}

void VDP_Init(VDP_State *state)
{
	state->access.write_pending = cc_false;

	state->access.read_mode = cc_false;
	state->access.selected_buffer = state->vram;
	state->access.selected_buffer_size_mask = CC_COUNT_OF(state->vram) - 1;
	state->access.index = 0;
	state->access.increment = 0;

	state->dma.awaiting_destination_address = cc_false;
	state->dma.source_address = 0;

	state->plane_a_address = 0;
	state->plane_b_address = 0;
	state->window_address = 0;
	state->sprite_table_address = 0;
	state->hscroll_address = 0;

	state->plane_width = 32;
	state->plane_height = 32;
	state->plane_width_bitmask = state->plane_width - 1;
	state->plane_height_bitmask = state->plane_height - 1;

	state->screen_width = 320;
	state->screen_height = 224;

	state->hscroll_mode = VDP_HSCROLL_MODE_FULL;
	state->vscroll_mode = VDP_VSCROLL_MODE_FULL;

	InitBlitLookupTable(state);
}

void VDP_RenderScanline(VDP_State *state, unsigned short scanline, void (*scanline_rendered_callback)(unsigned short scanline, void *pixels, unsigned short screen_width, unsigned short screen_height))
{
	unsigned char pixels[MAX_SCANLINE_WIDTH * 3];
	unsigned short i;

	for (i = 0; i < state->screen_width; ++i)
	{
		unsigned char plane_b_pixel;
		unsigned char plane_a_pixel;
		unsigned char final_pixel;

		unsigned short colour;
		unsigned char red;
		unsigned char green;
		unsigned char blue;

		/* Get Plane B pixel */
		plane_b_pixel = GetPixelFromPlane(state, i, scanline, state->vram + state->plane_b_address, state->vram + state->hscroll_address + 1, state->vsram + 1);

		/* Get Plane A pixel */
		plane_a_pixel = GetPixelFromPlane(state, i, scanline, state->vram + state->plane_a_address, state->vram + state->hscroll_address + 0, state->vsram + 0);

		final_pixel = state->blit_lookup[plane_b_pixel][plane_a_pixel];

		/* Obtain the Mega Drive-format colour from Colour RAM */
		colour = state->cram[final_pixel & 0x7F];

		/* Decompose the colour into its individual RGB colour channels */
		red = (colour >> 1) & 7;
		green = (colour >> 5) & 7;
		blue = (colour >> 9) & 7;

		/* Rearrange the colour into RGB24 */
		pixels[i * 3 + 0] = red << 5;
		pixels[i * 3 + 1] = green << 5;
		pixels[i * 3 + 2] = blue << 5;
	}

	scanline_rendered_callback(scanline, pixels, state->screen_width, state->screen_height);
}

unsigned short VDP_ReadData(VDP_State *state)
{
	unsigned short value = 0;

	if (!state->access.read_mode)
	{
		/* TODO - If I remember right, real Mega Drives crash when this happens */
		PrintError("Data was read from the VDP data port while the VDP was in write mode");
	}
	else
	{
		value = *DecodeAndIncrementAccessAddress(state);
	}

	return value;
}

unsigned short VDP_ReadControl(VDP_State *state)
{
	/* TODO */

	/* Reading from the control port aborts the VDP waiting for a second command word to be written.
	   This doesn't appear to be documented in the official SDK manuals we have, but the official
	   boot code makes use of this feature. */
	state->access.write_pending = cc_false;

	/* ...Not sure about this though. */
	state->dma.awaiting_destination_address = cc_false;

	return 0;
}

void VDP_WriteData(VDP_State *state, unsigned short value)
{
	if (state->access.read_mode)
	{
		PrintError("Data was written to the VDP data port while the VDP was in read mode");
	}
	else
	{
		*DecodeAndIncrementAccessAddress(state) = value;
	}
}

void VDP_WriteControl(VDP_State *state, unsigned short value, unsigned short (*read_callback)(void *user_data, unsigned long address), void *user_data)
{
	if (state->access.write_pending)
	{
		/* This command is setting up for a memory access (part 2) */
		const unsigned short destination_address = (state->access.cached_write & 0x3FFF) | ((value & 3) << 14);
		unsigned char access_mode = ((state->access.cached_write >> 14) & 3) | ((value >> 2) & 0x3C);

		if (state->dma.awaiting_destination_address)
			access_mode &= 7;

		state->access.write_pending = cc_false;

		state->access.index = destination_address;
		state->access.read_mode = (state->access.cached_write & 0xC000) == 0;

		switch (access_mode)
		{
			case 0: /* VRAM read */
			case 1: /* VRAM write */
				state->access.selected_buffer = state->vram;
				state->access.selected_buffer_size_mask = CC_COUNT_OF(state->vram) - 1;
				break;

			case 8: /* CRAM read */
			case 3: /* CRAM write */
				state->access.selected_buffer = state->cram;
				state->access.selected_buffer_size_mask = CC_COUNT_OF(state->cram) - 1;
				break;

			case 4: /* VSRAM read */
			case 5: /* VSRAM write */
				state->access.selected_buffer = state->vsram;
				state->access.selected_buffer_size_mask = CC_COUNT_OF(state->vsram) - 1;
				break;

			default: /* Invalid */
				PrintError("Invalid VDP access mode specified (0x%X)", access_mode);
				break;
		}

		if (state->dma.awaiting_destination_address)
		{
			/* Firing DMA */
			unsigned short i;

			state->dma.awaiting_destination_address = cc_false;

			switch (state->dma.mode)
			{
				case VDP_DMA_MODE_MEMORY_TO_VRAM:
					for (i = 0; i < state->dma.length; ++i)
					{
						const unsigned long source_address_high_bits = state->dma.source_address & ~0xFFFFul;
						unsigned short source_address_low_bits = (unsigned short)state->dma.source_address & 0xFFFFul;

						*DecodeAndIncrementAccessAddress(state) = read_callback(user_data, source_address_high_bits | source_address_low_bits);

						source_address_low_bits += 2;
						source_address_low_bits &= 0xFFFF;
					}

					break;

				case VDP_DMA_MODE_FILL:
					/* TODO */
					break;

				case VDP_DMA_MODE_COPY:
					/* TODO */
					break;
			}
		}
	}
	else if ((value & 0xC000) != 0x8000)
	{
		/* This command is setting up for a memory access (part 1) */
		state->access.write_pending = cc_true;
		state->access.cached_write = value;
	}
	else
	{
		const unsigned char reg = (value >> 8) & 0x3F;
		const unsigned char data = value & 0xFF;

		/* This command is setting a register */
		switch (reg)
		{
			case 0:
				/* MODE SET REGISTER NO.1 */
				/* TODO */
				break;

			case 1:
				/* MODE SET REGISTER NO.2 */
				/* TODO */
				break;

			case 2:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL A */
				state->plane_a_address = (data & 0x38) << (10 - 1);
				break;

			case 3:
				/* PATTERN NAME TABLE BASE ADDRESS FOR WINDOW */
				state->window_address = (data & 0x3E) << (10 - 1);
				break;

			case 4:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL B */
				state->plane_b_address = (data & 7) << (13 - 1);
				break;

			case 5:
				/* SPRITE ATTRIBUTE TABLE BASE ADDRESS */
				state->sprite_table_address = (data & 0x7F) << (9 - 1);
				break;

			case 7:
				/* BACKGROUND COLOR */
				/* TODO */
				break;

			case 10:
				/* H INTERRUPT REGISTER */
				/* TODO */
				break;

			case 11:
				/* MODE SET REGISTER NO.3 */

				/* TODO - External interrupt */

				state->vscroll_mode = data & 4 ? VDP_VSCROLL_MODE_2CELL : VDP_VSCROLL_MODE_FULL;

				switch (data & 3)
				{
					case 0:
						state->hscroll_mode = VDP_HSCROLL_MODE_FULL;
						break;

					case 1:
						PrintError("Prohibitied H-scroll mode selected");
						break;

					case 2:
						state->hscroll_mode = VDP_HSCROLL_MODE_1CELL;
						break;

					case 3:
						state->hscroll_mode = VDP_HSCROLL_MODE_1LINE;
						break;
				}

				break;

			case 12:
				/* MODE SET REGISTER NO.4 */
				/* TODO */
				break;

			case 13:
				/* H SCROLL DATA TABLE BASE ADDRESS */
				state->hscroll_address = (data & 0x3F) << (10 - 1);
				break;

			case 15:
				/* AUTO INCREMENT DATA */
				state->access.increment = data;
				break;

			case 16:
			{
				/* SCROLL SIZE */
				const unsigned char width_index  = (data >> 0) & 3;
				const unsigned char height_index = (data >> 4) & 3;

				if ((width_index == 3 && height_index != 0) || (height_index == 3 && width_index != 0))
				{
					PrintError("Selected plane size exceeds 64x64/32x128/128x32");
				}
				else
				{
					switch (width_index)
					{
						case 0:
							state->plane_width = 32;
							break;

						case 1:
							state->plane_width = 64;
							break;

						case 2:
							PrintError("Prohibited plane width selected");
							break;

						case 3:
							state->plane_width = 128;
							break;
					}

					switch (height_index)
					{
						case 0:
							state->plane_height = 32;
							break;

						case 1:
							state->plane_height = 64;
							break;

						case 2:
							PrintError("Prohibited plane height selected");
							break;

						case 3:
							state->plane_height = 128;
							break;
					}

					state->plane_width_bitmask = state->plane_width - 1;
					state->plane_height_bitmask = state->plane_height - 1;
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
				state->dma.length &= ~(0xFF << 0);
				state->dma.length |= data << 0;
				break;

			case 20:
				/* DMA LENGTH COUNTER HIGH */
				state->dma.length &= ~(0xFF << 8);
				state->dma.length |= data << 8;
				break;

			case 21:
				/* DMA SOURCE ADDRESS LOW */
				state->dma.source_address &= ~(0xFF << 0);
				state->dma.source_address |= data << 0;
				break;

			case 22:
				/* DMA SOURCE ADDRESS MID. */
				state->dma.source_address &= ~(0xFF << 8);
				state->dma.source_address |= data << 8;
				break;

			case 23:
				/* DMA SOURCE ADDRESS HIGH */
				state->dma.source_address &= ~(0xFF << 16);

				if (data & 0x80)
				{
					state->dma.source_address |= (data & 0x7F) << 16;
					state->dma.mode = VDP_DMA_MODE_MEMORY_TO_VRAM;
				}
				else
				{
					state->dma.source_address |= (data & 0x3F) << 16;
					state->dma.mode = data & 0x40 ? VDP_DMA_MODE_COPY : VDP_DMA_MODE_FILL;
				}

				state->dma.awaiting_destination_address = cc_true;

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
