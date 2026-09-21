/* LambdaMetafactory intrinsics. Generated adapters are ordinary interpreted
 * methods: captures are fields, and casts/conversions/calls use JVM bytecode. */
typedef struct { Class *args[256],*result;unsigned count; } LambdaSignature;
typedef struct { VM *vm;Class *cls;unsigned capacity;unsigned char *code;size_t length,code_capacity; } LambdaBuilder;
static LambdaSignature lambda_signature(VM *v,const char *desc) {
    LambdaSignature s;memset(&s,0,sizeof s);
    if(*desc++!='(')fail(v,"invalid lambda method descriptor");
    while(*desc&&*desc!=')') {
        if(s.count==256)fail(v,"too many lambda parameters");
        Class *c=descriptor_class(v,&desc);if(c->primitive=='V')fail(v,"void lambda parameter");s.args[s.count++]=c;
    }
    if(*desc++!=')')fail(v,"truncated lambda method descriptor");
    s.result=descriptor_class(v,&desc);if(*desc)fail(v,"invalid lambda return descriptor");return s;
}
static char *lambda_descriptor(VM *v,Class *c) {
    if(c->primitive){char d[2]={(char)c->primitive,0};return copy(v,d);}
    if(c->component)return copy(v,c->name);
    size_t n=strlen(c->name);char *d=(char *)alloc(v,n+3);d[0]='L';memcpy(d+1,c->name,n);d[n+1]=';';return d;
}
static unsigned lambda_pool(LambdaBuilder *b,unsigned tag,unsigned a,unsigned c,const char *text) {
    Class *k=b->cls;
    for(unsigned i=1;i<k->nc;i++) {
        CP *p=&k->cp[i];if(p->tag==tag&&p->a==a&&p->b==c&&((!text&&!p->text)||(text&&p->text&&!strcmp(text,p->text))))return i;
    }
    if(k->nc==65535)fail(b->vm,"lambda constant pool overflow");
    if(k->nc==b->capacity) {
        unsigned next=b->capacity?b->capacity*2:32;if(next>65535)next=65535;
        CP *pool=(CP *)alloc(b->vm,next*sizeof *pool);if(k->cp)memcpy(pool,k->cp,k->nc*sizeof *pool);
        release(b->vm,k->cp);k->cp=pool;b->capacity=next;
    }
    unsigned index=k->nc++;CP *p=&k->cp[index];p->tag=(uint8_t)tag;p->a=(uint16_t)a;p->b=(uint16_t)c;if(text)p->text=copy(b->vm,text);return index;
}
static unsigned lambda_class_constant(LambdaBuilder *b,Class *c) {
    unsigned name=lambda_pool(b,1,0,0,c->name);return lambda_pool(b,7,name,0,NULL);
}
static unsigned lambda_reference(LambdaBuilder *b,unsigned tag,Class *owner,const char *name,const char *desc) {
    unsigned cls=lambda_class_constant(b,owner),n=lambda_pool(b,1,0,0,name),d=lambda_pool(b,1,0,0,desc);
    unsigned nt=lambda_pool(b,12,n,d,NULL);return lambda_pool(b,tag,cls,nt,NULL);
}
static void lambda_byte(LambdaBuilder *b,unsigned byte) {
    if(b->length==b->code_capacity) {
        size_t next=b->code_capacity?b->code_capacity*2:128;
        if(next>65536)fail(b->vm,"lambda adapter too large");
        unsigned char *code=(unsigned char *)alloc(b->vm,next);if(b->length)memcpy(code,b->code,b->length);
        release(b->vm,b->code);b->code=code;b->code_capacity=next;
    }
    b->code[b->length++]=(unsigned char)byte;
}
static void lambda_word(LambdaBuilder *b,unsigned word){lambda_byte(b,word>>8);lambda_byte(b,word);}
static void lambda_cp_instruction(LambdaBuilder *b,unsigned op,unsigned index){lambda_byte(b,op);lambda_word(b,index);}
static void lambda_string_constant(LambdaBuilder *b,const char *text) {
    unsigned utf_index=lambda_pool(b,1,0,0,text);
    lambda_cp_instruction(b,0x13,lambda_pool(b,8,utf_index,0,NULL));
}
static void lambda_small_int(LambdaBuilder *b,unsigned value) {
    if(value>32767)fail(b->vm,"lambda integer constant too large");
    lambda_byte(b,0x11);lambda_word(b,value);
}
static unsigned lambda_slots(Class *c){return c->primitive=='J'||c->primitive=='D'?2:1;}
static unsigned lambda_load_opcode(Class *c) {
    return !c->primitive?0x19:c->primitive=='J'?0x16:c->primitive=='F'?0x17:c->primitive=='D'?0x18:0x15;
}
static void lambda_local(LambdaBuilder *b,Class *c,unsigned index) {
    if(index>255){lambda_byte(b,0xc4);lambda_byte(b,lambda_load_opcode(c));lambda_word(b,index);}
    else {lambda_byte(b,lambda_load_opcode(c));lambda_byte(b,index);}
}
static const char *lambda_wrapper(unsigned primitive) {
    switch(primitive){case 'Z':return "java/lang/Boolean";case 'B':return "java/lang/Byte";case 'C':return "java/lang/Character";case 'S':return "java/lang/Short";case 'I':return "java/lang/Integer";case 'J':return "java/lang/Long";case 'F':return "java/lang/Float";case 'D':return "java/lang/Double";default:return NULL;}
}
static const char *lambda_unbox_name(unsigned primitive) {
    switch(primitive){case 'Z':return "booleanValue";case 'B':return "byteValue";case 'C':return "charValue";case 'S':return "shortValue";case 'I':return "intValue";case 'J':return "longValue";case 'F':return "floatValue";case 'D':return "doubleValue";default:return NULL;}
}
static int lambda_widen(unsigned from,unsigned to) {
    if(from==to)return 1;
    switch(from){case 'B':return strchr("SIJFD",to)!=NULL;case 'S':case 'C':return strchr("IJFD",to)!=NULL;case 'I':return strchr("JFD",to)!=NULL;case 'J':return strchr("FD",to)!=NULL;case 'F':return to=='D';default:return 0;}
}
static void lambda_adapt(LambdaBuilder *b,Class *from,Class *to) {
    VM *v=b->vm;if(from==to)return;
    if(to->primitive=='V'){if(from->primitive!='V')lambda_byte(b,lambda_slots(from)==2?0x58:0x57);return;}
    if(from->primitive=='V')fail(v,"lambda cannot produce a value from void");
    if(from->primitive&&to->primitive) {
        unsigned f=from->primitive,t=to->primitive;
        if(!lambda_widen(f,t))fail(v,"lambda primitive conversion not permitted: %c to %c",f,t);
        unsigned op=0;
        if(strchr("BSCI",f)){if(t=='J')op=0x85;else if(t=='F')op=0x86;else if(t=='D')op=0x87;}
        else if(f=='J')op=t=='F'?0x89:0x8a;
        else if(f=='F'&&t=='D')op=0x8d;
        if(op)lambda_byte(b,op);return;
    }
    if(from->primitive) {
        Class *wrapper=load(v,lambda_wrapper(from->primitive));
        if(!subtype(wrapper,to))fail(v,"lambda boxing type mismatch: %s to %s",wrapper->name,to->name);
        char desc[96];snprintf(desc,sizeof desc,"(%c)L%s;",from->primitive,wrapper->name);
        lambda_cp_instruction(b,0xb8,lambda_reference(b,10,wrapper,"valueOf",desc));return;
    }
    if(to->primitive) {
        const char *primitive=wrapper_primitive(from->name);Class *boxed=from,*unboxed;
        if(primitive)unboxed=load(v,primitive);
        else {
            boxed=load(v,to->primitive=='Z'||to->primitive=='C'?lambda_wrapper(to->primitive):"java/lang/Number");
            if(!subtype(from,boxed))lambda_cp_instruction(b,0xc0,lambda_class_constant(b,boxed));
            unboxed=to;
        }
        char desc[4]={'(',')',(char)unboxed->primitive,0};
        lambda_cp_instruction(b,0xb6,lambda_reference(b,10,boxed,lambda_unbox_name(unboxed->primitive),desc));
        lambda_adapt(b,unboxed,to);return;
    }
    if(!subtype(from,to))lambda_cp_instruction(b,0xc0,lambda_class_constant(b,to));
}
static void lambda_specialization(VM *v,LambdaSignature *sam,LambdaSignature *dynamic) {
    if(sam->count!=dynamic->count)fail(v,"lambda SAM arity mismatch");
    for(unsigned i=0;i<sam->count;i++)if(sam->args[i]!=dynamic->args[i]&&
        (sam->args[i]->primitive||dynamic->args[i]->primitive||!subtype(dynamic->args[i],sam->args[i])))fail(v,"invalid lambda SAM specialization");
    if(sam->result!=dynamic->result&&(sam->result->primitive||dynamic->result->primitive||!subtype(dynamic->result,sam->result)))fail(v,"invalid lambda result specialization");
}
static unsigned lambda_boot_int(VM *v,Class *owner,Bootstrap *bootstrap,unsigned *index) {
    if(*index>=bootstrap->nargs)fail(v,"truncated alternate lambda arguments");
    int32_t value=(int32_t)cp(v,owner,bootstrap->args[(*index)++],3)->bits;
    if(value<0)fail(v,"negative alternate lambda argument");return (unsigned)value;
}
static Class *lambda_build(VM *v,Class *owner,CP *site,const char *factory_desc,Bootstrap *bootstrap,int alternate) {
    if(bootstrap->nargs<(alternate?4:3)||(!alternate&&bootstrap->nargs!=3))fail(v,"invalid lambda bootstrap arguments");
    const char *sam_desc=utf(v,owner,cp(v,owner,bootstrap->args[0],16)->a);
    const char *dynamic_desc=utf(v,owner,cp(v,owner,bootstrap->args[2],16)->a);
    CP *handle=cp(v,owner,bootstrap->args[1],15);
    unsigned kind=handle->a;if(kind<5||kind>9)fail(v,"lambda method-handle kind not supported: %u",kind);
    CP *reference=cp(v,owner,handle->b,0);if(reference->tag!=10&&reference->tag!=11)fail(v,"lambda requires a direct method or constructor handle");
    CP *nt=cp(v,owner,reference->b,12);Class *target=load(v,classname(v,owner,reference->a));
    const char *target_name=utf(v,owner,nt->a),*target_desc=utf(v,owner,nt->b);
    const char *sam_name=utf(v,owner,cp(v,owner,site->b,12)->a);
    LambdaSignature factory=lambda_signature(v,factory_desc),sam=lambda_signature(v,sam_desc),dynamic=lambda_signature(v,dynamic_desc),implementation=lambda_signature(v,target_desc);
    if(!(factory.result->access&0x200))fail(v,"lambda factory return type is not an interface");
    lambda_specialization(v,&sam,&dynamic);
    int receiver=kind==5||kind==7||kind==9;
    if(kind==8&&strcmp(target_name,"<init>"))fail(v,"constructor lambda handle must name <init>");
    if(kind!=8&&(!strcmp(target_name,"<init>")||!strcmp(target_name,"<clinit>")))fail(v,"invalid lambda target name");
    if(receiver) {
        if(implementation.count==256)fail(v,"lambda target arity too large");
        memmove(implementation.args+1,implementation.args,implementation.count*sizeof(Class *));implementation.args[0]=target;implementation.count++;
    }
    if(kind==8)implementation.result=target;
    if(factory.count+sam.count!=implementation.count)fail(v,"lambda capture/target arity mismatch");
    for(unsigned i=0;i<factory.count;i++)if(factory.args[i]!=implementation.args[i]&&!(i==0&&receiver&&subtype(factory.args[i],implementation.args[i])))fail(v,"lambda capture type mismatch");
    Class *interfaces[256];interfaces[0]=factory.result;unsigned ni=1;
    const char *descriptors[256];descriptors[0]=sam_desc;unsigned nm=1,index=3;int serializable=0;
    if(alternate) {
        unsigned flags=lambda_boot_int(v,owner,bootstrap,&index);
        serializable=!!(flags&1);
        if(flags&~7U)fail(v,"unknown alternate lambda flags");
        if(flags&2) {
            unsigned count=lambda_boot_int(v,owner,bootstrap,&index);if(count>255)fail(v,"too many lambda marker interfaces");
            for(unsigned i=0;i<count;i++) {
                if(index==bootstrap->nargs)fail(v,"missing lambda marker interface");
                Class *marker=load(v,utf(v,owner,cp(v,owner,bootstrap->args[index++],7)->a));
                if(!(marker->access&0x200))fail(v,"lambda marker is not an interface");interfaces[ni++]=marker;
            }
        }
        if(flags&4) {
            unsigned count=lambda_boot_int(v,owner,bootstrap,&index);if(count>255)fail(v,"too many lambda bridge methods");
            for(unsigned i=0;i<count;i++) {
                if(index==bootstrap->nargs)fail(v,"missing lambda bridge descriptor");
                const char *d=utf(v,owner,cp(v,owner,bootstrap->args[index++],16)->a);int duplicate=0;
                for(unsigned j=0;j<nm;j++)if(!strcmp(descriptors[j],d))duplicate=1;if(!duplicate)descriptors[nm++]=d;
            }
        }
        if(index!=bootstrap->nargs)fail(v,"unused alternate lambda arguments");
    }
    if(serializable) {
        Class *marker=load(v,"java/io/Serializable");int present=0;
        for(unsigned i=0;i<ni;i++)if(subtype(interfaces[i],marker))present=1;
        if(!present){if(ni==256)fail(v,"too many lambda interfaces");interfaces[ni++]=marker;}
    }
    if(v->nclasses==MAX_CLASSES)fail(v,"class count limit (%d)",MAX_CLASSES);
    Class *c=(Class *)alloc(v,sizeof *c);v->classes[v->nclasses++]=c;
    char name[768];if(snprintf(name,sizeof name,"%s$$Lambda$%u",owner->name,(unsigned)(site-owner->cp))>=(int)sizeof name)fail(v,"lambda class name too long");
    c->name=copy(v,name);c->super=load(v,"java/lang/Object");c->app_loader=owner->app_loader;c->access=0x1030;
    c->ni=(uint16_t)ni;c->interfaces=(Class **)alloc(v,ni*sizeof(Class *));memcpy(c->interfaces,interfaces,ni*sizeof(Class *));
    c->nf=(uint16_t)factory.count;c->slots=factory.count;c->fields=(Field *)alloc(v,c->nf*sizeof(Field));
    for(unsigned i=0;i<c->nf;i++) {
        Field *f=&c->fields[i];char name[32];snprintf(name,sizeof name,"capture$%u",i);
        f->name=copy(v,name);f->desc=lambda_descriptor(v,factory.args[i]);f->slot=i;f->flags=0x1012;
    }
    c->nm=(uint16_t)(nm+serializable);c->methods=(Method *)alloc(v,c->nm*sizeof(Method));c->nc=1;
    LambdaBuilder builder={0};builder.vm=v;builder.cls=c;
    /* Seed a pool allocation: the first append must have room for index one. */
    builder.capacity=32;c->cp=(CP *)alloc(v,builder.capacity*sizeof(CP));
    for(unsigned j=0;j<nm;j++) {
        Method *m=&c->methods[j];m->owner=c;m->name=copy(v,sam_name);m->desc=copy(v,descriptors[j]);m->flags=(uint16_t)(0x1001|(j?0x40:0));
        LambdaSignature bridge=lambda_signature(v,descriptors[j]);lambda_specialization(v,&bridge,&dynamic);
        builder.code=NULL;builder.length=builder.code_capacity=0;
        if(kind==8){lambda_cp_instruction(&builder,0xbb,lambda_class_constant(&builder,target));lambda_byte(&builder,0x59);}
        for(unsigned i=0;i<factory.count;i++) {
            lambda_byte(&builder,0x2a);lambda_cp_instruction(&builder,0xb4,lambda_reference(&builder,9,c,c->fields[i].name,c->fields[i].desc));
        }
        unsigned local=1;
        for(unsigned i=0;i<bridge.count;i++) {
            lambda_local(&builder,bridge.args[i],local);local+=lambda_slots(bridge.args[i]);
            lambda_adapt(&builder,bridge.args[i],dynamic.args[i]);lambda_adapt(&builder,dynamic.args[i],implementation.args[factory.count+i]);
        }
        unsigned op=kind==6?0xb8:kind==8||kind==7?0xb7:kind==9?0xb9:0xb6;
        lambda_cp_instruction(&builder,op,lambda_reference(&builder,reference->tag,target,target_name,target_desc));
        if(op==0xb9){unsigned units=0;for(unsigned i=0;i<implementation.count;i++)units+=lambda_slots(implementation.args[i]);if(units>255)fail(v,"lambda invokeinterface arity too large");lambda_byte(&builder,units);lambda_byte(&builder,0);}
        lambda_adapt(&builder,implementation.result,dynamic.result);lambda_adapt(&builder,dynamic.result,bridge.result);
        lambda_byte(&builder,bridge.result->primitive=='V'?0xb1:!bridge.result->primitive?0xb0:bridge.result->primitive=='J'?0xad:bridge.result->primitive=='F'?0xae:bridge.result->primitive=='D'?0xaf:0xac);
        m->locals=(uint16_t)local;m->stack=(uint16_t)(implementation.count*2+8);m->code=builder.code;m->length=(uint32_t)builder.length;
    }
    if(serializable) {
        /* Follow InnerClassLambdaMetafactory's writeReplace protocol. The
         * original SerializedLambda owns metadata, capture cloning, and
         * readResolve; javac's actual $deserializeLambda$ validates/relinks it. */
        Method *m=&c->methods[nm];m->owner=c;m->name=copy(v,"writeReplace");m->desc=copy(v,"()Ljava/lang/Object;");m->flags=0x12;
        builder.code=NULL;builder.length=builder.code_capacity=0;
        Class *serialized=load(v,"java/lang/invoke/SerializedLambda"),*object=load(v,"java/lang/Object");
        lambda_cp_instruction(&builder,0xbb,lambda_class_constant(&builder,serialized));lambda_byte(&builder,0x59);
        lambda_cp_instruction(&builder,0x13,lambda_class_constant(&builder,owner));
        lambda_string_constant(&builder,factory.result->name);lambda_string_constant(&builder,sam_name);lambda_string_constant(&builder,sam_desc);
        lambda_small_int(&builder,kind);lambda_string_constant(&builder,target->name);lambda_string_constant(&builder,target_name);
        lambda_string_constant(&builder,target_desc);lambda_string_constant(&builder,dynamic_desc);
        lambda_small_int(&builder,factory.count);lambda_cp_instruction(&builder,0xbd,lambda_class_constant(&builder,object));
        for(unsigned i=0;i<factory.count;i++) {
            lambda_byte(&builder,0x59);lambda_small_int(&builder,i);lambda_byte(&builder,0x2a);
            lambda_cp_instruction(&builder,0xb4,lambda_reference(&builder,9,c,c->fields[i].name,c->fields[i].desc));
            lambda_adapt(&builder,factory.args[i],object);lambda_byte(&builder,0x53);
        }
        lambda_cp_instruction(&builder,0xb7,lambda_reference(&builder,10,serialized,"<init>","(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Object;)V"));
        lambda_byte(&builder,0xb0);m->locals=1;m->stack=24;m->code=builder.code;m->length=(uint32_t)builder.length;
    }
    site->lambda_class=c;return c;
}
static Value lambda_capture(VM *v,Class *owner,CP *site,const char *desc,Bootstrap *bootstrap,int alternate,Value *args,unsigned count) {
    Class *c=site->lambda_class?site->lambda_class:lambda_build(v,owner,site,desc,bootstrap,alternate);
    CP *handle=cp(v,owner,bootstrap->args[1],15);
    if(count&&(handle->a==5||handle->a==7||handle->a==9)&&!obj(args[0])){throwing(v,"java/lang/NullPointerException");return rv(NULL);}
    if(count!=c->slots)fail(v,"lambda capture count mismatch");
    initialize(v,c);if(v->exception)return rv(NULL);
    Object *o=new_object(v,c,'o',count);if(count)memcpy(o->data,args,count*sizeof(Value));return rv(o);
}
