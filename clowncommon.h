#ifndef CLOWNCOMMON_H
#define CLOWNCOMMON_H

/* Integer types. */
#ifndef CC_INTEGERS_DEFINED
#define CC_INTEGERS_DEFINED

#if defined(CC_USE_C99_INTEGERS)
 /* Use C99's/C++11's better integer types if available. */
#include <inttypes.h>
#include <stdint.h>

typedef int_least8_t cc_s8l;
typedef int_least16_t cc_s16l;
typedef int_least32_t cc_s32l;

typedef uint_least8_t cc_u8l;
typedef uint_least16_t cc_u16l;
typedef uint_least32_t cc_u32l;

typedef int_fast8_t cc_s8f;
typedef int_fast16_t cc_s16f;
typedef int_fast32_t cc_s32f;

typedef uint_fast8_t cc_u8f;
typedef uint_fast16_t cc_u16f;
typedef uint_fast32_t cc_u32f;

#define CC_PRIdLEAST8 PRIdLEAST8
#define CC_PRIiLEAST8 PRIiLEAST8
#define CC_PRIuLEAST8 PRIuLEAST8
#define CC_PRIoLEAST8 PRIoLEAST8
#define CC_PRIxLEAST8 PRIxLEAST8
#define CC_PRIXLEAST8 PRIXLEAST8

#define CC_PRIdLEAST16 PRIdLEAST16
#define CC_PRIiLEAST16 PRIiLEAST16
#define CC_PRIuLEAST16 PRIuLEAST16
#define CC_PRIoLEAST16 PRIoLEAST16
#define CC_PRIxLEAST16 PRIxLEAST16
#define CC_PRIXLEAST16 PRIXLEAST16

#define CC_PRIdLEAST32 PRIdLEAST32
#define CC_PRIiLEAST32 PRIiLEAST32
#define CC_PRIuLEAST32 PRIuLEAST32
#define CC_PRIoLEAST32 PRIoLEAST32
#define CC_PRIxLEAST32 PRIxLEAST32
#define CC_PRIXLEAST32 PRIXLEAST32

#define CC_PRIdFAST8 PRIdFAST8
#define CC_PRIiFAST8 PRIiFAST8
#define CC_PRIuFAST8 PRIuFAST8
#define CC_PRIoFAST8 PRIoFAST8
#define CC_PRIxFAST8 PRIxFAST8
#define CC_PRIXFAST8 PRIXFAST8

#define CC_PRIdFAST16 PRIdFAST16
#define CC_PRIiFAST16 PRIiFAST16
#define CC_PRIuFAST16 PRIuFAST16
#define CC_PRIoFAST16 PRIoFAST16
#define CC_PRIxFAST16 PRIxFAST16
#define CC_PRIXFAST16 PRIXFAST16

#define CC_PRIdFAST32 PRIdFAST32
#define CC_PRIiFAST32 PRIiFAST32
#define CC_PRIuFAST32 PRIuFAST32
#define CC_PRIoFAST32 PRIoFAST32
#define CC_PRIxFAST32 PRIxFAST32
#define CC_PRIXFAST32 PRIXFAST32
#else
 /* Fall back on C89's/C++98's dumb types. */
typedef signed char cc_s8l;
typedef signed short cc_s16l;
typedef signed long cc_s32l;

typedef unsigned char cc_u8l;
typedef unsigned short cc_u16l;
typedef unsigned long cc_u32l;

typedef signed int cc_s8f;
typedef signed int cc_s16f;
typedef signed long cc_s32f;

typedef unsigned int cc_u8f;
typedef unsigned int cc_u16f;
typedef unsigned long cc_u32f;

#define CC_PRIdLEAST8 "d"
#define CC_PRIiLEAST8 "i"
#define CC_PRIuLEAST8 "u"
#define CC_PRIoLEAST8 "o"
#define CC_PRIxLEAST8 "x"
#define CC_PRIXLEAST8 "X"

#define CC_PRIdLEAST16 "d"
#define CC_PRIiLEAST16 "i"
#define CC_PRIuLEAST16 "u"
#define CC_PRIoLEAST16 "o"
#define CC_PRIxLEAST16 "x"
#define CC_PRIXLEAST16 "X"

#define CC_PRIdLEAST32 "ld"
#define CC_PRIiLEAST32 "li"
#define CC_PRIuLEAST32 "lu"
#define CC_PRIoLEAST32 "lo"
#define CC_PRIxLEAST32 "lx"
#define CC_PRIXLEAST32 "lX"

#define CC_PRIdFAST8 "d"
#define CC_PRIiFAST8 "i"
#define CC_PRIuFAST8 "u"
#define CC_PRIoFAST8 "o"
#define CC_PRIxFAST8 "x"
#define CC_PRIXFAST8 "X"

#define CC_PRIdFAST16 "d"
#define CC_PRIiFAST16 "i"
#define CC_PRIuFAST16 "u"
#define CC_PRIoFAST16 "o"
#define CC_PRIxFAST16 "x"
#define CC_PRIXFAST16 "X"

#define CC_PRIdFAST32 "ld"
#define CC_PRIiFAST32 "li"
#define CC_PRIuFAST32 "lu"
#define CC_PRIoFAST32 "lo"
#define CC_PRIxFAST32 "lx"
#define CC_PRIXFAST32 "lX"
#endif

/* Boolean. */
typedef cc_u8l cc_bool;
enum
{
	cc_false = 0,
	cc_true = 1
};

#endif

/* Common macros. */
#define CC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CC_CLAMP(min, max, x) (CC_MAX((min), CC_MIN((max), (x))))
#define CC_COUNT_OF(array) (sizeof(array) / sizeof(*(array)))
#define CC_DIVIDE_ROUND(a, b) (((a) + (b / 2)) / (b))
#define CC_DIVIDE_CEILING(a, b) (((a) + (b - 1)) / (b))
#define CC_SIGN_EXTEND(type, bit_index, value) (((value) & (((type)1u << (bit_index)) - (type)1u)) - ((value) & ((type)1u << (bit_index))))
#define CC_SIGN_EXTEND_UINT(bit_index, value) CC_SIGN_EXTEND(unsigned int, bit_index, value)
#define CC_SIGN_EXTEND_ULONG(bit_index, value) CC_SIGN_EXTEND(unsigned long, bit_index, value)

/* GCC 3.2 is the earliest version of GCC of which I can find proof of supporting this. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
 #define CC_ATTRIBUTE_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
 #define CC_ATTRIBUTE_PRINTF(a, b)
#endif

/* Common constants. */
#define CC_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

#endif /* CLOWNCOMMON_H */
