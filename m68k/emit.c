#include "emit.h"

#include <stdarg.h>
#include <stdio.h>

#include "../clowncommon.h"

FILE *emit_file;
unsigned int emit_indentation;

void Emit(const char* const line)
{
	EmitFormatted("%s", line);
}

CC_ATTRIBUTE_PRINTF(1, 2) void EmitFormatted(const char* const line, ...)
{
	unsigned int i;
	va_list args;

	for (i = 0; i < emit_indentation; ++i)
		fputc('\t', emit_file);

	va_start(args, line);
	vfprintf(emit_file, line, args);
	va_end(args);

	fputc('\n', emit_file);
}
