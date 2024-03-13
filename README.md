# Overview

This is clownmdemu, a Sega Mega Drive (a.k.a. Sega Genesis) emulator.

It is currently in the very early stages of development: it can run some games,
but many standard features of the Mega Drive are unemulated (see `TODO.md` for
more information).


# Frontends

To actually play games with clownmdemu, you'll need to use a frontend.
Currently there are two official frontends:
- The standalone frontend, which includes a variety of debugging menus:
  https://github.com/Clownacy/clownmdemu-frontend
- The libretro frontend, for use with libretro implementations such as
  RetroArch:
  https://github.com/Clownacy/clownmdemu-libretro


# Design

clownmdemu's code adheres to the following principles, emphasising minimalism
and portability:

- Use C89. This is required in order to support as many compilers as possible.
  Ideally, the code should be valid C++ as well, in order to support both C and
  C++ compilers.

- Use integer-only logic. The Sega Mega Drive/Sega Genesis had no support for
  floating point types. Therefore, emulating it should not require floating
  point types. Additionally, floating point types are prone to rounding error
  and other precision issues, so I do not trust them. I am also concerned about
  their performance compared to integers, especially on lower-end hardware.

- Do not use dynamic memory. Memory allocation is slow, will cause memory leaks
  when not correctly freed, and can cause software to fail mid-execution when
  memory is exhausted, which is more effort to account for than it is worth.

- Use no global state. All state should be kept in a struct which functions
  access through a pointer, allowing for such things as simple fast save-state
  support as well as the possibility of running multiple instances of the
  emulator at once. Note that the state must not contain pointers, so that it
  is relocatable and position-independent. Likewise, the state must not depend
  on outside state, so that it is useable across multiple executions of the
  emulator.

- Operate within the guarantees of the C standard: no undefined behaviour and
  no implementation-defined behaviour.

  - Do not depend on integer type sizes. 'char', 'short', 'int', and 'long' may
    be different sizes on different platforms, so do not rely on their ability
    to hold (or not hold) values larger than their standard capacities:
    - -127 to 127 for 'signed char'.
    - 0 to 255 for 'unsigned char'.
    - -32767 to 32767 for 'short' and 'int'.
    - 0 to 65535 for 'unsigned short' and 'unsigned int'.
    - -2147483647 to 2147483647 for 'long'.
    - 0 to 4294967295 for 'unsigned long'.

  - Do not assume that 'char' is always signed by default: it is not. For
    instance, it is unsigned by default on ARM CPUs.

  - Do not rely on C language extensions.

  - Do not rely on endianness. Code should work correctly on both little-endian
    and big-endian CPUs.

  - Do not rely on signed number representation. That is to say, do not assume
    that negative numbers are represented in binary as two's complement.

- Use original code. No emulation components are taken from other emulators or
  standalone libraries. For example, the rather than use something like
  [Musashi](https://github.com/kstenerud/Musashi), the 68000 emulation core is
  custom. Likewise, a custom YM2612 emulation core is used instead of
  [Nuked-OPN2](https://github.com/nukeykt/Nuked-OPN2).

clownmdemu itself is implemented as a library, with all platform-specific logic
being relegated to a separate frontend program.

clownmdemu attempts to balance correctness with performance, acting as a more
high-level emulator than accuracy-focussed alternatives may.

clownmdemu exposes a relatively low-level interface: audio from the FM and PSG
are output separately at their native sample rates, and video is output a
single scanline at a time in its native indexed format. This is to give the
frontend the most flexibility in how it can process the data for delivery to
the user. For instance, if the platform can play multiple separate audio
streams at once, then the frontend can skip the expensive steps of audio
resampling and mixing.


# Compiling

clownmdemu can be built using CMake, however it should not be hard to make it
use a different build system if necessary as the emulator's build process is
not complicated.

Be aware that this repo uses Git submodules: use `git submodule update --init`
to pull in these submodules before compiling.


# Licence

clownmdemu is free software, licensed under the AGPLv3 (or any later version).
See `LICENCE.txt` for more information.
