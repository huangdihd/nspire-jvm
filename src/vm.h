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
    const char *timezone; /* Explicit region/offset; target defaults to UTC. */
    const char *temp_directory; /* java.io.tmpdir; host /tmp, target launch cwd. */
} VmOptions;
/* Returns the Java exit/halt status, 0 after normal main completion, or 1 on
 * uncaught main exceptions / VM failures. Always preserves the C caller. */
int vm_run(const VmOptions *options, int argc, const char **argv);
#endif
