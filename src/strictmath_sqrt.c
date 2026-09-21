#include "fdlibm_config.h"
#include "../vendor/openjdk8-fdlibm/generated/e_sqrt.c"
double nspire_fdlibm_sqrt(double value) { return __ieee754_sqrt(value); }
