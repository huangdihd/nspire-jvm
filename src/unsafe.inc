/* VM-managed field handles and atomic operations for the OpenJDK class library.
 * Offsets are opaque VM values, never arbitrary native addresses. The scheduler
 * cannot switch inside these native operations, so CAS is atomic across all
 * Java threads supported by this interpreter.
 */
static uint64_t field_offset(VM *v,Class *owner,Field *f) {
    for(int i=0;i<v->nclasses;i++)if(v->classes[i]==owner)
        return ((uint64_t)(i+1)<<32)+(unsigned)(f-owner->fields)+1;
    fail(v,"invalid field owner");return 0;
}
static Value *unsafe_slot(VM *v,Object *o,uint64_t offset,int tag) {
    if(!o){throwing(v,"java/lang/NullPointerException");return NULL;}
    Value *slot;
    if(o->kind=='a') {
        if(offset<16||(offset-16)%8||(offset-16)/8>=o->count)fail(v,"Unsafe array offset outside object");
        slot=&o->data[(offset-16)/8];
    } else {
        uint64_t ci=offset>>32,fi=offset&UINT32_MAX;
        if(!ci||ci>(unsigned)v->nclasses||!fi)fail(v,"invalid Unsafe field offset");
        Class *owner=v->classes[ci-1];
        if(fi>owner->nf)fail(v,"invalid Unsafe field index");
        Field *f=&owner->fields[fi-1];
        if(f->flags&STATIC) {
            if(o->kind!='c'||o->represented!=owner)fail(v,"invalid Unsafe static field base");
            slot=&f->value;
        } else {
            if(!subtype(o->cls,owner)||f->slot>=o->count)fail(v,"invalid Unsafe field receiver");
            slot=&o->data[f->slot];
        }
    }
    if(slot->tag!=tag)fail(v,"Unsafe field/array type mismatch");
    return slot;
}
static Value unsafe_call(VM *v,const char *n,const char *d,Value *a,int stat) {
    Value none=iv(0);
    if(stat&&!strcmp(n,"getUnsafe")&&!strcmp(d,"()Lsun/misc/Unsafe;")) {
        if(!v->unsafe_instance)v->unsafe_instance=new_object(v,load(v,"sun/misc/Unsafe"),'o',0);
        return rv(v->unsafe_instance);
    }
    if(stat)goto missing;
    if((!strcmp(n,"objectFieldOffset")||!strcmp(n,"staticFieldOffset"))&&!strcmp(d,"(Ljava/lang/reflect/Field;)J")) {
        Object *o=nonnull(v,a[1]);if(!o)return none;
        if(!o->represented_field)fail(v,"Unsafe expected reflected field");
        if(!!(o->represented_field->flags&STATIC)!=!strcmp(n,"staticFieldOffset")){throwing(v,"java/lang/IllegalArgumentException");return none;}
        return val(field_offset(v,o->represented,o->represented_field),LONG);
    }
    if(!strcmp(n,"staticFieldBase")&&!strcmp(d,"(Ljava/lang/reflect/Field;)Ljava/lang/Object;")) {
        Object *o=nonnull(v,a[1]);if(!o)return none;
        if(!o->represented_field||!(o->represented_field->flags&STATIC)){throwing(v,"java/lang/IllegalArgumentException");return none;}
        return rv(class_mirror(v,o->represented));
    }
    if((!strcmp(n,"arrayBaseOffset")||!strcmp(n,"arrayIndexScale"))&&!strcmp(d,"(Ljava/lang/Class;)I")) {
        Object *o=nonnull(v,a[1]);if(!o)return none;
        if(o->kind!='c'||!o->represented->component){throwing(v,"java/lang/IllegalArgumentException");return none;}
        return iv(!strcmp(n,"arrayBaseOffset")?16:8);
    }
    if(!strcmp(n,"ensureClassInitialized")&&!strcmp(d,"(Ljava/lang/Class;)V")) {
        Object *o=nonnull(v,a[1]);if(o){if(o->kind!='c')fail(v,"Class expected");initialize(v,o->represented);}return none;
    }
    const char *types[]={"Int","Long","Object"};const char *codes[]={"I","J","Ljava/lang/Object;"};
    const int tags[]={INT,LONG,REF};
    for(unsigned i=0;i<3;i++) {
        char name[64],desc[128];
        snprintf(name,sizeof name,"compareAndSwap%s",types[i]);
        snprintf(desc,sizeof desc,"(Ljava/lang/Object;J%s%s)Z",codes[i],codes[i]);
        if(!strcmp(n,name)&&!strcmp(d,desc)) {
            Value *slot=unsafe_slot(v,obj(a[1]),a[2].bits,tags[i]);if(!slot)return none;
            if(slot->bits!=a[3].bits)return iv(0);*slot=a[4];return iv(1);
        }
        for(unsigned mode=0;mode<2;mode++) {
            if(mode==0&&tags[i]==REF)continue;
            snprintf(name,sizeof name,mode==0?"getAndAdd%s":"getAndSet%s",types[i]);
            snprintf(desc,sizeof desc,"(Ljava/lang/Object;J%s)%s",codes[i],codes[i]);
            if(!strcmp(n,name)&&!strcmp(d,desc)) {
                Value *slot=unsafe_slot(v,obj(a[1]),a[2].bits,tags[i]);if(!slot)return none;
                Value old=*slot;
                if(mode==1)*slot=a[3];
                else if(tags[i]==INT)*slot=iv((int32_t)((uint32_t)old.bits+(uint32_t)a[3].bits));
                else *slot=val(old.bits+a[3].bits,LONG);
                return old;
            }
        }
        for(unsigned mode=0;mode<5;mode++) {
            int get=mode<2;
            snprintf(name,sizeof name,mode==0?"get%s":mode==1?"get%sVolatile":mode==2?"put%s":mode==3?"put%sVolatile":"putOrdered%s",types[i]);
            if(get)snprintf(desc,sizeof desc,"(Ljava/lang/Object;J)%s",codes[i]);
            else snprintf(desc,sizeof desc,"(Ljava/lang/Object;J%s)V",codes[i]);
            if(!strcmp(n,name)&&!strcmp(d,desc)) {
                Value *slot=unsafe_slot(v,obj(a[1]),a[2].bits,tags[i]);if(!slot)return none;
                if(get)return *slot;*slot=a[3];return none;
            }
        }
    }
    if(!strcmp(n,"unpark")&&!strcmp(d,"(Ljava/lang/Object;)V")) {
        Object *o=obj(a[1]);if(!o)return none;
        if(!subtype(o->cls,load(v,"java/lang/Thread")))fail(v,"Unsafe.unpark expects Thread");
        VmThread *t=thread_record(v,o);t->permit=1;if(t->state==T_PARK)t->state=T_RUN;return none;
    }
    if(!strcmp(n,"park")&&!strcmp(d,"(ZJ)V")) {
        VmThread *t=v->current;int64_t timeout=(int64_t)a[2].bits;
        if(t->permit){t->permit=0;return none;}
        if(t->interrupted||timeout<0)return none;
        uint64_t delay=0;
        if(integer(a[1])) {
            uint64_t epoch=(uint64_t)time(NULL)*1000;
            if((uint64_t)timeout<=epoch)return none;
            delay=(uint64_t)timeout-epoch;
        } else if(timeout)delay=(uint64_t)timeout/1000000+((uint64_t)timeout%1000000!=0);
        uint64_t now=vm_millis();t->deadline=delay?(delay>UINT64_MAX-now?UINT64_MAX:now+delay):0;
        t->state=T_PARK;schedule(v);t->permit=0;t->deadline=0;return none;
    }
missing:
    fail(v,"runtime method not implemented: sun/misc/Unsafe.%s%s",n,d);return none;
}
