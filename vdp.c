#include "vdp.h"

#include <stddef.h>

#include "error.h"

void VDP_Init(VDP_State *state)
{
	/* TODO */
	(void)state;
	/* Initialise VDP lookup table */
}

void VDP_RenderScanline(VDP_State *state, void (*scanline_rendered_callback)(void *pixels, size_t screen_width, size_t screen_height))
{
	unsigned char scanline[MAX_SCANLINE_WIDTH];

	(void)state;

	scanline_rendered_callback(scanline, 320, 224);
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
		unsigned char *buffer = &state->access.selected_buffer[(state->access.index * 2) & state->access.selected_buffer_size];
		value |= *buffer++ << 8;
		value |= *buffer++ << 0;
	}

	return value;
}

unsigned short VDP_ReadControl(VDP_State *state)
{
	/* TODO */

	/* Reading from the control port aborts the VDP waiting for a second command word to be written.
	   This doesn't appear to be documented in the official SDK manuals we have, but the official
	   boot code makes use of this features. */
	state->write_pending = cc_false;

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
		unsigned char *buffer = &state->access.selected_buffer[(state->access.index * 2) & state->access.selected_buffer_size];
		*buffer++ = (unsigned char)(value >> 8) & 0xFF;
		*buffer++ = (unsigned char)(value >> 0) & 0xFF;
	}
}

void VDP_WriteControl(VDP_State *state, unsigned short value)
{
	if (state->write_pending)
	{
		/* This command is setting up for a memory access (part 2) */
		const unsigned char access_mode = ((state->cached_write >> 12) & 3) | ((value >> 2) & 0x3C);

		state->write_pending = cc_false;
		state->access.index = (state->cached_write & 0x3FFF) | ((value & 3) << 14);
		state->access.read_mode = (state->cached_write & 0xC000) == 0;

		switch (access_mode)
		{
			case 0: /* VRAM read */
			case 1: /* VRAM write */
				state->access.selected_buffer = state->vram;
				state->access.selected_buffer_size = sizeof(state->vram) - 1;
				break;

			case 8: /* CRAM read */
			case 3: /* CRAM write */
				state->access.selected_buffer = state->cram;
				state->access.selected_buffer_size = sizeof(state->cram) - 1;
				break;

			case 4: /* VSRAM read */
			case 5: /* VSRAM write */
				state->access.selected_buffer = state->vsram;
				state->access.selected_buffer_size = sizeof(state->vsram) - 1;
				break;

			default: /* Invalid */
				PrintError("Invalid VDP access mode specified (0x%X)", access_mode);
				break;
		}
	}
	else if ((value & 0xC000) != 0x8000)
	{
		/* This command is setting up for a memory access (part 1) */
		state->write_pending = cc_true;
		state->cached_write = value;
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
				/* TODO */
				break;

			case 3:
				/* PATTERN NAME TABLE BASE ADDRESS FOR WINDOW */
				/* TODO */
				break;

			case 4:
				/* PATTERN NAME TABLE BASE ADDRESS FOR SCROLL B */
				/* TODO */
				break;

			case 5:
				/* SPRITE ATTRIBUTE TABLE BASE ADDRESS */
				/* TODO */
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
				/* TODO */
				break;

			case 12:
				/* MODE SET REGISTER NO.4 */
				/* TODO */
				break;

			case 13:
				/* H SCROLL DATA TABLE BASE ADDRESS */
				/* TODO */
				break;

			case 15:
				/* AUTO INCREMENT DATA */
				/* TODO */
				break;

			case 16:
				/* SCROLL SIZE */
				/* TODO */
				break;

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
				/* TODO */
				break;

			case 20:
				/* DMA LENGTH COUNTER HIGH */
				/* TODO */
				break;

			case 21:
				/* DMA SOURCE ADDRESS LOW */
				/* TODO */
				break;

			case 22:
				/* DMA SOURCE ADDRESS MID. */
				/* TODO */
				break;

			case 23:
				/* DMA SOURCE ADDERSS HIGH */
				/* TODO */
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
