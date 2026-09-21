#ifndef NJVM_CONTEXT_H
#define NJVM_CONTEXT_H
#include <stddef.h>
#include <stdint.h>
#ifdef _TINSPIRE
typedef struct { uintptr_t saved[10]; } VmContext;
#else
#include <ucontext.h>
typedef struct VmContext {
    ucontext_t state;
    void (*entry)(void *); void *arg;
    const void *bottom; size_t size; void *asan_fake;
    struct VmContext *from;
} VmContext;
#endif
int vm_context_init(VmContext *,void *,size_t,void (*)(void *),void *);
void vm_context_switch(VmContext *,VmContext *);
uint64_t vm_millis(void);
void vm_idle_millis(unsigned);
#endif
