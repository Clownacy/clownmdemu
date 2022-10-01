cmake_minimum_required(VERSION 3.18)

project(generator LANGUAGES C)

add_executable(generator
	"emit.c"
	"emit.h"
	"emit-instructions.c"
	"emit-instructions.h"
	"generator.c"
)

set_target_properties(generator PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)
