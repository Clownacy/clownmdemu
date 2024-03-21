#include "clownmdemu.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "clowncommon/clowncommon.h"

#include "bus-main-m68k.h"
#include "bus-sub-m68k.h"
#include "bus-z80.h"
#include "clown68000/interpreter/clown68000.h"
#include "error.h"
#include "fm.h"
#include "psg.h"
#include "vdp.h"
#include "z80.h"

#define MAX_ROM_SIZE (1024 * 1024 * 4) /* 4MiB */

static cc_u32f ReadU32BE(const cc_u8l* const bytes)
{
	cc_u8f i;
	cc_u32f value;

	value = 0;

	for (i = 0; i < 4; ++i)
		value |= (cc_u32f)bytes[i] << (8 * (4 - 1 - i));

	return value;
}

static void BytesTo68kRAM(cc_u16l* const ram, const cc_u8l* const bytes, const size_t total_bytes)
{
	size_t i;

	for (i = 0; i < total_bytes / 2; ++i)
		ram[i] = ((cc_u16f)bytes[i * 2 + 0] << 8) | bytes[i * 2 + 1];
}

static void CDSectorTo68kRAM(const ClownMDEmu_Callbacks* const callbacks, cc_u16l* const ram)
{
	const cc_u8l* const sector_bytes = callbacks->cd_sector_read((void*)callbacks->user_data);

	BytesTo68kRAM(ram, sector_bytes, 0x800);
}

static void CDSectorsTo68kRAM(const ClownMDEmu_Callbacks* const callbacks, cc_u16l* const ram, const cc_u32f start, const cc_u32f length)
{
	cc_u32f i;

	callbacks->cd_seeked((void*)callbacks->user_data, start / 0x800);

	for (i = 0; i < CC_DIVIDE_CEILING(length, 0x800); ++i)
		CDSectorTo68kRAM(callbacks, &ram[i * 0x800 / 2]);
}

void ClownMDEmu_Constant_Initialise(ClownMDEmu_Constant *constant)
{
	Z80_Constant_Initialise(&constant->z80);
	VDP_Constant_Initialise(&constant->vdp);
	FM_Constant_Initialise(&constant->fm);
	PSG_Constant_Initialise(&constant->psg);
}

static cc_bool FrontendControllerCallbackCommon(void* const user_data, const Controller_Button button, const cc_u8f joypad_index)
{
	ClownMDEmu_Button frontend_button;

	const ClownMDEmu_Callbacks *frontend_callbacks = (const ClownMDEmu_Callbacks*)user_data;

	switch (button)
	{
		case CONTROLLER_BUTTON_UP:
			frontend_button = CLOWNMDEMU_BUTTON_UP;
			break;

		case CONTROLLER_BUTTON_DOWN:
			frontend_button = CLOWNMDEMU_BUTTON_DOWN;
			break;

		case CONTROLLER_BUTTON_LEFT:
			frontend_button = CLOWNMDEMU_BUTTON_LEFT;
			break;

		case CONTROLLER_BUTTON_RIGHT:
			frontend_button = CLOWNMDEMU_BUTTON_RIGHT;
			break;

		case CONTROLLER_BUTTON_A:
			frontend_button = CLOWNMDEMU_BUTTON_A;
			break;

		case CONTROLLER_BUTTON_B:
			frontend_button = CLOWNMDEMU_BUTTON_B;
			break;

		case CONTROLLER_BUTTON_C:
			frontend_button = CLOWNMDEMU_BUTTON_C;
			break;

		case CONTROLLER_BUTTON_START:
			frontend_button = CLOWNMDEMU_BUTTON_START;
			break;

		case CONTROLLER_BUTTON_TOTAL:
			assert(cc_false);
			/* Fallthrough */
		/* TODO: These buttons. */
		case CONTROLLER_BUTTON_X:
		case CONTROLLER_BUTTON_Y:
		case CONTROLLER_BUTTON_Z:
		case CONTROLLER_BUTTON_MODE:
			return cc_false;
	}

	return frontend_callbacks->input_requested((void*)frontend_callbacks->user_data, joypad_index, frontend_button);
}

static cc_bool FrontendController1Callback(void* const user_data, const Controller_Button button)
{
	return FrontendControllerCallbackCommon(user_data, button, 0);
}

static cc_bool FrontendController2Callback(void* const user_data, const Controller_Button button)
{
	return FrontendControllerCallbackCommon(user_data, button, 1);
}

static cc_u8f IOPortToController_ReadCallback(void* const user_data, const cc_u16f cycles)
{
	const IOPortToController_Parameters *parameters = (const IOPortToController_Parameters*)user_data;

	return Controller_Read(parameters->controller, cycles, parameters->frontend_callbacks);
}

static void IOPortToController_WriteCallback(void* const user_data, const cc_u8f value, const cc_u16f cycles)
{
	const IOPortToController_Parameters *parameters = (const IOPortToController_Parameters*)user_data;

	Controller_Write(parameters->controller, value, cycles);
}

void ClownMDEmu_State_Initialise(ClownMDEmu_State *state)
{
	cc_u16f i;

	/* M68K */
	state->m68k.cycle_countdown = 1;

	/* Z80 */
	Z80_State_Initialise(&state->z80.state);
	state->z80.cycle_countdown = 1;
	state->z80.bank = 0;
	state->z80.bus_requested = cc_true;
	state->z80.reset_held = cc_true;

	VDP_State_Initialise(&state->vdp);
	FM_State_Initialise(&state->fm);
	PSG_State_Initialise(&state->psg);

	for (i = 0; i < CC_COUNT_OF(state->io_ports); ++i)
	{
		/* The standard Sega SDK bootcode uses this to detect soft-resets. */
		IOPort_Initialise(&state->io_ports[i]);
	}

	IOPort_SetCallbacks(&state->io_ports[0], IOPortToController_ReadCallback, IOPortToController_WriteCallback);
	IOPort_SetCallbacks(&state->io_ports[1], IOPortToController_ReadCallback, IOPortToController_WriteCallback);

	Controller_Initialise(&state->controllers[0], FrontendController1Callback);
	Controller_Initialise(&state->controllers[1], FrontendController2Callback);

	/* Mega CD */
	state->mega_cd.m68k.cycle_countdown = 1;
	state->mega_cd.m68k.bus_requested = cc_true;
	state->mega_cd.m68k.reset_held = cc_true;

	state->mega_cd.prg_ram.bank = 0;

	state->mega_cd.word_ram.in_1m_mode = cc_true; /* Confirmed by my Visual Sound Test homebrew. */
	/* Page 24 of MEGA-CD HARDWARE MANUAL confirms this. */
	state->mega_cd.word_ram.dmna = cc_false;
	state->mega_cd.word_ram.ret = cc_true;

	state->mega_cd.communication.flag = 0;

	for (i = 0; i < CC_COUNT_OF(state->mega_cd.communication.command); ++i)
		state->mega_cd.communication.command[i] = 0;

	for (i = 0; i < CC_COUNT_OF(state->mega_cd.communication.status); ++i)
		state->mega_cd.communication.status[i] = 0;

	state->mega_cd.cd.current_sector = 0;
	state->mega_cd.cd.total_buffered_sectors = 0;
	state->mega_cd.cd.cdc_delay = 0;
	state->mega_cd.cd.cdc_ready = cc_false;
	
	for (i = 0; i < CC_COUNT_OF(state->mega_cd.irq.enabled); ++i)
		state->mega_cd.irq.enabled[i] = cc_false;

	state->mega_cd.irq.irq1_pending = cc_false;
	state->mega_cd.irq.irq3_countdown_master = state->mega_cd.irq.irq3_countdown = 0;

	PCM_State_Initialise(&state->mega_cd.pcm);

	state->mega_cd.boot_from_cd = cc_false;
	state->mega_cd.hblank_address = 0xFFFF;
	state->mega_cd.delayed_dma_word = 0;
}

void ClownMDEmu_Parameters_Initialise(ClownMDEmu *clownmdemu, const ClownMDEmu_Configuration *configuration, const ClownMDEmu_Constant *constant, ClownMDEmu_State *state)
{
	clownmdemu->configuration = configuration;
	clownmdemu->constant = constant;
	clownmdemu->state = state;

	clownmdemu->m68k = &state->m68k.state;

	clownmdemu->z80.constant = &constant->z80;
	clownmdemu->z80.state = &state->z80.state;

	clownmdemu->mcd_m68k = &state->mega_cd.m68k.state;

	clownmdemu->vdp.configuration = &configuration->vdp;
	clownmdemu->vdp.constant = &constant->vdp;
	clownmdemu->vdp.state = &state->vdp;

	FM_Parameters_Initialise(&clownmdemu->fm, &configuration->fm, &constant->fm, &state->fm);

	clownmdemu->psg.configuration = &configuration->psg;
	clownmdemu->psg.constant = &constant->psg;
	clownmdemu->psg.state = &state->psg;

	clownmdemu->pcm.state = &state->mega_cd.pcm;
}

void ClownMDEmu_Iterate(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks, mcd_m68k_read_write_callbacks;
	CPUCallbackUserData cpu_callback_user_data;
	cc_u8f h_int_counter;

	const cc_u16f television_vertical_resolution = clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? 312 : 262; /* PAL and NTSC, respectively */
	const cc_u16f console_vertical_resolution = (clownmdemu->state->vdp.v30_enabled ? 30 : 28) * 8; /* 240 and 224 */
	const cc_u16f cycles_per_frame = clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_PAL) : CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MASTER_CLOCK_NTSC);
	const cc_u16f cycles_per_scanline = cycles_per_frame / television_vertical_resolution;
	const CycleMegaDrive cycles_per_frame_mega_drive = MakeCycleMegaDrive(cycles_per_frame);
	const CycleMegaCD cycles_per_frame_mega_cd = MakeCycleMegaCD(clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_PAL ? CLOWNMDEMU_DIVIDE_BY_PAL_FRAMERATE(CLOWNMDEMU_MCD_MASTER_CLOCK) : CLOWNMDEMU_DIVIDE_BY_NTSC_FRAMERATE(CLOWNMDEMU_MCD_MASTER_CLOCK));

	cpu_callback_user_data.data_and_callbacks.data = clownmdemu;
	cpu_callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;
	cpu_callback_user_data.sync.m68k.current_cycle = 0;
	/* TODO: This is awful; stop doing this. */
	cpu_callback_user_data.sync.m68k.cycle_countdown = &clownmdemu->state->m68k.cycle_countdown;
	cpu_callback_user_data.sync.z80.current_cycle = 0;
	cpu_callback_user_data.sync.z80.cycle_countdown = &clownmdemu->state->z80.cycle_countdown;
	cpu_callback_user_data.sync.mcd_m68k.current_cycle = 0;
	cpu_callback_user_data.sync.mcd_m68k.cycle_countdown = &clownmdemu->state->mega_cd.m68k.cycle_countdown;
	cpu_callback_user_data.sync.mcd_m68k_irq3.current_cycle = 0;
	cpu_callback_user_data.sync.mcd_m68k_irq3.cycle_countdown = &clownmdemu->state->mega_cd.irq.irq3_countdown;
	cpu_callback_user_data.sync.fm.current_cycle = 0;
	cpu_callback_user_data.sync.psg.current_cycle = 0;
	cpu_callback_user_data.sync.pcm.current_cycle = 0;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	m68k_read_write_callbacks.user_data = &cpu_callback_user_data;

	mcd_m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	mcd_m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	mcd_m68k_read_write_callbacks.user_data = &cpu_callback_user_data;

	/* Reload H-Int counter at the top of the screen, just like real hardware does */
	h_int_counter = clownmdemu->state->vdp.h_int_interval;

	clownmdemu->state->vdp.currently_in_vblank = cc_false;

	for (clownmdemu->state->current_scanline = 0; clownmdemu->state->current_scanline < television_vertical_resolution; ++clownmdemu->state->current_scanline)
	{
		const cc_u16f scanline = clownmdemu->state->current_scanline;
		const CycleMegaDrive current_cycle = MakeCycleMegaDrive(cycles_per_scanline * (1 + scanline));

		/* Sync the 68k, since it's the one thing that can influence the VDP */
		SyncM68k(clownmdemu, &cpu_callback_user_data, current_cycle);

		/* Only render scanlines and generate H-Ints for scanlines that the console outputs to */
		if (scanline < console_vertical_resolution)
		{
			if (clownmdemu->state->vdp.double_resolution_enabled)
			{
				VDP_RenderScanline(&clownmdemu->vdp, scanline * 2, callbacks->scanline_rendered, callbacks->user_data);
				VDP_RenderScanline(&clownmdemu->vdp, scanline * 2 + 1, callbacks->scanline_rendered, callbacks->user_data);
			}
			else
			{
				VDP_RenderScanline(&clownmdemu->vdp, scanline, callbacks->scanline_rendered, callbacks->user_data);
			}

			/* Fire a H-Int if we've reached the requested line */
			if (h_int_counter-- == 0)
			{
				h_int_counter = clownmdemu->state->vdp.h_int_interval;

				/* Do H-Int */
				if (clownmdemu->state->vdp.h_int_enabled)
					Clown68000_Interrupt(clownmdemu->m68k, &m68k_read_write_callbacks, 4);
			}
		}
		else if (scanline == console_vertical_resolution) /* Check if we have reached the end of the console-output scanlines */
		{
			/* Do V-Int */
			if (clownmdemu->state->vdp.v_int_enabled)
			{
				Clown68000_Interrupt(clownmdemu->m68k, &m68k_read_write_callbacks, 6);

				SyncZ80(clownmdemu, &cpu_callback_user_data, current_cycle);
				Z80_Interrupt(&clownmdemu->z80);
			}

			/* Flag that we have entered the V-blank region */
			clownmdemu->state->vdp.currently_in_vblank = cc_true;
		}
	}

	/* Update everything for the rest of the frame. */
	SyncM68k(clownmdemu, &cpu_callback_user_data, cycles_per_frame_mega_drive);
	SyncZ80(clownmdemu, &cpu_callback_user_data, cycles_per_frame_mega_drive);
	SyncMCDM68k(clownmdemu, &cpu_callback_user_data, cycles_per_frame_mega_cd);
	SyncFM(&cpu_callback_user_data, cycles_per_frame_mega_drive);
	SyncPSG(&cpu_callback_user_data, cycles_per_frame_mega_drive);
	SyncPCM(&cpu_callback_user_data, cycles_per_frame_mega_cd);

	/* Fire IRQ1 if needed. */
	/* TODO: This is a hack. Look into when this interrupt should actually be done. */
	if (clownmdemu->state->mega_cd.irq.irq1_pending)
	{
		clownmdemu->state->mega_cd.irq.irq1_pending = cc_false;
		Clown68000_Interrupt(clownmdemu->mcd_m68k, &mcd_m68k_read_write_callbacks, 1);
	}
}

void ClownMDEmu_Reset(const ClownMDEmu *clownmdemu, const ClownMDEmu_Callbacks *callbacks, const cc_bool cd_boot)
{
	Clown68000_ReadWriteCallbacks m68k_read_write_callbacks;
	CPUCallbackUserData callback_user_data;

	clownmdemu->state->mega_cd.boot_from_cd = cd_boot;

	if (cd_boot)
	{
		/* Boot from CD ("Mode 2"). */
		const cc_u8l *sector_bytes;
		cc_u32f ip_start, ip_length, sp_start, sp_length;
		/*cc_u8l region;*/

		/* Read first sector. */
		callbacks->cd_seeked((void*)callbacks->user_data, 0);
		sector_bytes = callbacks->cd_sector_read((void*)callbacks->user_data);
		ip_start = ReadU32BE(&sector_bytes[0x30]);
		ip_length = ReadU32BE(&sector_bytes[0x34]);
		sp_start = ReadU32BE(&sector_bytes[0x40]);
		sp_length = ReadU32BE(&sector_bytes[0x44]);
		/*region = sector_bytes[0x1F0];*/

		/* Don't allow overflowing the PRG-RAM array. */
		sp_length = CC_MIN(CC_COUNT_OF(clownmdemu->state->mega_cd.prg_ram.buffer) * 2 - 0x6000, sp_length);

		/* Read Initial Program. */
		BytesTo68kRAM(clownmdemu->state->mega_cd.word_ram.buffer, &sector_bytes[0x200], 0x600);

		/* Load additional Initial Program data if necessary. */
		if (ip_start != 0x200 || ip_length != 0x600)
			CDSectorsTo68kRAM(callbacks, &clownmdemu->state->mega_cd.word_ram.buffer[0x600 / 2], 0x800, 32 * 0x800);

		/* This is what the Mega CD's BIOS does. */
		memcpy(clownmdemu->state->m68k.ram, clownmdemu->state->mega_cd.word_ram.buffer, 0x8000);

		/* Read Sub Program. */
		CDSectorsTo68kRAM(callbacks, &clownmdemu->state->mega_cd.prg_ram.buffer[0x6000 / 2], sp_start, sp_length);

		/* Give WORD-RAM to the SUB-CPU. */
		clownmdemu->state->mega_cd.word_ram.dmna = cc_true;
		clownmdemu->state->mega_cd.word_ram.ret = cc_false;
	}

	callback_user_data.data_and_callbacks.data = clownmdemu;
	callback_user_data.data_and_callbacks.frontend_callbacks = callbacks;

	m68k_read_write_callbacks.user_data = &callback_user_data;

	m68k_read_write_callbacks.read_callback = M68kReadCallback;
	m68k_read_write_callbacks.write_callback = M68kWriteCallback;
	Clown68000_Reset(clownmdemu->m68k, &m68k_read_write_callbacks);

	Z80_Reset(&clownmdemu->z80);

	m68k_read_write_callbacks.read_callback = MCDM68kReadCallback;
	m68k_read_write_callbacks.write_callback = MCDM68kWriteCallback;
	Clown68000_Reset(clownmdemu->mcd_m68k, &m68k_read_write_callbacks);
}

void ClownMDEmu_SetErrorCallback(void (*error_callback)(void *user_data, const char *format, va_list arg), const void* const user_data)
{
	SetErrorCallback(error_callback, user_data);
	Clown68000_SetErrorCallback(error_callback, user_data);
}
