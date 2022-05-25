#ifndef CLOWNCOMMON_H
#define CLOWNCOMMON_H

/* Boolean. */
typedef unsigned char cc_bool;
enum
{
	cc_false = 0,
	cc_true = 1
};

/* Common macros. */
#define CC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CC_CLAMP(min, max, x) (CC_MAX((min), CC_MIN((max), (x))))
#define CC_COUNT_OF(array) (sizeof(array) / sizeof(*(array)))
#define CC_DIVIDE_ROUND(a, b) (((a) + (b / 2)) / (b))
#define CC_DIVIDE_CEILING(a, b) (((a) + (b - 1)) / (b))

/* Common constants. */
#define CC_PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

#endif /* CLOWNCOMMON_H */
