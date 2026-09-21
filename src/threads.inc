/* Cooperative scheduler for the interpreter, included after object helpers.
 * A Java thread owns its C stack; switching occurs only at VM safepoints.
 * All native state containing Java references is saved and scanned by GC.
 */
#define THREAD_STACK_SIZE (256U*1024U)
#define MAX_LIVE_THREADS 32U
static void thread_entry(void *opaque);
static int thread_alive(VmThread *t) {return t&&!t->finished&&t->state!=T_NEW&&t->state!=T_DONE;}
static int workers_alive(VM *v) {
    for(VmThread *t=v->threads;t;t=t->next)if(t!=v->main_thread&&!t->daemon&&thread_alive(t))return 1;
    return 0;
}
static void save_thread(VM *v,VmThread *t) {
    t->frame=v->frame;t->depth=v->depth;t->exception=v->exception;t->nr=v->nr;
    memcpy(t->roots,v->roots,v->nr*sizeof(Object *));
}
static void restore_thread(VM *v,VmThread *t) {
    v->current=t;v->frame=t->frame;v->depth=t->depth;v->exception=t->exception;v->nr=t->nr;
    memcpy(v->roots,t->roots,t->nr*sizeof(Object *));
}
static void abort_vm(VM *v) {
    /* Never longjmp across fiber stacks: return to the saved main C stack first. */
    if(v->current&&v->main_thread&&v->current!=v->main_thread) {
        VmThread *old=v->current;v->fatal=1;save_thread(v,old);
        restore_thread(v,v->main_thread);
        vm_context_switch(&old->context,&v->main_thread->context);
        abort();
    }
    longjmp(v->abort,1);
}
static void schedule(VM *v) {
    VmThread *old=v->current;
    for(;;) {
        if(v->opt.cancelled&&v->opt.cancelled())fail(v,"cancelled by user");
        uint64_t now=vm_millis();int timed=0;
        for(VmThread *t=v->threads;t;t=t->next) {
            if(t!=old&&t->state==T_DONE&&t->stack) {release(v,t->stack);t->stack=NULL;}
            switch(t->state) {
            case T_MONITOR:if(!t->waiting->monitor_owner)t->state=T_RUN;break;
            case T_CLASS:if(t->waiting_class->init!=1)t->state=T_RUN;break;
            case T_DRAIN:if(!workers_alive(v))t->state=T_RUN;break;
            case T_JOIN:if(!thread_alive(t->joining)){t->state=T_RUN;break;}
                /* fall through */
            case T_SLEEP:case T_WAIT:case T_PARK:
                if(t->interrupted||(t->deadline&&now>=t->deadline))t->state=T_RUN;
                else if(t->deadline)timed=1;
                break;
            default:break;
            }
        }
        VmThread *next=old->next?old->next:v->threads,*start=next;
        do {
            if(next->state==T_RUN) {
                if(next==old)return;
                save_thread(v,old);restore_thread(v,next);
                vm_context_switch(&old->context,&next->context);
                if(v->fatal)longjmp(v->abort,1);
                return;
            }
            next=next->next?next->next:v->threads;
        }while(next!=start);
        if(!timed)fail(v,"thread deadlock: no runnable threads or timed wakeups");
        vm_idle_millis(1);
    }
}
static VmThread *thread_record(VM *v,Object *o) {
    if(o->thread)return o->thread;
    VmThread *t=(VmThread *)alloc(v,sizeof *t);t->vm=v;t->object=o;t->id=++v->next_thread_id;
    t->daemon=v->current?v->current->daemon:0;t->state=T_NEW;t->next=v->threads;v->threads=t;o->thread=t;
    return t;
}
static int monitor_enter(VM *v,Object *o) {
    if(!o){throwing(v,"java/lang/NullPointerException");return 0;}
    VmThread *t=v->current;t->waiting=o;
    while(o->monitor_owner&&o->monitor_owner!=t){t->state=T_MONITOR;schedule(v);}
    t->waiting=NULL;
    if(o->monitor_depth==UINT_MAX)fail(v,"monitor recursion overflow");
    o->monitor_owner=t;o->monitor_depth++;return 1;
}
static void monitor_exit(VM *v,Object *o) {
    if(!o){throwing(v,"java/lang/NullPointerException");return;}
    if(o->monitor_owner!=v->current){throwing(v,"java/lang/IllegalMonitorStateException");return;}
    if(!--o->monitor_depth)o->monitor_owner=NULL;
}
static uint64_t timeout_deadline(VM *v,int64_t millis,int nanos,int indefinite) {
    if(millis<0||nanos<0||nanos>999999){throwing(v,"java/lang/IllegalArgumentException");return 0;}
    if(indefinite&&!millis&&!nanos)return 0;
    uint64_t now=vm_millis(),delay=(uint64_t)millis+(nanos?1:0);
    return delay>UINT64_MAX-now?UINT64_MAX:now+delay;
}
static int consume_interrupt(VM *v) {
    if(!v->current->interrupted)return 0;
    v->current->interrupted=0;throwing(v,"java/lang/InterruptedException");return 1;
}
static void object_wait(VM *v,Object *o,int64_t millis,int nanos) {
    uint64_t deadline=timeout_deadline(v,millis,nanos,1);if(v->exception)return;
    VmThread *t=v->current;
    if(o->monitor_owner!=t){throwing(v,"java/lang/IllegalMonitorStateException");return;}
    if(consume_interrupt(v))return;
    unsigned depth=o->monitor_depth;o->monitor_owner=NULL;o->monitor_depth=0;
    t->waiting=o;t->deadline=deadline;t->state=T_WAIT;schedule(v);
    /* wait must reacquire the full recursion count before throwing interruption. */
    monitor_enter(v,o);o->monitor_depth=depth;t->deadline=0;consume_interrupt(v);
}
static void object_notify(VM *v,Object *o,int all) {
    if(o->monitor_owner!=v->current){throwing(v,"java/lang/IllegalMonitorStateException");return;}
    for(VmThread *t=v->threads;t;t=t->next)if(t->state==T_WAIT&&t->waiting==o){t->state=T_RUN;if(!all)break;}
}
static void thread_start(VM *v,Object *o) {
    VmThread *t=thread_record(v,o);
    if(t->state!=T_NEW){throwing(v,"java/lang/IllegalThreadStateException");return;}
    if(v->live_threads>=MAX_LIVE_THREADS)fail(v,"limit of %u live Java threads",MAX_LIVE_THREADS);
    t->stack=alloc(v,THREAD_STACK_SIZE);
    if(vm_context_init(&t->context,t->stack,THREAD_STACK_SIZE,thread_entry,t))fail(v,"cannot create thread context");
    t->state=T_RUN;v->live_threads++;
}
static void thread_sleep(VM *v,int64_t millis,int nanos) {
    uint64_t deadline=timeout_deadline(v,millis,nanos,0);if(v->exception||consume_interrupt(v))return;
    v->current->deadline=deadline;v->current->state=T_SLEEP;schedule(v);
    v->current->deadline=0;consume_interrupt(v);
}
static void thread_join(VM *v,Object *o,int64_t millis,int nanos) {
    uint64_t deadline=timeout_deadline(v,millis,nanos,1);if(v->exception)return;
    VmThread *t=v->current,*target=thread_record(v,o);
    if(!thread_alive(target))return;
    if(consume_interrupt(v))return;
    t->joining=target;t->deadline=deadline;t->state=T_JOIN;schedule(v);
    t->joining=NULL;t->deadline=0;consume_interrupt(v);
}
