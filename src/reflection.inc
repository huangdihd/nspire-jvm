/* Constructor reflection for bytecode-defined classes. Methods, fields,
 * annotations, nestmate access and most builtin constructors are still partial. */
static Class *descriptor_class(VM *v,const char **cursor) {
    const char *start=*cursor,*p=start;while(*p=='[')p++;
    if(*p=='L'){p=strchr(p,';');if(!p)fail(v,"invalid reflection descriptor");p++;}else if(*p)p++;else fail(v,"truncated reflection descriptor");
    *cursor=p;if(*start!='['&&*start!='L')return load(v,primitive_name(*start));
    size_t n=(size_t)(p-start);if(*start=='L'){start++;n-=2;}
    char *name=(char *)alloc(v,n+1);memcpy(name,start,n);Class *type=load(v,name);release(v,name);return type;
}
static const char *constructor_desc(Object *o){return o->represented_method?o->represented_method->desc:"()V";}
static unsigned constructor_flags(Object *o){return o->represented_method?o->represented_method->flags:1;}
static Object *lookup_constructor(VM *v,Class *type,Object *params,int declared) {
    size_t count=params?params->count:0;
    if(params&&(params->kind!='a'||strcmp(params->array_desc,"[Ljava/lang/Class;")))fail(v,"constructor parameter types require Class[]");
    if(type->builtin&&!type->primitive&&!type->component&&!(type->access&0x200)&&strcmp(type->name,"java/lang/Object"))fail(v,"reflection of builtin constructors is not implemented: %s",type->name);
    Method *found=NULL;int matched=!count&&!strcmp(type->name,"java/lang/Object");
    for(unsigned i=0;!matched&&i<type->nm;i++) {
        Method *m=&type->methods[i];if(strcmp(m->name,"<init>")||(!declared&&!(m->flags&1))||nargs(v,m->desc)!=count)continue;
        const char *p=m->desc+1;int equal=1;
        for(size_t j=0;j<count;j++) {
            Object *mirror=obj(params->data[j]);Class *expected=descriptor_class(v,&p);
            if(!mirror||mirror->kind!='c'||mirror->represented!=expected)equal=0;
        }
        if(equal){found=m;matched=1;}
    }
    if(!matched){throwing(v,"java/lang/NoSuchMethodException");return NULL;}
    Object *result=new_object(v,load(v,"java/lang/reflect/Constructor"),'C',0);
    result->represented=type;result->represented_method=found;return result;
}
static int same_package(Class *a,Class *b) {
    if(!a||!b||a->app_loader!=b->app_loader)return 0;
    const char *x=strrchr(a->name,'/'),*y=strrchr(b->name,'/');
    size_t nx=x?(size_t)(x-a->name):0,ny=y?(size_t)(y-b->name):0;
    return nx==ny&&!strncmp(a->name,b->name,nx);
}
static int constructor_access(VM *v,Object *o) {
    if(o->accessible)return 1;
    Class *owner=o->represented,*caller=v->frame?v->frame->method->owner:NULL;
    if(caller==owner)return 1;
    if(!(owner->access&1)&&!same_package(owner,caller))return 0;
    unsigned flags=constructor_flags(o);if(flags&1)return 1;if(flags&2)return 0;
    return same_package(owner,caller);
}
static Object *construct(VM *v,Object *ctor,Object *args) {
    Class *owner=ctor->represented;unsigned count=nargs(v,constructor_desc(ctor));
    if(args&&(args->kind!='a'||(args->array_desc[1]!='L'&&args->array_desc[1]!='[')))fail(v,"Constructor.newInstance requires Object[]");
    if(!constructor_access(v,ctor)){throwing(v,"java/lang/IllegalAccessException");return NULL;}
    if(owner->access&0x400){throwing(v,"java/lang/InstantiationException");return NULL;}
    if(owner->access&0x4000){throwing(v,"java/lang/IllegalArgumentException");return NULL;}
    /* HotSpot initializes the declaring class before reading/converting args;
     * its initializer can therefore change the supplied argument array. */
    initialize(v,owner);if(v->exception)return NULL;
    if((args?args->count:0)!=count){throwing(v,"java/lang/IllegalArgumentException");return NULL;}
    /* Keep converted arguments rooted across allocation and constructor code.
     * Wide values occupy one Value here. */
    Object *snapshot=new_object(v,load(v,"java/lang/Object"),'o',(size_t)count+1);root(v,snapshot);
    const char *p=constructor_desc(ctor)+1;
    for(unsigned i=0;i<count;i++) {
        Class *type=descriptor_class(v,&p);Value value=args->data[i];Object *o=obj(value);
        if(!type->primitive) {
            if(o&&!subtype(o->cls,type)){throwing(v,"java/lang/IllegalArgumentException");break;}
        } else if(o&&o->kind=='B'&&type->primitive=='Z')value=o->data[0];
        else if(o&&o->kind=='w') {
            int32_t x=integer(o->data[0]);
            switch(type->primitive){case 'I':value=iv(x);break;case 'J':value=val((uint64_t)(int64_t)x,LONG);break;case 'F':value=fv((float)x);break;case 'D':value=dv((double)x);break;default:throwing(v,"java/lang/IllegalArgumentException");}
        } else {if(o&&o->kind!='B'&&wrapper_primitive(o->cls->name))fail(v,"reflection unboxing is not implemented for %s",o->cls->name);throwing(v,"java/lang/IllegalArgumentException");}
        if(v->exception)break;snapshot->data[i+1]=value;
    }
    if(v->exception){v->nr--;return NULL;}
    Object *o=new_object(v,owner,'o',owner->slots);snapshot->data[0]=rv(o);
    for(Class *k=owner;k;k=k->super)for(unsigned i=0;i<k->nf;i++)if(!(k->fields[i].flags&STATIC))o->data[k->fields[i].slot]=zero(k->fields[i].desc);
    if(ctor->represented_method)execute(v,ctor->represented_method,snapshot->data,count+1);
    if(v->exception) {
        Object *cause=v->exception;root(v,cause);v->exception=NULL;
        Object *wrapped=new_object(v,load(v,"java/lang/reflect/InvocationTargetException"),'o',0);wrapped->cause=cause;
        v->exception=wrapped;v->nr--;
    }
    v->nr--;return v->exception?NULL:o;
}
static Value reflection_native(VM *v,Class *c,const char *n,const char *d,Value *a,int stat,int *handled) {
    Object *self=stat?NULL:obj(a[0]);*handled=1;
    if(!stat&&!strcmp(c->name,"java/lang/Class")&&
       (!strcmp(n,"getConstructor")||!strcmp(n,"getDeclaredConstructor"))&&!strcmp(d,"([Ljava/lang/Class;)Ljava/lang/reflect/Constructor;"))
        return rv(lookup_constructor(v,self->represented,obj(a[1]),!strcmp(n,"getDeclaredConstructor")));
    if(!stat&&!strcmp(c->name,"java/lang/reflect/InvocationTargetException")&&!strcmp(n,"getTargetException")&&!strcmp(d,"()Ljava/lang/Throwable;"))return rv(self->cause);
    if(self&&self->kind=='C') {
        if(!strcmp(n,"newInstance")&&!strcmp(d,"([Ljava/lang/Object;)Ljava/lang/Object;"))return rv(construct(v,self,obj(a[1])));
        if(!strcmp(n,"getDeclaringClass")&&!strcmp(d,"()Ljava/lang/Class;"))return rv(class_mirror(v,self->represented));
        if(!strcmp(n,"getName")&&!strcmp(d,"()Ljava/lang/String;"))return rv(class_name_string(v,self->represented,0));
        if(!strcmp(n,"getModifiers")&&!strcmp(d,"()I"))return iv((int32_t)constructor_flags(self));
        if(!strcmp(n,"getParameterCount")&&!strcmp(d,"()I"))return iv((int32_t)nargs(v,constructor_desc(self)));
        if(!strcmp(n,"isVarArgs")&&!strcmp(d,"()Z"))return iv(!!(constructor_flags(self)&0x80));
        if(!strcmp(n,"isSynthetic")&&!strcmp(d,"()Z"))return iv(!!(constructor_flags(self)&0x1000));
        if(!strcmp(n,"isAccessible")&&!strcmp(d,"()Z"))return iv(self->accessible);
        if(!strcmp(n,"setAccessible")&&!strcmp(d,"(Z)V")){self->accessible=!!integer(a[1]);return iv(0);}
        if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")){Object *o=obj(a[1]);return iv(o&&o->kind=='C'&&o->represented==self->represented&&o->represented_method==self->represented_method);}
        if(!strcmp(n,"hashCode")&&!strcmp(d,"()I")) {
            uint32_t hash=0;const unsigned char *p=(const unsigned char *)self->represented->name;
            while(*p){unsigned ch=utf_unit(&p);hash=31*hash+(ch=='/'?'.':ch);}return iv((int32_t)hash);
        }
        if(!strcmp(n,"getParameterTypes")&&!strcmp(d,"()[Ljava/lang/Class;")) {
            unsigned count=nargs(v,constructor_desc(self));Object *types=array_new(v,"[Ljava/lang/Class;",(int32_t)count);root(v,types);
            const char *p=constructor_desc(self)+1;for(unsigned i=0;i<count;i++)types->data[i]=rv(class_mirror(v,descriptor_class(v,&p)));
            v->nr--;return rv(types);
        }
    }
    *handled=0;return iv(0);
}
