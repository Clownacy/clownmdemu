#ifndef CLOWNCOMMON_H
#define CLOWNCOMMON_H

/* Boolean */
typedef unsigned char cc_bool;
enum
{
	cc_false = 0,
	cc_true = 1
};

/* Common macros */
#define CC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CC_CLAMP(x, min, max) (CC_MIN((max), CC_MAX((min), (x))))
#define CC_COUNT_OF(array) (sizeof(array) / sizeof(*(array)))
#define CC_DIVIDE_ROUND(a, b) (((a) + (b / 2)) / (b))
#define CC_DIVIDE_CEILING(a, b) (((a) + (b - 1)) / (b))

#endif /* CLOWNCOMMON_H */
