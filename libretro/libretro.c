#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "libretro.h"
#include "libretro_core_options.h"

#include "../clownmdemu.h"

#include "../frontend/mixer.h"

#define FRAMEBUFFER_WIDTH 320
#define FRAMEBUFFER_HEIGHT 480

#define SAMPLE_RATE_NO_LOWPASS 48000
#define SAMPLE_RATE_WITH_LOWPASS 22000 /* This sounds about right compared to my PAL Model 1 Mega Drive. */

/* Mixer data. */
static Mixer_Constant mixer_constant;
static Mixer_State mixer_state;
static const Mixer mixer = {&mixer_constant, &mixer_state};

/* clownmdemu data. */
static ClownMDEmu_Configuration clownmdemu_configuration;
static ClownMDEmu_Constant clownmdemu_constant;
static ClownMDEmu_State clownmdemu_state;
static const ClownMDEmu clownmdemu = {&clownmdemu_configuration, &clownmdemu_constant, &clownmdemu_state};

/* Frontend data. */
static union
{
	uint16_t u16[FRAMEBUFFER_HEIGHT][FRAMEBUFFER_WIDTH];
	uint32_t u32[FRAMEBUFFER_HEIGHT][FRAMEBUFFER_WIDTH];
} framebuffer;

static union
{
	uint16_t u16[16 * 4 * 3]; /* 16 colours, 4 palette lines, 3 brightnesses. */
	uint32_t u32[16 * 4 * 3];
} colours;

static unsigned int current_screen_width;
static unsigned int current_screen_height;
static const unsigned char *rom;
static size_t rom_size;
static cc_bool pal_mode_enabled;
static cc_bool tall_interlace_mode_2;
static cc_bool lowpass_filter_enabled;

static struct
{
	retro_environment_t        environment;
	retro_video_refresh_t      video;
	retro_audio_sample_t       audio;
	retro_audio_sample_batch_t audio_batch;
	retro_input_poll_t         input_poll;
	retro_input_state_t        input_state;
	CC_ATTRIBUTE_PRINTF(2, 3) retro_log_printf_t log;
} libretro_callbacks;

static unsigned int CartridgeReadCallback(const void *user_data, unsigned long address)
{
	(void)user_data;

	if (address >= rom_size)
		return 0;
	else
		return rom[address];
}

static void CartridgeWriteCallback(const void *user_data, unsigned long address, unsigned int value)
{
	(void)user_data;
	(void)address;
	(void)value;
}

static void ColourUpdatedCallback_0RGB1555(const void *user_data, unsigned int index, unsigned int colour)
{
	/* Convert from 0BGR4444 to 0RGB1555. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	(void)user_data;

	colours.u16[index] = (((red   << 1) | (red   >> 3)) << (5 * 2))
	                   | (((green << 1) | (green >> 3)) << (5 * 1))
	                   | (((blue  << 1) | (blue  >> 3)) << (5 * 0));
}

static void ColourUpdatedCallback_RGB565(const void *user_data, unsigned int index, unsigned int colour)
{
	/* Convert from 0BGR4444 to RGB565. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	(void)user_data;

	colours.u16[index] = (((red   << 1) | (red   >> 3)) << 11)
	                   | (((green << 2) | (green >> 2)) << 5)
	                   | (((blue  << 1) | (blue  >> 3)) << 0);
}

static void ColourUpdatedCallback_XRGB8888(const void *user_data, unsigned int index, unsigned int colour)
{
	/* Convert from 0BGR4444 to XRGB8888. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	(void)user_data;

	colours.u32[index] = (((red   << 4) | (red   >> 0)) << (8 * 2))
	                   | (((green << 4) | (green >> 0)) << (8 * 1))
	                   | (((blue  << 4) | (blue  >> 0)) << (8 * 0));
}

static void ScanlineRenderedCallback_16Bit(const void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height)
{
	const unsigned char *source_pixel_pointer = pixels;
	uint16_t *destination_pixel_pointer = framebuffer.u16[scanline];

	unsigned int i;

	(void)user_data;

	for (i = 0; i < screen_width; ++i)
		*destination_pixel_pointer++ = colours.u16[*source_pixel_pointer++];

	current_screen_width = screen_width;
	current_screen_height = screen_height;
}

static void ScanlineRenderedCallback_32Bit(const void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height)
{
	const unsigned char *source_pixel_pointer = pixels;
	uint32_t *destination_pixel_pointer = framebuffer.u32[scanline];

	unsigned int i;

	(void)user_data;

	for (i = 0; i < screen_width; ++i)
		*destination_pixel_pointer++ = colours.u32[*source_pixel_pointer++];

	current_screen_width = screen_width;
	current_screen_height = screen_height;
}

static cc_bool InputRequestedCallback(const void *user_data, unsigned int player_id, ClownMDEmu_Button button_id)
{
	unsigned int libretro_button_id;

	(void)user_data;

	switch (button_id)
	{
		default:
			/* Fallthrough */
		case CLOWNMDEMU_BUTTON_UP:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_UP;
			break;

		case CLOWNMDEMU_BUTTON_DOWN:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_DOWN;
			break;

		case CLOWNMDEMU_BUTTON_LEFT:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_LEFT;
			break;

		case CLOWNMDEMU_BUTTON_RIGHT:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_RIGHT;
			break;

		case CLOWNMDEMU_BUTTON_A:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_Y;
			break;

		case CLOWNMDEMU_BUTTON_B:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_B;
			break;

		case CLOWNMDEMU_BUTTON_C:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_A;
			break;

		case CLOWNMDEMU_BUTTON_START:
			libretro_button_id = RETRO_DEVICE_ID_JOYPAD_START;
			break;
	}

	return libretro_callbacks.input_state(player_id, RETRO_DEVICE_JOYPAD, 0, libretro_button_id);
}

static void FMAudioToBeGeneratedCallback(const void *user_data, size_t total_frames, void (*generate_fm_audio)(const ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_frames))
{
	(void)user_data;

	generate_fm_audio(&clownmdemu, Mixer_AllocateFMSamples(&mixer, total_frames), total_frames);
}

static void PSGAudioToBeGeneratedCallback(const void *user_data, size_t total_samples, void (*generate_psg_audio)(const ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_samples))
{
	(void)user_data;

	generate_psg_audio(&clownmdemu, Mixer_AllocatePSGSamples(&mixer, total_samples), total_samples);
}

static ClownMDEmu_Callbacks clownmdemu_callbacks = {
	NULL,
	CartridgeReadCallback,
	CartridgeWriteCallback,
	ColourUpdatedCallback_0RGB1555,
	ScanlineRenderedCallback_16Bit,
	InputRequestedCallback,
	FMAudioToBeGeneratedCallback,
	PSGAudioToBeGeneratedCallback
};

CC_ATTRIBUTE_PRINTF(2, 3) static void FallbackErrorLogCallback(enum retro_log_level level, const char *format, ...)
{
	va_list args;

	switch (level)
	{
		case RETRO_LOG_DEBUG:
			fputs("RETRO_LOG_DEBUG: ", stderr);
			break;

		case RETRO_LOG_INFO:
			fputs("RETRO_LOG_INFO: ", stderr);
			break;

		case RETRO_LOG_WARN:
			fputs("RETRO_LOG_WARN: ", stderr);
			break;

		case RETRO_LOG_ERROR:
			fputs("RETRO_LOG_ERROR: ", stderr);
			break;

		case RETRO_LOG_DUMMY:
			break;
	}

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

static void ClownMDEmuErrorLog(const char *format, va_list arg)
{
	/* libretro lacks an error log callback that takes a va_list for some dumbass reason,
	   so we'll have to expand the message to a plain string here. */
	char message_buffer[0x100];

	/* TODO: This unbounded printf is so gross... */
	vsprintf(message_buffer, format, arg);
	strcat(message_buffer, "\n");

	libretro_callbacks.log(RETRO_LOG_WARN, message_buffer);
}

static cc_bool DoOptionBoolean(const char *key)
{
	struct retro_variable variable;
	variable.key = key;
	return libretro_callbacks.environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) && variable.value != NULL && strcmp(variable.value, "enabled") == 0;
}

static void UpdateOptions(cc_bool only_update_flags)
{
	const cc_bool lowpass_filter_changed = lowpass_filter_enabled != DoOptionBoolean("lowpass_filter");
	const cc_bool pal_mode_changed = pal_mode_enabled != DoOptionBoolean("pal_mode");

	lowpass_filter_enabled ^= lowpass_filter_changed;
	pal_mode_enabled ^= pal_mode_changed;

	if ((lowpass_filter_changed || pal_mode_changed) && !only_update_flags)
	{
		Mixer_State_Initialise(&mixer_state, lowpass_filter_enabled ? SAMPLE_RATE_WITH_LOWPASS : SAMPLE_RATE_NO_LOWPASS, pal_mode_enabled, cc_false);

		{
		struct retro_system_av_info info;
		retro_get_system_av_info(&info);
		libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
		}
	}

	tall_interlace_mode_2 = DoOptionBoolean("tall_interlace_mode_2");

	clownmdemu_configuration.general.region = DoOptionBoolean("overseas_region") ? CLOWNMDEMU_REGION_OVERSEAS : CLOWNMDEMU_REGION_DOMESTIC;
	clownmdemu_configuration.general.tv_standard = pal_mode_enabled ? CLOWNMDEMU_TV_STANDARD_PAL : CLOWNMDEMU_TV_STANDARD_NTSC;
	clownmdemu_configuration.vdp.planes_disabled[0] = DoOptionBoolean("disable_plane_a");
	clownmdemu_configuration.vdp.planes_disabled[1] = DoOptionBoolean("disable_plane_b");
	clownmdemu_configuration.vdp.sprites_disabled = DoOptionBoolean("disable_sprite_plane");
}

void retro_init(void)
{
	/* Inform frontend of serialisation quirks. */
	{
	uint64_t serialisation_quirks = RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT | RETRO_SERIALIZATION_QUIRK_PLATFORM_DEPENDENT;
	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &serialisation_quirks);
	}

	/* Initialise clownmdemu. */
	UpdateOptions(cc_true);

	ClownMDEmu_SetErrorCallback(ClownMDEmuErrorLog);

	ClownMDEmu_Constant_Initialise(&clownmdemu_constant);
	ClownMDEmu_State_Initialise(&clownmdemu_state);

	/* Initialise the mixer. */
	Mixer_Constant_Initialise(&mixer_constant);
	Mixer_State_Initialise(&mixer_state, lowpass_filter_enabled ? SAMPLE_RATE_WITH_LOWPASS : SAMPLE_RATE_NO_LOWPASS, pal_mode_enabled, cc_false);
}


void retro_deinit(void)
{
}

unsigned int retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned int port, unsigned int device)
{
	(void)port;
	(void)device;

	/* TODO */
	/*libretro_callbacks.log(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);*/
}

void retro_get_system_info(struct retro_system_info *info)
{
	info->library_name     = "clownmdemu";
	info->library_version  = "v0.1";
	info->need_fullpath    = false;
	info->valid_extensions = "bin|md|gen";
	info->block_extract    = false;
}

static void SetGeometry(struct retro_game_geometry *geometry)
{
	geometry->base_width   = 256;
	geometry->base_height  = 224;
	geometry->max_width    = FRAMEBUFFER_WIDTH;
	geometry->max_height   = FRAMEBUFFER_HEIGHT;
	geometry->aspect_ratio = 320.0f / (float)current_screen_height;

	/* Squish the aspect ratio vertically when in Interlace Mode 2. */
	if (!tall_interlace_mode_2 && current_screen_height >= 448)
		geometry->aspect_ratio *= 2.0f;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	/* Initialise this to avoid a division by 0 in SetGeometry. */
	current_screen_height = 224;

	SetGeometry(&info->geometry);

	info->timing.fps = pal_mode_enabled ? 50.0 : 60.0 / 1.001;	/* Standard PAL and NTSC framerates. */
	info->timing.sample_rate = lowpass_filter_enabled ? (double)SAMPLE_RATE_WITH_LOWPASS : (double)SAMPLE_RATE_NO_LOWPASS;
}

void retro_set_environment(retro_environment_t environment_callback)
{
	libretro_callbacks.environment = environment_callback;

	/* Declare the options to the frontend. */
	libretro_set_core_options(libretro_callbacks.environment);

	/* Retrieve a log callback from the frontend. */
	{
	struct retro_log_callback logging;
	if (libretro_callbacks.environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging) && logging.log != NULL)
		libretro_callbacks.log = logging.log;
	else
		libretro_callbacks.log = FallbackErrorLogCallback;
	}

	/* TODO: Specialised controller types. */
	{
	/*static const struct retro_controller_description controllers[] = {
		{"Control Pad", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)},
	};

	static const struct retro_controller_info ports[] = {
		{controllers, CC_COUNT_OF(controllers)},
		{NULL, 0}
	};

	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);*/
	}

	/* Give the buttons proper names. */
	{
	const struct retro_input_descriptor desc[] = {
		/* Player 1. */
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up"    },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down"  },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left"  },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "A"     },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B"     },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "C"     },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		/* Player 2. */
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up"    },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down"  },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left"  },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "A"     },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "B"     },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "C"     },
		{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		/* End. */
		{ 0, 0, 0, 0, NULL }
	};

	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void*)&desc);
	}
}

void retro_set_audio_sample(retro_audio_sample_t audio_callback)
{
	libretro_callbacks.audio = audio_callback;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t audio_batch_callback)
{
	libretro_callbacks.audio_batch = audio_batch_callback;
}

void retro_set_input_poll(retro_input_poll_t input_poll_callback)
{
	libretro_callbacks.input_poll = input_poll_callback;
}

void retro_set_input_state(retro_input_state_t input_state_callback)
{
	libretro_callbacks.input_state = input_state_callback;
}

void retro_set_video_refresh(retro_video_refresh_t video_callback)
{
	libretro_callbacks.video = video_callback;
}

void retro_reset(void)
{
	ClownMDEmu_Reset(&clownmdemu, &clownmdemu_callbacks);
}

static void MixerCompleteCallback(const void *user_data, short *audio_samples, size_t total_frames)
{
	(void)user_data;

	libretro_callbacks.audio_batch(audio_samples, total_frames);
}

void retro_run(void)
{
	bool options_updated;

	/* Refresh options if they've been updated. */
	if (libretro_callbacks.environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) && options_updated)
		UpdateOptions(cc_false);

	/* Poll inputs. */
	libretro_callbacks.input_poll();

	Mixer_Begin(&mixer);

	ClownMDEmu_Iterate(&clownmdemu, &clownmdemu_callbacks);

	Mixer_End(&mixer, MixerCompleteCallback, NULL);

	/* Update aspect ratio. */
	{
	struct retro_game_geometry geometry;
	SetGeometry(&geometry);
	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
	}

	/* Upload the completed frame to the frontend. */
	if (clownmdemu_callbacks.scanline_rendered == ScanlineRenderedCallback_16Bit)
		libretro_callbacks.video(&framebuffer.u16, current_screen_width, current_screen_height, sizeof(framebuffer.u16[0]));
	else
		libretro_callbacks.video(&framebuffer.u32, current_screen_width, current_screen_height, sizeof(framebuffer.u32[0]));
}

bool retro_load_game(const struct retro_game_info *info)
{
	enum retro_pixel_format pixel_format;

	/* Determine which pixel format to render as. */
	pixel_format = RETRO_PIXEL_FORMAT_RGB565;
	if (libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format))
	{
		clownmdemu_callbacks.colour_updated = ColourUpdatedCallback_RGB565;
		clownmdemu_callbacks.scanline_rendered = ScanlineRenderedCallback_16Bit;
	}
	else
	{
		pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
		if (libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format))
		{
			clownmdemu_callbacks.colour_updated = ColourUpdatedCallback_XRGB8888;
			clownmdemu_callbacks.scanline_rendered = ScanlineRenderedCallback_32Bit;
		}
		else
		{
			clownmdemu_callbacks.colour_updated = ColourUpdatedCallback_0RGB1555;
			clownmdemu_callbacks.scanline_rendered = ScanlineRenderedCallback_16Bit;
		}
	}

	/* Initialise the ROM. */
	rom = (const unsigned char*)info->data;
	rom_size = info->size;

	/* Boot the emulated Mega Drive. */
	ClownMDEmu_Reset(&clownmdemu, &clownmdemu_callbacks);

	return true;
}

void retro_unload_game(void)
{
	/* Nothing to do here... */
}

unsigned int retro_get_region(void)
{
	return pal_mode_enabled ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned int type, const struct retro_game_info *info, size_t num)
{
	(void)type;
	(void)info;
	(void)num;

	/* We don't need anything special. */
	return false;
}

size_t retro_serialize_size(void)
{
	return sizeof(clownmdemu_state);
}

bool retro_serialize(void *data, size_t size)
{
	(void)size;

	memcpy(data, &clownmdemu_state, sizeof(clownmdemu_state));
	return true;
}

bool retro_unserialize(const void *data, size_t size)
{
	(void)size;

	memcpy(&clownmdemu_state, data, sizeof(clownmdemu_state));
	return true;
}

void* retro_get_memory_data(unsigned int id)
{
	(void)id;

	return NULL;
}

size_t retro_get_memory_size(unsigned int id)
{
	(void)id;

	return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned int index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}
