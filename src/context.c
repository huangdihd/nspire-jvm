#include "context.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef _TINSPIRE
#include <libndls.h>
#include <sys/time.h>
extern void vm_arm_switch(VmContext *,VmContext *);
extern void vm_arm_entry(void);
int vm_context_init(VmContext *c,void *stack,size_t size,void (*entry)(void *),void *arg) {
    memset(c,0,sizeof *c);
    c->saved[0]=(uintptr_t)arg; c->saved[1]=(uintptr_t)entry;
    c->saved[8]=((uintptr_t)stack+size)&~(uintptr_t)7;
    c->saved[9]=(uintptr_t)vm_arm_entry;
    return 0;
}
void vm_context_switch(VmContext *from,VmContext *to) { vm_arm_switch(from,to); }
uint64_t vm_millis(void) {
    struct timeval tv; gettimeofday(&tv,NULL);
    return (uint64_t)tv.tv_sec*1000+(unsigned long)tv.tv_usec/1000;
}
void vm_idle_millis(unsigned ms) { msleep(ms); }
#else
/* Optional ASan fiber hooks, so supported sanitizer builds understand stacks. */
extern void __sanitizer_start_switch_fiber(void **,const void *,size_t) __attribute__((weak));
extern void __sanitizer_finish_switch_fiber(void *,const void **,size_t *) __attribute__((weak));
static void finish_switch(VmContext *c) {
    if(__sanitizer_finish_switch_fiber) {
        const void *bottom=NULL;size_t size=0;
        __sanitizer_finish_switch_fiber(c->asan_fake,&bottom,&size);
        if(c->from&&!c->from->bottom){c->from->bottom=bottom;c->from->size=size;}
    }
}
static void entry_bridge(unsigned low,unsigned high) {
    uintptr_t bits=low;
#if UINTPTR_MAX > UINT32_MAX
    bits|=(uintptr_t)high<<32;
#else
    (void)high;
#endif
    VmContext *c=(VmContext *)bits;finish_switch(c);c->entry(c->arg);abort();
}
int vm_context_init(VmContext *c,void *stack,size_t size,void (*entry)(void *),void *arg) {
    memset(c,0,sizeof *c);if(getcontext(&c->state))return -1;
    c->entry=entry;c->arg=arg;c->bottom=stack;c->size=size;
    c->state.uc_stack.ss_sp=stack;c->state.uc_stack.ss_size=size;c->state.uc_link=NULL;
    uintptr_t bits=(uintptr_t)c;unsigned high=0;
#if UINTPTR_MAX > UINT32_MAX
    high=(unsigned)(bits>>32);
#endif
    makecontext(&c->state,(void (*)(void))entry_bridge,2,(unsigned)bits,high);return 0;
}
void vm_context_switch(VmContext *from,VmContext *to) {
    to->from=from;
    if(__sanitizer_start_switch_fiber)__sanitizer_start_switch_fiber(&from->asan_fake,to->bottom,to->size);
    if(swapcontext(&from->state,&to->state))abort();finish_switch(from);
}
uint64_t vm_millis(void) {struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000+(unsigned long)ts.tv_nsec/1000000;}
void vm_idle_millis(unsigned ms) {struct timespec ts={ms/1000,(long)(ms%1000)*1000000};nanosleep(&ts,NULL);}
#endif
