#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _TINSPIRE
#include <libndls.h>
static int cancelled(void) { return isKeyPressed(KEY_NSPIRE_ESC); }
#endif
int main(int argc, char **argv) {
    VmOptions o = {"demo.jar.tns", "Demo", 8 * 1024 * 1024, 100000000ULL, NULL, NULL};
    int first = 1, result;
#ifdef _TINSPIRE
    enable_relative_paths(argv);
    o.cancelled = cancelled;
    /* Adjacent text config: one classpath and one main class, each on a line. */
    char cp[512], mainname[256], bootcp[512];
    FILE *f = fopen("jvm.cfg.tns", "rb");
    if (f) {
        if (fgets(cp, sizeof cp, f) && fgets(mainname, sizeof mainname, f)) {
            cp[strcspn(cp, "\r\n")] = 0;
            mainname[strcspn(mainname, "\r\n")] = 0;
            o.classpath = cp; o.main_class = mainname;
            if(fgets(bootcp,sizeof bootcp,f)) {
                bootcp[strcspn(bootcp,"\r\n")]=0;
                if(*bootcp)o.bootclasspath=bootcp;
            }
        }
        fclose(f);
    }
    puts("Nspire JVM 0.1 - experimental\nESC: stop execution\n");
#else
    while (first < argc && argv[first][0] == '-') {
        const char *opt = argv[first++];
        if (!strcmp(opt, "--version")) { puts("Nspire JVM 0.1 (experimental subset; not Java SE)"); return 0; }
        if (first == argc) { fprintf(stderr, "Missing value for %s\n", opt); return 2; }
        const char *v = argv[first++];
        if (!strcmp(opt, "-cp")) o.classpath = v;
        else if (!strcmp(opt, "-bootclasspath")) o.bootclasspath = v;
        else if (!strcmp(opt, "--heap")) o.heap_limit = (size_t)strtoull(v, NULL, 10);
        else if (!strcmp(opt, "--steps")) o.instruction_limit = strtoull(v, NULL, 10);
        else { fprintf(stderr, "Unknown option: %s\n", opt); return 2; }
    }
    if (first >= argc) {
        fputs("Usage: nspire-jvm [-bootclasspath runtime.jar] -cp 'app.jar;lib.jar' [--heap bytes] [--steps count] Main [args...]\n", stderr);
        return 2;
    }
    o.main_class = argv[first++];
#endif
    result = vm_run(&o, argc - first, (const char **)(argv + first));
#ifdef _TINSPIRE
    printf("\nVM exit: %d. Press a key.\n", result);
    wait_no_key_pressed(); wait_key_pressed();
#endif
    return result;
}
