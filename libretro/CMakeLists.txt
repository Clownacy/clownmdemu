cmake_minimum_required(VERSION 3.18)

project(clownmdemu_libretro LANGUAGES C)

# Sources.
add_library(clownmdemu_libretro
	"libretro.c"
)

# Only require C90.
set_target_properties(clownmdemu_libretro PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

# Add a definition from the Makefile.
target_compile_definitions(clownmdemu_libretro PRIVATE __LIBRETRO__)

# Link clownmdemu core.
add_subdirectory("../" EXCLUDE_FROM_ALL "clownmdemu")
target_link_libraries(clownmdemu_libretro clownmdemu)
