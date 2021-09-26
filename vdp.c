#include "vdp.h"

#include <stddef.h>

#define MAX_SCANLINE_WIDTH 320

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

unsigned short VDP_ReadStatus(VDP_State *state)
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
