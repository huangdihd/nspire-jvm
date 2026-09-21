#ifndef NSPIRE_VM_H
#define NSPIRE_VM_H
#include <stddef.h>
/* Experimental JVM subset with cooperative threads. Not a Java SE runtime. */
typedef struct {
    const char *classpath; /* Semicolon-separated JARs/directories, all platforms. */
    const char *main_class;
    size_t heap_limit;
    unsigned long long instruction_limit; /* zero = no limit */
    int (*cancelled)(void);
    const char *bootclasspath; /* Supplemental runtime classes, searched first. */
} VmOptions;
int vm_run(const VmOptions *options, int argc, const char **argv);
#endif
