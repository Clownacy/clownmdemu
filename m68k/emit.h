#ifndef M68K_EMIT_H
#define M68K_EMIT_H

#include <stdio.h>

#include "../clowncommon/clowncommon.h"

extern FILE *emit_file;
extern unsigned int emit_indentation;

void Emit(const char* const line);
CC_ATTRIBUTE_PRINTF(1, 2) void EmitFormatted(const char* const line, ...);

#endif /* M68K_EMIT_H */
