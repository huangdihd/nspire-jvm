/* Parameter objects execute the preserved OpenJDK Java implementation. Their
 * names/flags originate in MethodParameters; types originate in descriptors.
 * Each executable mirror owns its cache, traced through the ordinary GC. */
static void parameter_error(VM *v,const char *message) {
    Value arg=rv(string(v,message));
    v->exception=annotation_exception(v,"java/lang/reflect/MalformedParametersException","(Ljava/lang/String;)V",&arg,1);
}
static int parameter_verify(VM *v,Method *m,unsigned count) {
    if(!m||!m->parameters.bytes)return 1;
    Reader r=annotation_reader(v,&m->parameters);unsigned entries=readn(&r,1);
    if(entries!=count){parameter_error(v,"Wrong number of parameters in MethodParameters attribute");return 0;}
    for(unsigned i=0;i<entries;i++) {
        unsigned index=readn(&r,2),flags=readn(&r,2);
        if(index) {
            if(index>=m->owner->nc||m->owner->cp[index].tag!=1){parameter_error(v,"Invalid constant pool index");return 0;}
            const char *name=m->owner->cp[index].text;
            if(!*name||strpbrk(name,".;[/")){parameter_error(v,"Invalid parameter name");return 0;}
        }
        if(flags&~0x9010U){parameter_error(v,"Invalid parameter modifiers");return 0;}
    }
    return 1;
}
static Object *executable_parameters(VM *v,Object *self) {
    if(self->parameters_cache)return self->parameters_cache;
    Method *m=self->represented_method;const char *desc=m?m->desc:constructor_desc(self);
    unsigned count=nargs(v,desc),saved=v->nr;root(v,self);
    if(!parameter_verify(v,m,count)){v->nr=saved;return NULL;}
    Class *type=load(v,"java/lang/reflect/Parameter");initialize(v,type);
    if(v->exception){v->nr=saved;return NULL;}
    Method *ctor=method(type,"<init>","(Ljava/lang/String;ILjava/lang/reflect/Executable;I)V");
    if(!ctor)fail(v,"Parameter constructor is missing");
    Object *array=array_new(v,"[Ljava/lang/reflect/Parameter;",(int32_t)count);root(v,array);
    Reader r={0};if(m&&m->parameters.bytes){r=annotation_reader(v,&m->parameters);(void)readn(&r,1);}
    for(unsigned i=0;i<count&&!v->exception;i++) {
        const char *name=NULL;unsigned flags=0;char synthesized[32];
        if(r.p){unsigned index=readn(&r,2);flags=readn(&r,2);if(index)name=m->owner->cp[index].text;}
        else {snprintf(synthesized,sizeof synthesized,"arg%u",i);name=synthesized;}
        Object *o=new_object(v,type,'o',type->slots);array->data[i]=rv(o);
        for(Class *c=type;c;c=c->super)for(unsigned j=0;j<c->nf;j++)if(!(c->fields[j].flags&STATIC))o->data[c->fields[j].slot]=zero(c->fields[j].desc);
        Value args[]={rv(o),rv(name?string(v,name):NULL),iv((int32_t)flags),rv(self),iv((int32_t)i)};
        execute(v,ctor,args,5);
    }
    if(!v->exception)self->parameters_cache=array;v->nr=saved;return v->exception?NULL:array;
}
static Object *parameter_types(VM *v,Object *self) {
    Method *m=self->represented_method;const char *desc=m?m->desc:constructor_desc(self),*p=desc+1;
    unsigned count=nargs(v,desc);Object *types=array_new(v,"[Ljava/lang/Class;",(int32_t)count);root(v,types);
    for(unsigned i=0;i<count;i++)types->data[i]=rv(class_mirror(v,descriptor_class(v,&p)));
    v->nr--;return types;
}
static Object *parameter_annotations(VM *v,Object *self) {
    Method *m=self->represented_method;unsigned count=nargs(v,m?m->desc:constructor_desc(self));
    if(m&&m->owner->builtin)fail(v,"intrinsic parameter annotation metadata is not imported");
    Reader r={0};unsigned entries=count;
    if(m&&m->parameter_annotations.bytes) {r=annotation_reader(v,&m->parameter_annotations);entries=readn(&r,1);}
    /* Java 8 permits shortened annotation tables for synthetic constructor
     * arguments in enums and inner/local/anonymous classes. Preserve the raw
     * table in those cases, as the original Constructor does. */
    if(entries!=count) {
        Class *owner=self->represented;int special=self->kind=='C'&&
            ((owner->access&0x4000)||owner->local_class||
             (owner->declaring_name&&((owner->inner_access&STATIC)||entries+1==count)));
        if(!special){throwing(v,"java/lang/annotation/AnnotationFormatError");return NULL;}
    }
    Object *out=array_new(v,"[[Ljava/lang/annotation/Annotation;",(int32_t)entries);root(v,out);
    for(unsigned i=0;i<entries&&!v->exception;i++) {
        unsigned number=r.p?readn(&r,2):0;Object *row=array_new(v,"[Ljava/lang/annotation/Annotation;",(int32_t)number);out->data[i]=rv(row);unsigned used=0;
        for(unsigned j=0;j<number&&!v->exception;j++) {
            Object *o=annotation_parse(&r,m->owner,0,1);if(!o)continue;
            for(unsigned k=0;k<used;k++)if(obj(row->data[k])->represented==o->represented){throwing(v,"java/lang/annotation/AnnotationFormatError");break;}
            if(!v->exception)row->data[used++]=rv(o);
        }
        if(!v->exception&&used!=number){Object *trim=array_new(v,"[Ljava/lang/annotation/Annotation;",(int32_t)used);memcpy(trim->data,row->data,used*sizeof(Value));out->data[i]=rv(trim);}
    }
    if(r.p&&!v->exception&&r.pos!=r.n)fail(v,"invalid RuntimeVisibleParameterAnnotations length");
    v->nr--;return v->exception?NULL:out;
}
static Object *parameter_type_name(VM *v,Class *c) {
    unsigned dimensions=0;while(c->component){dimensions++;c=c->component;}
    size_t length=strlen(c->name);char *text=(char *)alloc(v,length+dimensions*2+1);
    for(size_t i=0;i<length;i++)text[i]=c->name[i]=='/'?'.':c->name[i];
    for(unsigned i=0;i<dimensions;i++){text[length++]='[';text[length++]=']';}
    Object *result=string(v,text);release(v,text);return result;
}
static Value parameter_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int stat,int *handled) {
    (void)c;*handled=0;if(stat)return iv(0);Object *self=obj(args[0]);
    if(self->kind=='c'&&!strcmp(name,"getTypeName")&&!strcmp(desc,"()Ljava/lang/String;")) {
        *handled=1;return rv(parameter_type_name(v,self->represented));
    }
    if(!strcmp(self->cls->name,"java/lang/reflect/Parameter")&&!strcmp(name,"isAnnotationPresent")&&!strcmp(desc,"(Ljava/lang/Class;)Z")) {
        *handled=1;Method *get=method(self->cls,"getAnnotation","(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;");
        Value result=execute(v,get,args,2);return iv(!v->exception&&obj(result)!=NULL);
    }
    if(self->kind!='M'&&self->kind!='C')return iv(0);Method *m=self->represented_method;
    if(!strcmp(name,"getParameters")&&!strcmp(desc,"()[Ljava/lang/reflect/Parameter;")) {
        *handled=1;Object *cache=executable_parameters(v,self);if(!cache)return rv(NULL);
        Object *result=array_new(v,cache->array_desc,(int32_t)cache->count);memcpy(result->data,cache->data,cache->count*sizeof(Value));return rv(result);
    }
    if(!strcmp(name,"hasRealParameterData")&&!strcmp(desc,"()Z")) {
        *handled=1;executable_parameters(v,self);return iv(!v->exception&&m&&m->parameters.bytes);
    }
    if((!strcmp(name,"getGenericParameterTypes")||!strcmp(name,"getAllGenericParameterTypes"))&&!strcmp(desc,"()[Ljava/lang/reflect/Type;")) {
        *handled=1;
        if(m&&(m->signature||m->owner->builtin))fail(v,"generic parameter signatures are not implemented: %s.%s",m->owner->name,m->name);
        return rv(parameter_types(v,self));
    }
    if(!strcmp(name,"getParameterAnnotations")&&!strcmp(desc,"()[[Ljava/lang/annotation/Annotation;")) {
        *handled=1;return rv(parameter_annotations(v,self));
    }
    return iv(0);
}
