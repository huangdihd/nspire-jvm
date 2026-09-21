/* Count live descriptors across repeated vm_run calls in one host process. */
#include "vm.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
static int descriptors(void) {
    DIR *dir=opendir("/proc/self/fd");if(!dir)return -1;
    int count=0;struct dirent *entry;
    while((entry=readdir(dir)))if(strcmp(entry->d_name,".")&&strcmp(entry->d_name,".."))count++;
    closedir(dir);return count;
}
int main(int argc,char **argv) {
    if(argc!=3&&argc!=4)return 2;
    const char *entry=argc==4?argv[3]:"FileGcTest";
    VmOptions options={0};options.classpath=argv[1];options.bootclasspath=argv[2];options.main_class=entry;
    options.heap_limit=8*1024*1024;options.instruction_limit=100000000;
    int initial=descriptors();if(initial<0)return 3;
    for(int i=0;i<6;i++){
        const char *argument="fatal";int result=vm_run(&options,i%2,&argument);
        if(result!=i%2||descriptors()!=initial){fputs("Descriptor leak or unexpected VM result\n",stderr);return 4;}
        options.main_class="ConsoleCloseTest";
        if(vm_run(&options,0,NULL)||descriptors()!=initial){fputs("Console close escaped the VM lifetime\n",stderr);return 5;}
        options.main_class=entry;
    }
    puts("PASS descriptors unchanged across six normal/fatal VM runs");return 0;
}
