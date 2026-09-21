/* Platform boundary for the preserved OpenJDK Unix canonicalizer. */
#ifndef NSPIRE_CANONICAL_H
#define NSPIRE_CANONICAL_H
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
int canonicalize(char *original, char *resolved, int len);
#endif
