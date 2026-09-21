/* Reflection over real classfile declarations and pinned intrinsic API metadata.
 * Returned mirrors are independent: setAccessible on one cannot affect another. */
#include "../vendor/openjdk8-api/methods.inc"
static Method *reflection_declarations(VM *v,Class *c,unsigned *count) {
    if(!c->builtin){*count=c->nm;return c->methods;}
    if(!c->reflection_loaded) {
        unsigned size=0;
        for(size_t i=0;i<sizeof builtin_method_records/sizeof *builtin_method_records;i++)if(!strcmp(c->name,builtin_method_records[i].owner))size++;
        c->reflection_methods=(Method *)alloc(v,size*sizeof(Method));c->nreflection_methods=size;unsigned used=0;
        for(size_t i=0;i<sizeof builtin_method_records/sizeof *builtin_method_records;i++) {
            const struct BuiltinMethodRecord *r=&builtin_method_records[i];if(strcmp(c->name,r->owner))continue;
            Method *m=&c->reflection_methods[used++];m->owner=c;m->name=(char *)r->name;m->desc=(char *)r->desc;m->flags=(uint16_t)r->flags;
            if(*r->exceptions) {
                m->nexceptions=1;for(const char *p=r->exceptions;*p;p++)if(*p==';')m->nexceptions++;
                m->exception_names=(char **)alloc(v,m->nexceptions*sizeof(char *));const char *p=r->exceptions;
                for(unsigned j=0;j<m->nexceptions;j++){const char *end=strchr(p,';');size_t len=end?(size_t)(end-p):strlen(p);char *name=(char *)alloc(v,len+1);memcpy(name,p,len);m->exception_names[j]=name;p+=len+(end?1:0);}
            }
        }
        c->reflection_loaded=1;
    }
    *count=c->nreflection_methods;return c->reflection_methods;
}
typedef struct { Method **methods;size_t count,capacity;Class **seen;unsigned visited; } MethodSet;
static void reflection_gather(VM *v,Class *c,Class *root,MethodSet *set,int declared) {
    if(!c||c->primitive)return;
    for(unsigned i=0;i<set->visited;i++)if(set->seen[i]==c)return;
    if(set->visited==MAX_CLASSES)fail(v,"reflection hierarchy exceeds class limit");set->seen[set->visited++]=c;
    unsigned count=0;Method *methods=c->component?NULL:reflection_declarations(v,c,&count);
    for(unsigned i=0;i<count;i++) {
        Method *m=&methods[i];if(m->name[0]=='<'||(!declared&&!(m->flags&1)))continue;
        if(!declared&&c!=root&&(c->access&0x200)&&(m->flags&STATIC))continue;
        if(set->count==set->capacity) {
            size_t capacity=set->capacity?set->capacity*2:32;if(capacity>16384)fail(v,"reflected method count exceeds 16384");
            Method **next=(Method **)alloc(v,capacity*sizeof(Method *));if(set->count)memcpy(next,set->methods,set->count*sizeof(Method *));release(v,set->methods);set->methods=next;set->capacity=capacity;
        }
        set->methods[set->count++]=m;
    }
    if(declared)return;
    if(!(c->access&0x200))reflection_gather(v,c->super,root,set,0);
    for(unsigned i=0;i<c->ni;i++)reflection_gather(v,c->interfaces[i],root,set,0);
}
static MethodSet reflection_set(VM *v,Class *c,int declared) {
    MethodSet set={0};set.seen=(Class **)alloc(v,MAX_CLASSES*sizeof(Class *));reflection_gather(v,c,c,&set,declared);release(v,set.seen);set.seen=NULL;
    if(!declared)for(size_t i=0;i<set.count;i++) {
        Method *m=set.methods[i];if(!m)continue;
        for(size_t j=0;j<set.count;j++) {
            Method *n=set.methods[j];if(!n||n==m||strcmp(n->name,m->name)||strcmp(n->desc,m->desc))continue;
            int mi=!!(m->owner->access&0x200),ni=!!(n->owner->access&0x200);
            if((mi&&!ni)||(mi==ni&&subtype(n->owner,m->owner))){set.methods[i]=NULL;break;}
        }
    }
    size_t count=0;for(size_t i=0;i<set.count;i++)if(set.methods[i])set.methods[count++]=set.methods[i];set.count=count;return set;
}
static void reflection_resolve_signature(VM *v,Method *m) {
    const char *p=m->desc+1;while(*p&&*p!=')')descriptor_class(v,&p);
    if(*p!=')')fail(v,"invalid reflected method descriptor");p++;descriptor_class(v,&p);
    for(unsigned i=0;i<m->nexceptions;i++)load(v,m->exception_names[i]);
}
static Object *reflection_methods(VM *v,Class *c,int declared) {
    MethodSet set=reflection_set(v,c,declared);
    for(size_t i=0;i<set.count;i++)reflection_resolve_signature(v,set.methods[i]);
    Object *out=array_new(v,"[Ljava/lang/reflect/Method;",(int32_t)set.count);root(v,out);
    for(size_t i=0;i<set.count;i++)out->data[i]=rv(annotation_method_mirror(v,set.methods[i]));
    v->nr--;release(v,set.methods);return out;
}
static int reflection_parameters_match(VM *v,Method *m,Object *params) {
    size_t count=params?params->count:0;if(nargs(v,m->desc)!=count)return 0;
    const char *p=m->desc+1;
    for(size_t i=0;i<count;i++) {Object *type=obj(params->data[i]);Class *c=descriptor_class(v,&p);if(!type||type->kind!='c'||type->represented!=c)return 0;}return 1;
}
static Class *reflection_return(VM *v,Method *m){const char *p=strchr(m->desc,')')+1;return descriptor_class(v,&p);}
static Object *reflection_lookup(VM *v,Class *c,Object *name,Object *params,int declared) {
    if(!name){throwing(v,"java/lang/NullPointerException");return NULL;}
    if(params&&(params->kind!='a'||strcmp(params->array_desc,"[Ljava/lang/Class;")))fail(v,"method parameter types require Class[]");
    MethodSet set=reflection_set(v,c,declared);Method *found=NULL;
    for(size_t i=0;i<set.count;i++) {
        Method *m=set.methods[i];if(strcmp(m->name,name->text)||!reflection_parameters_match(v,m,params))continue;
        if(!found||subtype(reflection_return(v,m),reflection_return(v,found)))found=m;
    }
    release(v,set.methods);if(!found){throwing(v,"java/lang/NoSuchMethodException");return NULL;}
    reflection_resolve_signature(v,found);return annotation_method_mirror(v,found);
}
static int reflection_method_access(VM *v,Object *mirror,Object *receiver) {
    if(mirror->accessible)return 1;Method *m=mirror->represented_method;Class *owner=m->owner,*caller=v->frame?v->frame->method->owner:NULL;
    if(caller==owner)return 1;
    if(!(owner->access&1)&&!same_package(owner,caller))return 0;
    if(m->flags&1)return 1;if(m->flags&2)return 0;
    if(same_package(owner,caller))return 1;
    return (m->flags&4)&&caller&&subtype(caller,owner)&&((m->flags&STATIC)||(receiver&&subtype(receiver->cls,caller)));
}
static int reflection_convert(VM *v,Value value,Class *to,Value *result) {
    Object *o=obj(value);*result=value;
    if(!to->primitive){if(!o||subtype(o->cls,to))return 1;throwing(v,"java/lang/IllegalArgumentException");return 0;}
    const char *p=o?wrapper_primitive(o->cls->name):NULL;
    unsigned from=p?load(v,p)->primitive:0;
    if(!from||!lambda_widen(from,to->primitive)){throwing(v,"java/lang/IllegalArgumentException");return 0;}
    if(!o->count||(o->kind!='w'&&o->kind!='B'))fail(v,"wrapper value is unavailable for reflection: %s",o->cls->name);
    Value a=o->data[0];
    if(to->primitive=='D')*result=dv(a.tag==DOUBLE?dbl(a):a.tag==FLOAT?(double)flt(a):a.tag==LONG?(double)(int64_t)a.bits:(double)integer(a));
    else if(to->primitive=='F')*result=fv(a.tag==FLOAT?flt(a):a.tag==LONG?(float)(int64_t)a.bits:(float)integer(a));
    else if(to->primitive=='J')*result=a.tag==LONG?a:val((uint64_t)(int64_t)integer(a),LONG);
    else *result=a;return 1;
}
static Method *reflection_virtual(VM *v,Method *original,Class *receiver) {
    if(original->flags&(STATIC|2))return original;
    Method *target=original;Class *chain[MAX_DEPTH];unsigned depth=0;
    for(Class *c=receiver;c;c=c->super){if(depth==MAX_DEPTH)fail(v,"reflection receiver hierarchy exceeds depth limit");chain[depth++]=c;}
    while(depth) {
        Class *c=chain[--depth];unsigned count;Method *methods=reflection_declarations(v,c,&count);
        for(unsigned i=0;i<count;i++) {
            Method *m=&methods[i];if((m->flags&(STATIC|2))||strcmp(m->name,original->name)||strcmp(m->desc,original->desc))continue;
            if(m==target)continue;
            if((target->owner->access&0x200)||((target->flags&5)||same_package(target->owner,c)))target=m;
        }
    }
    if(target==original&&(original->owner->access&0x200)) {
        Method *m=default_method(v,receiver,original->name,original->desc);if(m)target=m;
    }
    return target;
}
static Object *reflection_invoke(VM *v,Object *mirror,Object *receiver,Object *args) {
    Method *m=mirror->represented_method;int stat=!!(m->flags&STATIC);unsigned count=nargs(v,m->desc),saved=v->nr;
    if(!stat){if(!receiver){throwing(v,"java/lang/NullPointerException");return NULL;}if(!subtype(receiver->cls,m->owner)){throwing(v,"java/lang/IllegalArgumentException");return NULL;}}
    if(!reflection_method_access(v,mirror,receiver)){throwing(v,"java/lang/IllegalAccessException");return NULL;}
    if(stat){initialize(v,m->owner);if(v->exception)return NULL;}
    if(args&&(args->kind!='a'||(args->array_desc[1]!='L'&&args->array_desc[1]!='[')))fail(v,"Method.invoke requires Object[]");
    if((args?args->count:0)!=count){throwing(v,"java/lang/IllegalArgumentException");return NULL;}
    Object *snapshot=new_object(v,load(v,"java/lang/Object"),'o',count+1);root(v,snapshot);snapshot->data[0]=rv(receiver);
    const char *p=m->desc+1;
    for(unsigned i=0;i<count;i++)if(!reflection_convert(v,args->data[i],descriptor_class(v,&p),&snapshot->data[i+1])){v->nr=saved;return NULL;}
    Method *target=stat?m:reflection_virtual(v,m,receiver->cls);Value result=iv(0);
    if(!v->exception) {
        Value *a=snapshot->data+(stat?1:0);unsigned n=count+(stat?0:1);
        result=execute(v,target,a,n);
    }
    if(v->exception) {
        Object *cause=v->exception;root(v,cause);v->exception=NULL;
        Object *exception=new_object(v,load(v,"java/lang/reflect/InvocationTargetException"),'o',0);exception->cause=cause;v->exception=exception;v->nr=saved;return NULL;
    }
    Class *returns=reflection_return(v,m);Object *boxed=NULL;
    if(!returns->primitive)boxed=obj(result);
    else if(returns->primitive!='V') {
        Class *wrapper=load(v,lambda_wrapper(returns->primitive));initialize(v,wrapper);
        if(!v->exception){char desc[96];snprintf(desc,sizeof desc,"(%c)L%s;",returns->primitive,wrapper->name);boxed=obj(native_call(v,wrapper,"valueOf",desc,&result,1,1));}
    }
    v->nr=saved;return boxed;
}
static Object *reflection_package(VM *v,Class *c) {
    if(c->primitive||c->component)return NULL;if(c->package_object)return c->package_object;
    const char *slash=strrchr(c->name,'/');size_t length=slash?(size_t)(slash-c->name):0;
    char *name=(char *)alloc(v,length+1);for(size_t i=0;i<length;i++)name[i]=c->name[i]=='/'?'.':c->name[i];
    Object *result=NULL;
    for(int i=0;i<v->nclasses;i++){Class *other=v->classes[i];if(other->app_loader==c->app_loader&&other->package_object&&!strcmp(other->package_object->text,name)){result=other->package_object;break;}}
    if(!result){result=new_object(v,load(v,"java/lang/Package"),'k',0);set_text(v,result,name);}
    release(v,name);c->package_object=result;return result;
}
static Value method_reflection_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int stat,int *handled) {
    *handled=0;if(stat)return iv(0);Object *self=obj(args[0]);
    if(!strcmp(c->name,"java/lang/Class")) {
        if(!strcmp(name,"getPackage")&&!strcmp(desc,"()Ljava/lang/Package;")){*handled=1;return rv(reflection_package(v,self->represented));}
        int declared=!strncmp(name,"getDeclared",11);
        if((!strcmp(name,"getMethods")||!strcmp(name,"getDeclaredMethods"))&&!strcmp(desc,"()[Ljava/lang/reflect/Method;")){*handled=1;return rv(reflection_methods(v,self->represented,declared));}
        if((!strcmp(name,"getMethod")||!strcmp(name,"getDeclaredMethod"))&&!strcmp(desc,"(Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/reflect/Method;")){*handled=1;return rv(reflection_lookup(v,self->represented,obj(args[1]),obj(args[2]),declared));}
    }
    if(self->kind=='k'&&!strcmp(name,"getName")&&!strcmp(desc,"()Ljava/lang/String;")){*handled=1;return rv(string(v,self->text));}
    if(self->kind!='M'||!self->represented_method)return iv(0);Method *m=self->represented_method;*handled=1;
    if(m->owner->builtin&&strstr(name,"Annotation"))fail(v,"intrinsic method annotation metadata is not imported: %s.%s",m->owner->name,m->name);
    if(!strcmp(name,"invoke")&&!strcmp(desc,"(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;"))return rv(reflection_invoke(v,self,obj(args[1]),obj(args[2])));
    if(!strcmp(name,"getName")&&!strcmp(desc,"()Ljava/lang/String;"))return rv(string(v,m->name));
    if(!strcmp(name,"getDeclaringClass")&&!strcmp(desc,"()Ljava/lang/Class;"))return rv(class_mirror(v,m->owner));
    if(!strcmp(name,"getReturnType")&&!strcmp(desc,"()Ljava/lang/Class;"))return rv(class_mirror(v,reflection_return(v,m)));
    if(!strcmp(name,"getModifiers")&&!strcmp(desc,"()I"))return iv(m->flags&0x1dff);
    if(!strcmp(name,"getParameterCount")&&!strcmp(desc,"()I"))return iv((int32_t)nargs(v,m->desc));
    if(!strcmp(name,"isBridge")&&!strcmp(desc,"()Z"))return iv(!!(m->flags&0x40));
    if(!strcmp(name,"isVarArgs")&&!strcmp(desc,"()Z"))return iv(!!(m->flags&0x80));
    if(!strcmp(name,"isSynthetic")&&!strcmp(desc,"()Z"))return iv(!!(m->flags&0x1000));
    if(!strcmp(name,"isDefault")&&!strcmp(desc,"()Z"))return iv((m->owner->access&0x200)&&!(m->flags&(STATIC|0x400|2)));
    if(!strcmp(name,"isAccessible")&&!strcmp(desc,"()Z"))return iv(self->accessible);
    if(!strcmp(name,"setAccessible")&&!strcmp(desc,"(Z)V")){self->accessible=!!integer(args[1]);return iv(0);}
    if(!strcmp(name,"equals")&&!strcmp(desc,"(Ljava/lang/Object;)Z")) {Object *other=obj(args[1]);return iv(other&&other->kind=='M'&&other->represented_method==m);}
    if(!strcmp(name,"hashCode")&&!strcmp(desc,"()I")) {
        uint32_t hash=0;const unsigned char *p=(const unsigned char *)m->owner->name;while(*p){unsigned ch=utf_unit(&p);hash=31*hash+(ch=='/'?'.':ch);}return iv((int32_t)(hash^annotation_string_hash(m->name)));
    }
    if(!strcmp(name,"getParameterTypes")&&!strcmp(desc,"()[Ljava/lang/Class;")) {
        unsigned count=nargs(v,m->desc);Object *types=array_new(v,"[Ljava/lang/Class;",(int32_t)count);root(v,types);const char *p=m->desc+1;
        for(unsigned i=0;i<count;i++)types->data[i]=rv(class_mirror(v,descriptor_class(v,&p)));v->nr--;return rv(types);
    }
    if(!strcmp(name,"getExceptionTypes")&&!strcmp(desc,"()[Ljava/lang/Class;")) {
        Object *types=array_new(v,"[Ljava/lang/Class;",(int32_t)m->nexceptions);root(v,types);
        for(unsigned i=0;i<m->nexceptions;i++)types->data[i]=rv(class_mirror(v,load(v,m->exception_names[i])));v->nr--;return rv(types);
    }
    *handled=0;return iv(0);
}
