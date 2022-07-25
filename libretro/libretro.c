#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "libretro.h"

#include "../clownmdemu.h"

#define FRAMEBUFFER_WIDTH 320
#define FRAMEBUFFER_HEIGHT 480

/* Prevent errors when '__attribute__((format(printf, X, X)))' is not supported. */
/* GCC 3.2 is the earliest version of GCC of which I can find proof of supporting this. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#define ATTRIBUTE_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define ATTRIBUTE_PRINTF(a, b)
#endif

static ClownMDEmu_Configuration clownmdemu_configuration;
static ClownMDEmu_Constant clownmdemu_constant;
static ClownMDEmu_State clownmdemu_state;
static ClownMDEmu clownmdemu = {&clownmdemu_configuration, &clownmdemu_constant, &clownmdemu_state};

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

struct
{
	retro_environment_t        environment;
	retro_video_refresh_t      video;
	retro_audio_sample_t       audio;
	retro_audio_sample_batch_t audio_batch;
	retro_input_poll_t         input_poll;
	retro_input_state_t        input_state;
	ATTRIBUTE_PRINTF(2, 3) retro_log_printf_t log;
} libretro_callbacks;

static unsigned int CartridgeReadCallback(void *user_data, unsigned long address)
{
	(void)user_data;

	if (address >= rom_size)
		return 0;
	else
		return rom[address];
}

static void CartridgeWriteCallback(void *user_data, unsigned long address, unsigned int value)
{
	(void)user_data;
	(void)address;
	(void)value;
}

static void ColourUpdatedCallback_0RGB1555(void *user_data, unsigned int index, unsigned int colour)
{
	(void)user_data;

	/* Convert from 0BGR4444 to 0RGB1555. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	colours.u16[index] = (((red   << 1) | (red   >> 3)) << (5 * 2))
	                   | (((green << 1) | (green >> 3)) << (5 * 1))
	                   | (((blue  << 1) | (blue  >> 3)) << (5 * 0));
}

static void ColourUpdatedCallback_RGB565(void *user_data, unsigned int index, unsigned int colour)
{
	(void)user_data;

	/* Convert from 0BGR4444 to RGB565. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	colours.u16[index] = (((red   << 1) | (red   >> 3)) << 11)
	                   | (((green << 2) | (green >> 2)) << 5)
	                   | (((blue  << 1) | (blue  >> 3)) << 0);
}

static void ColourUpdatedCallback_XRGB8888(void *user_data, unsigned int index, unsigned int colour)
{
	(void)user_data;

	/* Convert from 0BGR4444 to XRGB8888. */
	const unsigned int red   = (colour >> 0) & 0xF;
	const unsigned int green = (colour >> 4) & 0xF;
	const unsigned int blue  = (colour >> 8) & 0xF;

	colours.u32[index] = (((red   << 4) | (red   >> 0)) << (8 * 2))
	                   | (((green << 4) | (green >> 0)) << (8 * 1))
	                   | (((blue  << 4) | (blue  >> 0)) << (8 * 0));
}

static void ScanlineRenderedCallback_16Bit(void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height)
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

static void ScanlineRenderedCallback_32Bit(void *user_data, unsigned int scanline, const unsigned char *pixels, unsigned int screen_width, unsigned int screen_height)
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

static cc_bool InputRequestedCallback(void *user_data, unsigned int player_id, ClownMDEmu_Button button_id)
{
	cc_bool button_state = cc_false;

	if (player_id == 0)
	{
		unsigned int libretro_button_id;

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

		button_state = libretro_callbacks.input_state(0, RETRO_DEVICE_JOYPAD, 0, libretro_button_id);
	}

	return button_state;
}

static void FMAudioToBeGeneratedCallback(void *user_data, size_t total_frames, void (*generate_fm_audio)(ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_frames))
{
	(void)user_data;

	short sample_buffer[0x100][2];
	size_t frames_remaining;

	frames_remaining = total_frames;

	while (frames_remaining != 0)
	{
		const size_t frames_to_do = CC_MIN(frames_remaining, CC_COUNT_OF(sample_buffer));

		memset(sample_buffer, 0, sizeof(sample_buffer));

		generate_fm_audio(&clownmdemu, &sample_buffer[0][0], frames_to_do);

		libretro_callbacks.audio_batch(&sample_buffer[0][0], frames_to_do);

		frames_remaining -= frames_to_do;
	}
}

static void PSGAudioToBeGeneratedCallback(void *user_data, size_t total_samples, void (*generate_psg_audio)(ClownMDEmu *clownmdemu, short *sample_buffer, size_t total_samples))
{
	(void)user_data;

	short dummy_buffer[0x100];
	size_t frames_remaining;

	frames_remaining = total_samples;

	while (frames_remaining != 0)
	{
		const size_t frames_to_do = CC_MIN(frames_remaining, CC_COUNT_OF(dummy_buffer));

		generate_psg_audio(&clownmdemu, &dummy_buffer[0], frames_to_do);

		frames_remaining -= frames_to_do;
	}
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

ATTRIBUTE_PRINTF(2, 3) static void FallbackErrorLogCallback(enum retro_log_level level, const char *format, ...)
{
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

	va_list args;
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

void retro_init(void)
{
	uint64_t serialisation_quirks = RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT | RETRO_SERIALIZATION_QUIRK_PLATFORM_DEPENDENT;
	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &serialisation_quirks);

	/* Emulate a Genesis. */
	clownmdemu_configuration.general.region = CLOWNMDEMU_REGION_OVERSEAS;
	clownmdemu_configuration.general.tv_standard = CLOWNMDEMU_TV_STANDARD_NTSC;

	ClownMDEmu_SetErrorCallback(ClownMDEmuErrorLog);

	ClownMDEmu_Constant_Initialise(&clownmdemu_constant);
	ClownMDEmu_State_Initialise(&clownmdemu_state);
}

void retro_deinit(void)
{
}

unsigned int retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	libretro_callbacks.log(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name     = "clownmdemu";
	info->library_version  = "0.1";
	info->need_fullpath    = false;
	info->valid_extensions = ".bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->geometry.base_width   = 320;
	info->geometry.base_height  = 224;
	info->geometry.max_width    = FRAMEBUFFER_WIDTH;
	info->geometry.max_height   = FRAMEBUFFER_HEIGHT;
	info->geometry.aspect_ratio = 320.0f / 224.0f;

	info->timing.fps            = 60.0 / 1.001;	/* Standard NTSC framerate. */
	info->timing.sample_rate    = CLOWNMDEMU_FM_SAMPLE_RATE_NTSC;
}

void retro_set_environment(retro_environment_t environment_callback)
{
	libretro_callbacks.environment = environment_callback;

	struct retro_log_callback logging;
	if (libretro_callbacks.environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
		libretro_callbacks.log = logging.log;
	else
		libretro_callbacks.log = FallbackErrorLogCallback;

	static const struct retro_controller_description controllers[] = {
		{ "Nintendo DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
	};

	static const struct retro_controller_info ports[] = {
		{ controllers, 1 },
		{ NULL, 0 },
	};

	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
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

static void check_variables(void)
{
}

void retro_run(void)
{
	void *current_framebuffer;
	size_t current_framebuffer_pitch;

	libretro_callbacks.input_poll();

	if (clownmdemu_callbacks.scanline_rendered == ScanlineRenderedCallback_16Bit)
	{
		current_framebuffer = &framebuffer.u16;
		current_framebuffer_pitch = sizeof(framebuffer.u16[0]);
	}
	else
	{
		current_framebuffer = &framebuffer.u32;
		current_framebuffer_pitch = sizeof(framebuffer.u32[0]);
	}

	ClownMDEmu_Iterate(&clownmdemu, &clownmdemu_callbacks);

	libretro_callbacks.video(current_framebuffer, current_screen_width, current_screen_height, current_framebuffer_pitch);

	bool updated = false;
	if (libretro_callbacks.environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
		check_variables();
}

bool retro_load_game(const struct retro_game_info *info)
{
	struct retro_input_descriptor desc[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0 },
	};

	libretro_callbacks.environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

	enum retro_pixel_format pixel_format;

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

	check_variables();

	rom = (const unsigned char*)info->data;
	rom_size = info->size;

	ClownMDEmu_Reset(&clownmdemu, &clownmdemu_callbacks);

	return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	return false;
}

size_t retro_serialize_size(void)
{
	return sizeof(clownmdemu_state);
}

bool retro_serialize(void *data_, size_t size)
{
	memcpy(data_, &clownmdemu_state, sizeof(clownmdemu_state));
	return true;
}

bool retro_unserialize(const void *data_, size_t size)
{
	memcpy(&clownmdemu_state, data_, sizeof(clownmdemu_state));
	return true;
}

void *retro_get_memory_data(unsigned id)
{
	(void)id;
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	(void)id;
	return 0;
}

void retro_cheat_reset(void)
{
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}

