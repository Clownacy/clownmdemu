cmake_minimum_required(VERSION 3.18)

option(CC_USE_C99_INTEGERS "Use C99 integer types instead of the original C89 ones. May save RAM depending on the platform's data model." OFF)

project(clownmdemu LANGUAGES C)

add_library(clownmdemu STATIC
	"clownmdemu.c"
	"clownmdemu.h"
	"clowncommon/clowncommon.h"
	"controller.c"
	"controller.h"
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
	"io_port.c"
	"io_port.h"
	"mcd_pcm.c"
	"mcd_pcm.h"
	"psg.c"
	"psg.h"
	"vdp.c"
	"vdp.h"
	"z80.c"
	"z80.h"
)

set_target_properties(clownmdemu PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

add_subdirectory("clown68000" EXCLUDE_FROM_ALL)

target_link_libraries(clownmdemu PRIVATE clown68000-interpreter)

if(CC_USE_C99_INTEGERS)
	target_compile_definitions(clownmdemu PUBLIC CC_USE_C99_INTEGERS)
endif()
