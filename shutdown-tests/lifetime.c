/* System.exit/halt must stop this VM, not this embedding process. */
#include "vm.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int descriptors(void) {
    DIR *d=opendir("/proc/self/fd");if(!d)return -1;
    int count=0;struct dirent *e;
    while((e=readdir(d)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,".."))count++;
    closedir(d);return count;
}
int main(int argc,char **argv) {
    if(argc!=3)return 2;
    VmOptions options={0};options.classpath=argv[1];options.bootclasspath=argv[2];
    options.main_class="ShutdownTest";options.heap_limit=1024*1024;options.instruction_limit=10000000;
    const char *modes[]={"exit","halt","worker-exit","worker-halt","natural","uncaught","halt-hook","fatal-hook","fatal-worker","late-delete"};
    int statuses[]={7,23,9,29,0,1,31,1,1,0};int initial=descriptors();
    if(initial<0)return 3;
    for(int pass=0;pass<2;pass++)for(unsigned i=0;i<sizeof statuses/sizeof *statuses;i++) {
        int actual=vm_run(&options,1,&modes[i]);
        if(actual!=statuses[i]||descriptors()!=initial){fprintf(stderr,"lifetime failed: %s status %d\n",modes[i],actual);return 4;}
        /* Only fixed names created in the driver's disposable cwd. */
        unlink("delete-me");unlink("parent/child");rmdir("parent");
        unlink("hook-1");unlink("hook-2");unlink("worker-finished");unlink("halt-start");
    }
    puts("PASS twenty embedded VM shutdowns with preserved process and descriptor count");return 0;
}
