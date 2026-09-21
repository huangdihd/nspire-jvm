/* Run the original OpenJDK shutdown state machine on real interpreter threads.
 * halt0 returns to vm_run's caller, including the Ndless UI; it must never exit
 * the embedding process or longjmp directly across a fiber stack. */
static Value shutdown_call(VM *v,const char *owner,const char *name,const char *desc,Value *args,unsigned count) {
    Class *c=load(v,owner);initialize(v,c);if(v->exception)return iv(0);
    Method *m=method(c,name,desc);
    if(!m||!(m->flags&STATIC))fail(v,"shutdown entry missing: %s.%s%s",owner,name,desc);
    return execute(v,m,args,count);
}
static Value shutdown_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int stat,int *handled) {
    const char *owner=c->name;*handled=1;
    if(stat&&!strcmp(owner,"java/lang/Shutdown")) {
        if(!strcmp(name,"beforeHalt")&&!strcmp(desc,"()V")) {
            /* This interpreter has no profiler/OS shutdown notification. Make
             * its C console output visible before running the shutdown code. */
            fflush(stdout);fflush(stderr);return iv(0);
        }
        if(!strcmp(name,"halt0")&&!strcmp(desc,"(I)V")) {
            v->status=integer(args[0]);abort_vm(v);
        }
    }
    if(!stat&&(!strcmp(owner,"sun/misc/JavaLangAccess")||!strcmp(owner,"nspire/JavaLangAccess"))&&
       !strcmp(name,"registerShutdownHook")&&!strcmp(desc,"(IZLjava/lang/Runnable;)V"))
        return shutdown_call(v,"java/lang/Shutdown","add",desc,args+1,3);
    if(!stat&&!strcmp(owner,"java/lang/Runtime")) {
        if(!strcmp(name,"addShutdownHook")&&!strcmp(desc,"(Ljava/lang/Thread;)V"))
            return shutdown_call(v,"java/lang/ApplicationShutdownHooks","add",desc,args+1,1);
        if(!strcmp(name,"removeShutdownHook")&&!strcmp(desc,"(Ljava/lang/Thread;)Z"))
            return shutdown_call(v,"java/lang/ApplicationShutdownHooks","remove",desc,args+1,1);
        if(!strcmp(name,"exit")&&!strcmp(desc,"(I)V"))
            return shutdown_call(v,"java/lang/Shutdown","exit",desc,args+1,1);
        if(!strcmp(name,"halt")&&!strcmp(desc,"(I)V")) {
            shutdown_call(v,"java/lang/Shutdown","beforeHalt","()V",NULL,0);
            if(!v->exception)return shutdown_call(v,"java/lang/Shutdown","halt",desc,args+1,1);
            return iv(0);
        }
    }
    if(stat&&!strcmp(owner,"java/lang/System")&&!strcmp(name,"exit")&&!strcmp(desc,"(I)V"))
        return shutdown_call(v,"java/lang/Shutdown","exit",desc,args,1);
    *handled=0;return iv(0);
}
static void shutdown_natural(VM *v) {
    /* With no registered/loaded shutdown machinery there is no Java work to
     * run. This also retains the standalone VM's no-bootclasspath mode. */
    int loaded=0;
    for(int i=0;i<v->nclasses;i++)if(!strcmp(v->classes[i]->name,"java/lang/Shutdown")){loaded=1;break;}
    if(!loaded)return;
    /* main has terminated. Give the C root stack its own Java thread identity
     * so hooks can join the original main without seeing it come alive again.
     * Transfer the host context metadata too, including ASan fiber state. */
    VmThread *old=v->main_thread;Class *tc=load(v,"java/lang/Thread");
    Object *object=new_object(v,tc,'o',tc->slots);
    for(unsigned i=0;i<tc->nf;i++)object->data[i]=zero(tc->fields[i].desc);
    VmThread *controller=thread_record(v,object);set_text(v,object,"DestroyJavaVM");
    controller->context=old->context;controller->state=T_RUN;controller->daemon=0;
    old->state=T_DONE;v->current=v->main_thread=controller;v->live_threads++;
    shutdown_call(v,"java/lang/Shutdown","shutdown","()V",NULL,0);
    if(v->exception){fprintf(stderr,"Uncaught shutdown exception: %s\n",v->exception->cls->name);v->exception=NULL;}
}
