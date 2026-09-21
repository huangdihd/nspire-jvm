#include "fdlibm_config.h"
#include "../vendor/openjdk8-fdlibm/upstream/e_log.c"
double nspire_fdlibm_log(double value) { return __ieee754_log(value); }
