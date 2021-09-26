cmake_minimum_required(VERSION 3.21.2)

project(clownmdemu LANGUAGES C)

add_library(clownmdemu
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

	PUBLIC_HEADER "clownmdemu.h"
)

target_include_directories(clownmdemu PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
