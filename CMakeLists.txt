cmake_minimum_required(VERSION 3.18)

option(CLOWNMDEMU_INSANE_INTEGERS "Use insane integetr types to test code that code is actually sane" OFF)

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
	"m68k/opcode.c"
	"m68k/opcode.h"
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

if(CLOWNMDEMU_INSANE_INTEGERS)
	target_compile_definitions(clownmdemu PUBLIC
		CC_INTEGERS_DEFINED

		cc_s8l=signed\ long\ long
		cc_s16l=signed\ long\ long
		cc_s32l=signed\ long\ long

		cc_u8l=unsigned\ long\ long
		cc_u16l=unsigned\ long\ long
		cc_u32l=unsigned\ long\ long

		cc_s8f=signed\ long\ long
		cc_s16f=signed\ long\ long
		cc_s32f=signed\ long\ long

		cc_u8f=unsigned\ long\ long
		cc_u16f=unsigned\ long\ long
		cc_u32f=unsigned\ long\ long

		CC_PRIdLEAST8=\"%%lld\"
		CC_PRIiLEAST8=\"%%lli\"
		CC_PRIuLEAST8=\"%%llu\"
		CC_PRIoLEAST8=\"%%llo\"
		CC_PRIxLEAST8=\"%%llx\"
		CC_PRIXLEAST8=\"%%llX\"

		CC_PRIdLEAST16=\"%%lld\"
		CC_PRIiLEAST16=\"%%lli\"
		CC_PRIuLEAST16=\"%%llu\"
		CC_PRIoLEAST16=\"%%llo\"
		CC_PRIxLEAST16=\"%%llx\"
		CC_PRIXLEAST16=\"%%llX\"

		CC_PRIdLEAST32=\"%%lld\"
		CC_PRIiLEAST32=\"%%lli\"
		CC_PRIuLEAST32=\"%%llu\"
		CC_PRIoLEAST32=\"%%llo\"
		CC_PRIxLEAST32=\"%%llx\"
		CC_PRIXLEAST32=\"%%llX\"

		CC_PRIdFAST8=\"%%lld\"
		CC_PRIiFAST8=\"%%lli\"
		CC_PRIuFAST8=\"%%llu\"
		CC_PRIoFAST8=\"%%llo\"
		CC_PRIxFAST8=\"%%llx\"
		CC_PRIXFAST8=\"%%llX\"

		CC_PRIdFAST16=\"%%lld\"
		CC_PRIiFAST16=\"%%lli\"
		CC_PRIuFAST16=\"%%llu\"
		CC_PRIoFAST16=\"%%llo\"
		CC_PRIxFAST16=\"%%llx\"
		CC_PRIXFAST16=\"%%llX\"

		CC_PRIdFAST32=\"%%lld\"
		CC_PRIiFAST32=\"%%lli\"
		CC_PRIuFAST32=\"%%llu\"
		CC_PRIoFAST32=\"%%llo\"
		CC_PRIxFAST32=\"%%llx\"
		CC_PRIXFAST32=\"%%llX\"

		cc_bool=cc_u8l
		cc_false=0
		cc_true=1
	)
endif()
