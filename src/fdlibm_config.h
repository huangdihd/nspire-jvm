/* Original bridge. Included only by fdlibm translation units. */
#include <stdint.h>
#include <float.h>
#include "strictmath.h"
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _LITTLE_ENDIAN
#elif __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error Unsupported double word order
#endif
#if defined(__FLOAT_WORD_ORDER__) && __FLOAT_WORD_ORDER__ != __BYTE_ORDER__
#error Mixed-endian doubles are unsupported
#endif
typedef char fdlibm_needs_int32[(sizeof(int)==4)?1:-1];
typedef char fdlibm_needs_binary64[(sizeof(double)==8 && DBL_MANT_DIG==53 && DBL_MAX_EXP==1024)?1:-1];
