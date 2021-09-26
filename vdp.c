#include "vdp.h"

#include <stddef.h>

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
	/* TODO - Reading from the data port causes real Mega Drives to crash (if the VDP isn't in read mode) */
	/* TODO */
	(void)state;
	return 0;
}

unsigned short VDP_ReadControl(VDP_State *state)
{
	/* TODO */
	(void)state;
	return 0;
}

void VDP_WriteData(VDP_State *state, unsigned short value)
{
	/* TODO */
	(void)state;
	(void)value;
}

void VDP_WriteControl(VDP_State *state, unsigned short value)
{
	/* TODO */
	(void)state;
	(void)value;
}
