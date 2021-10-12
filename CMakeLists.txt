cmake_minimum_required(VERSION 3.20)

project(clownmdemu LANGUAGES C)

add_library(clownmdemu STATIC
	"clownmdemu.c"
	"clownmdemu.h"
	"clowncommon.h"
	"error.c"
	"error.h"
	"m68k.c"
	"m68k.h"
	"vdp.c"
	"vdp.h"
)

set_target_properties(clownmdemu PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF

	CXX_STANDARD 98
	CXX_STANDARD_REQUIRED NO
	CXX_EXTENSIONS OFF
)
