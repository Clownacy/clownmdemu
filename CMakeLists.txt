cmake_minimum_required(VERSION 3.18)

project(clownmdemu LANGUAGES C)

add_library(clownmdemu STATIC
	"clownmdemu.c"
	"clownmdemu.h"
	"clowncommon.h"
	"error.c"
	"error.h"
	"fm.c"
	"fm.h"
	"fm_channel.c"
	"fm_channel.h"
	"fm_envelope.c"
	"fm_envelope.h"
	"fm_operator.c"
	"fm_operator.h"
	"fm_phase.c"
	"fm_phase.h"
	"m68k.c"
	"m68k.h"
	"psg.c"
	"psg.h"
	"vdp.c"
	"vdp.h"
)

set_target_properties(clownmdemu PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)
