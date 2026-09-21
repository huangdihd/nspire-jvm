/* Runtime-visible classfile annotations. Concrete implementation classes use
 * native member accessors; all values originate in classfile attributes.
 * Member values and deferred Java exceptions are ordinary GC-traced Values. */
static void annotation_skip_value(Reader *,Class *,unsigned);
static void annotation_skip_body(Reader *r,Class *owner,unsigned depth) {
    if(depth>64)fail(r->v,"annotation nesting limit exceeded");
    (void)utf(r->v,owner,readn(r,2));unsigned count=readn(r,2);
    while(count--){(void)utf(r->v,owner,readn(r,2));annotation_skip_value(r,owner,depth+1);}
}
static void annotation_skip_value(Reader *r,Class *owner,unsigned depth) {
    if(depth>64)fail(r->v,"annotation nesting limit exceeded");
    unsigned tag=readn(r,1);
    if(strchr("BCDFIJSZsc",(int)tag)&&tag){(void)readn(r,2);return;}
    if(tag=='e'){(void)readn(r,2);(void)readn(r,2);return;}
    if(tag=='@'){annotation_skip_body(r,owner,depth+1);return;}
    if(tag=='['){unsigned n=readn(r,2);while(n--)annotation_skip_value(r,owner,depth+1);return;}
    fail(r->v,"invalid annotation value tag %u",tag);
}
static Reader annotation_reader(VM *v,AnnotationData *data){Reader r={v,data->bytes,data->length,0};return r;}
static int annotation_raw(VM *v,Class *owner,const char *descriptor,Reader *found) {
    AnnotationData *data=&owner->annotations;if(!data->bytes)return 0;
    Reader r=annotation_reader(v,data);unsigned count=readn(&r,2);
    while(count--){Reader start=r;const char *name=utf(v,owner,readn(&r,2));r=start;annotation_skip_body(&r,owner,0);
        if(!strcmp(name,descriptor)){*found=start;return 1;}}
    return 0;
}
static int annotation_runtime(VM *v,Class *type) {
    Reader r;if(!annotation_raw(v,type,"Ljava/lang/annotation/Retention;",&r))return 0;
    (void)readn(&r,2);unsigned count=readn(&r,2);
    while(count--){const char *name=utf(v,type,readn(&r,2));Reader start=r;
        if(!strcmp(name,"value")&&readn(&r,1)=='e') {
            const char *desc=utf(v,type,readn(&r,2)),*value=utf(v,type,readn(&r,2));
            return !strcmp(desc,"Ljava/lang/annotation/RetentionPolicy;")&&!strcmp(value,"RUNTIME");
        }
        r=start;annotation_skip_value(&r,type,0);
    }
    return 0;
}
static int annotation_inherited(VM *v,Class *type) {Reader r;return annotation_raw(v,type,"Ljava/lang/annotation/Inherited;",&r);}
static int annotation_member(Method *m){return !(m->flags&STATIC)&&m->name[0]!='<'&&!strncmp(m->desc,"()",2);}
static Class *annotation_implementation(VM *v,Class *type) {
    if(type->annotation_impl)return type->annotation_impl;
    if(v->nclasses==MAX_CLASSES)fail(v,"class count limit (%d)",MAX_CLASSES);
    Class *c=(Class *)alloc(v,sizeof *c);char name[80];snprintf(name,sizeof name,"nspire/$Annotation$%d",v->nclasses);
    c->name=copy(v,name);c->super=load(v,"java/lang/Object");c->access=0x11;c->init=2;c->app_loader=type->app_loader;
    c->annotation_type=type;c->ni=2;c->interfaces=(Class **)alloc(v,2*sizeof(Class *));c->interfaces[0]=type;c->interfaces[1]=load(v,"java/io/Serializable");
    c->methods=(Method *)alloc(v,((size_t)type->nm+4)*sizeof(Method));
    for(unsigned i=0;i<type->nm;i++)if(annotation_member(&type->methods[i])) {
        Method *m=&c->methods[c->nm++];m->owner=c;m->name=type->methods[i].name;m->desc=type->methods[i].desc;m->flags=1|NATIVE;
    }
    const char *names[]={"annotationType","equals","hashCode","toString"};
    const char *descs[]={"()Ljava/lang/Class;","(Ljava/lang/Object;)Z","()I","()Ljava/lang/String;"};
    for(unsigned i=0;i<4;i++){Method *m=&c->methods[c->nm++];m->owner=c;m->name=(char *)names[i];m->desc=(char *)descs[i];m->flags=1|NATIVE;}
    if(v->nclasses==MAX_CLASSES)fail(v,"class count limit (%d)",MAX_CLASSES);
    v->classes[v->nclasses++]=c;type->annotation_impl=c;return c;
}
static Object *annotation_exception(VM *v,const char *name,const char *desc,Value *args,unsigned count) {
    unsigned saved=v->nr;for(unsigned i=0;i<count;i++)if(args[i].tag==REF)root(v,obj(args[i]));
    Class *c=load(v,name);initialize(v,c);if(v->exception){v->nr=saved;return v->exception;}
    Object *o=new_object(v,c,'o',c->slots);root(v,o);
    for(Class *p=c;p;p=p->super)for(unsigned i=0;i<p->nf;i++)if(!(p->fields[i].flags&STATIC))o->data[p->fields[i].slot]=zero(p->fields[i].desc);
    Value call[4];if(count>3)fail(v,"too many annotation exception arguments");call[0]=rv(o);for(unsigned i=0;i<count;i++)call[i+1]=args[i];
    Method *ctor=method(c,"<init>",desc);if(!ctor)fail(v,"missing annotation exception constructor");execute(v,ctor,call,count+1);
    v->nr=saved;return v->exception?v->exception:o;
}
static Class *annotation_resolve(VM *v,const char *desc,int optional) {
    const char *p=desc;char *name=NULL;
    if(*p=='L'){size_t n=strlen(p);if(n<3||p[n-1]!=';')fail(v,"invalid annotation descriptor");name=(char *)alloc(v,n-1);memcpy(name,p+1,n-2);p=name;}
    else if(*p!='['){if(p[1]||!primitive_name(*p))fail(v,"invalid annotation descriptor");return load(v,primitive_name(*p));}
    if(!class_available(v,p)) {
        if(!optional){char *text=copy(v,p);for(char *q=text;*q;q++)if(*q=='/')*q='.';
            Object *s=string(v,text);release(v,text);Value args[]={rv(s),rv(NULL)};
            v->exception=annotation_exception(v,"java/lang/TypeNotPresentException","(Ljava/lang/String;Ljava/lang/Throwable;)V",args,2);}
        release(v,name);return NULL;
    }
    Class *result=load(v,p);release(v,name);return result;
}
static Object *annotation_method_mirror(VM *v,Method *m) {
    Object *o=new_object(v,load(v,"java/lang/reflect/Method"),'M',0);o->represented=m->owner;o->represented_method=m;return o;
}
static void annotation_mismatch(VM *v,Method *member,unsigned tag) {
    Object *mirror=annotation_method_mirror(v,member);root(v,mirror);char text[48];snprintf(text,sizeof text,"classfile annotation tag %c",tag);
    Value args[]={rv(mirror),rv(string(v,text))};v->exception=annotation_exception(v,"java/lang/annotation/AnnotationTypeMismatchException","(Ljava/lang/reflect/Method;Ljava/lang/String;)V",args,2);v->nr--;
}
static Object *annotation_parse(Reader *,Class *,unsigned,int);
static Value annotation_value(Reader *r,Class *owner,const char *expected,Method *member,unsigned depth) {
    VM *v=r->v;if(depth>64)fail(v,"annotation nesting limit exceeded");
    Reader start=*r;unsigned tag=readn(r,1);Value result=iv(0);
    int matches=tag==(unsigned)*expected||(tag=='s'&&!strcmp(expected,"Ljava/lang/String;"))||(tag=='c'&&!strcmp(expected,"Ljava/lang/Class;"))||((tag=='e'||tag=='@')&&*expected=='L');
    if(!matches){*r=start;annotation_skip_value(r,owner,depth);annotation_mismatch(v,member,tag);return result;}
    if(tag&&strchr("BCDFIJSZ",(int)tag)) {
        CP *p=cp(v,owner,readn(r,2),tag=='D'?6:tag=='F'?4:tag=='J'?5:3);
        result=val(p->bits,tag=='D'?DOUBLE:tag=='F'?FLOAT:tag=='J'?LONG:INT);
        if(tag=='B')result=iv((int8_t)integer(result));if(tag=='C')result=iv((uint16_t)integer(result));if(tag=='S')result=iv((int16_t)integer(result));if(tag=='Z')result=iv(!!integer(result));return result;
    }
    if(tag=='s')return rv(string(v,utf(v,owner,readn(r,2))));
    if(tag=='c'){Class *c=annotation_resolve(v,utf(v,owner,readn(r,2)),0);return rv(c?class_mirror(v,c):NULL);}
    if(tag=='e') {
        const char *desc=utf(v,owner,readn(r,2)),*name=utf(v,owner,readn(r,2));
        if(strcmp(desc,expected)){annotation_mismatch(v,member,tag);return result;}
        Class *type=annotation_resolve(v,desc,0);if(!type)return result;
        if(!enum_type(type)){annotation_mismatch(v,member,tag);return result;}
        initialize(v,type);if(v->exception)return result;
        for(unsigned i=0;i<type->nf;i++){Field *f=&type->fields[i];if((f->flags&0x4008)==0x4008&&!strcmp(f->name,name)&&!strcmp(f->desc,desc))return f->value;}
        Value args[]={rv(class_mirror(v,type)),rv(string(v,name))};v->exception=annotation_exception(v,"java/lang/EnumConstantNotPresentException","(Ljava/lang/Class;Ljava/lang/String;)V",args,2);return result;
    }
    if(tag=='@') {
        Reader peek=*r;const char *desc=utf(v,owner,readn(&peek,2));
        if(strcmp(desc,expected)){annotation_skip_body(r,owner,depth+1);annotation_mismatch(v,member,tag);return result;}
        Object *o=annotation_parse(r,owner,depth+1,0);return rv(o);
    }
    if(tag=='[') {
        unsigned count=readn(r,2);Class *type=annotation_resolve(v,expected,0);
        if(!type){while(count--)annotation_skip_value(r,owner,depth+1);return result;}
        Object *array=array_new(v,expected,(int32_t)count);root(v,array);Object *error=NULL;unsigned saved=v->nr;
        for(unsigned i=0;i<count;i++) {
            if(error){annotation_skip_value(r,owner,depth+1);continue;}
            array->data[i]=annotation_value(r,owner,expected+1,member,depth+1);
            if(v->exception){error=v->exception;root(v,error);v->exception=NULL;}
        }
        v->nr=saved-1;v->exception=error;return rv(array);
    }
    fail(v,"invalid annotation value tag %u",tag);return result;
}
static Object *annotation_parse(Reader *r,Class *owner,unsigned depth,int top) {
    VM *v=r->v;if(depth>64)fail(v,"annotation nesting limit exceeded");Reader start=*r;
    const char *desc=utf(v,owner,readn(r,2));Class *type=annotation_resolve(v,desc,top);
    if(!type||!(type->access&0x2000)||(top&&!annotation_runtime(v,type))) {
        *r=start;annotation_skip_body(r,owner,depth);return NULL;
    }
    Object *o=new_object(v,annotation_implementation(v,type),'@',(size_t)type->nm*2);o->represented=type;root(v,o);
    unsigned count=readn(r,2);unsigned char *seen=(unsigned char *)alloc(v,type->nm);
    while(count--) {
        const char *name=utf(v,owner,readn(r,2));unsigned i;
        for(i=0;i<type->nm;i++)if(annotation_member(&type->methods[i])&&!strcmp(type->methods[i].name,name))break;
        if(i==type->nm){annotation_skip_value(r,owner,depth+1);continue;}
        if(seen[i]){throwing(v,"java/lang/annotation/AnnotationFormatError");break;}seen[i]=1;
        o->data[i]=annotation_value(r,owner,type->methods[i].desc+2,&type->methods[i],depth+1);
        if(v->exception){o->data[type->nm+i]=rv(v->exception);v->exception=NULL;}
    }
    for(unsigned i=0;i<type->nm&&!v->exception;i++)if(!seen[i]&&annotation_member(&type->methods[i])&&type->methods[i].annotation_default.bytes) {
        Method *m=&type->methods[i];Reader d=annotation_reader(v,&m->annotation_default);
        o->data[i]=annotation_value(&d,type,m->desc+2,m,depth+1);
        if(d.pos!=d.n)fail(v,"invalid AnnotationDefault length");
        if(v->exception){o->data[type->nm+i]=rv(v->exception);v->exception=NULL;}
    }
    release(v,seen);v->nr--;return v->exception?NULL:o;
}
static Object *annotation_declared(VM *v,Class *owner,AnnotationData *data) {
    if(data->cache)return data->cache;
    Reader r=annotation_reader(v,data);unsigned count=data->bytes?readn(&r,2):0;
    Object *values=array_new(v,"[Ljava/lang/annotation/Annotation;",(int32_t)count);root(v,values);unsigned used=0;
    for(unsigned i=0;i<count&&!v->exception;i++) {
        Object *o=annotation_parse(&r,owner,0,1);if(!o)continue;
        for(unsigned j=0;j<used;j++)if(obj(values->data[j])->represented==o->represented){throwing(v,"java/lang/annotation/AnnotationFormatError");break;}
        if(!v->exception)values->data[used++]=rv(o);
    }
    if(!v->exception&&r.pos!=r.n)fail(v,"invalid RuntimeVisibleAnnotations length");
    if(!v->exception){Object *trim=array_new(v,"[Ljava/lang/annotation/Annotation;",(int32_t)used);memcpy(trim->data,values->data,used*sizeof(Value));data->cache=trim;}
    v->nr--;return v->exception?NULL:data->cache;
}
static Object *annotation_find(Object *array,Class *type) {
    if(array)for(size_t i=0;i<array->count;i++)if(obj(array->data[i])->represented==type)return obj(array->data[i]);return NULL;
}
static Value annotation_get(VM *v,Object *self,unsigned i,int clone) {
    Class *type=self->represented;
    if(obj(self->data[type->nm+i])){v->exception=obj(self->data[type->nm+i]);return iv(0);}
    Value value=self->data[i];
    if(!value.tag){Value args[]={rv(class_mirror(v,type)),rv(string(v,type->methods[i].name))};
        v->exception=annotation_exception(v,"java/lang/annotation/IncompleteAnnotationException","(Ljava/lang/Class;Ljava/lang/String;)V",args,2);return iv(0);}
    if(clone&&value.tag==REF&&obj(value)&&obj(value)->kind=='a') {
        Object *array=obj(value),*copy=array_new(v,array->array_desc,(int32_t)array->count);memcpy(copy->data,array->data,array->count*sizeof(Value));return rv(copy);
    }
    return value;
}
static uint32_t annotation_string_hash(const char *text) {uint32_t hash=0;const unsigned char *p=(const unsigned char *)text;while(*p)hash=31*hash+utf_unit(&p);return hash;}
static uint64_t annotation_number_bits(Value value) {
    if(value.tag==DOUBLE&&isnan(dbl(value)))return UINT64_C(0x7ff8000000000000);
    if(value.tag==FLOAT&&isnan(flt(value)))return UINT64_C(0x7fc00000);return value.bits;
}
static Value annotation_virtual(VM *v,Object *o,const char *name,const char *desc,Value *args,unsigned count) {
    Method *m=method(o->cls,name,desc);if(!m)m=default_method(v,o->cls,name,desc);
    if(v->exception)return iv(0);if(m)return execute(v,m,args,count);
    /* Arrays inherit Object's implementations; their builtin class has no
     * separately bound equals/hashCode/toString implementation. */
    Class *base=o->cls;while(base&&(!base->builtin||base->component))base=base->super;
    if(!base)fail(v,"annotation member implementation is missing: %s",name);
    return native_call(v,base,name,desc,args,count,0);
}
static uint32_t annotation_value_hash(VM *v,Value value,const char *desc) {
    if(*desc=='['){Object *o=obj(value);uint32_t hash=1;for(size_t i=0;i<o->count&&!v->exception;i++)hash=31*hash+annotation_value_hash(v,o->data[i],desc+1);return hash;}
    if(value.tag!=REF){uint64_t bits=annotation_number_bits(value);return *desc=='Z'?(integer(value)?1231:1237):(uint32_t)(bits^(wide(value)?bits>>32:0));}
    Object *o=obj(value);if(o->kind=='s')return annotation_string_hash(o->text);
    Value args[]={value};return (uint32_t)integer(annotation_virtual(v,o,"hashCode","()I",args,1));
}
static int annotation_values_equal(VM *v,Value a,Value b,const char *desc) {
    if(a.tag!=b.tag)return 0;if(a.tag!=REF)return annotation_number_bits(a)==annotation_number_bits(b);
    Object *x=obj(a),*y=obj(b);if(x==y)return 1;if(!x||!y)return 0;
    if(*desc=='['){if(y->kind!='a'||strcmp(x->array_desc,y->array_desc)||x->count!=y->count)return 0;
        for(size_t i=0;i<x->count;i++)if(!annotation_values_equal(v,x->data[i],y->data[i],desc+1)||v->exception)return 0;return 1;}
    if(x->kind=='s')return y->kind=='s'&&!strcmp(x->text,y->text);
    Value args[]={a,b};return integer(annotation_virtual(v,x,"equals","(Ljava/lang/Object;)Z",args,2));
}
static void annotation_text(VM *v,ConcatBuffer *b,Value value,const char *desc) {
    char text[128];const char *s=NULL;
    if(*desc=='['){Object *o=obj(value);concat_append(v,b,"{",1);for(size_t i=0;i<o->count&&!v->exception;i++){if(i)concat_append(v,b,", ",2);annotation_text(v,b,o->data[i],desc+1);}concat_append(v,b,"}",1);return;}
    if(value.tag==REF){Object *o=obj(value);
        if(o->kind=='s'){concat_append(v,b,"\"",1);const unsigned char *p=(const unsigned char *)o->text;while(*p){unsigned ch=utf_unit(&p);if(ch=='"'||ch=='\\'){text[0]='\\';text[1]=(char)ch;concat_append(v,b,text,2);}else if(ch<32){snprintf(text,sizeof text,"\\u%04x",ch);concat_append(v,b,text,6);}else {size_t n=write_unit(text,ch);concat_append(v,b,text,n);}}concat_append(v,b,"\"",1);return;}
        if(o->kind=='c'){Object *name=class_name_string(v,o->represented,0);concat_append(v,b,name->text,strlen(name->text));concat_append(v,b,".class",6);return;}
        Value call[]={value};Value result=annotation_virtual(v,o,"toString","()Ljava/lang/String;",call,1);if(v->exception)return;s=obj(result)->text;
    }else if(*desc=='Z')s=integer(value)?"true":"false";
    else if(*desc=='C'){snprintf(text,sizeof text,"'\\u%04x'",(uint16_t)integer(value));s=text;}
    else if(*desc=='F'||*desc=='D')fail(v,"annotation floating-point text formatting is not implemented");
    else s=as_text(v,value,text);
    concat_append(v,b,s,strlen(s));
}
static Value annotation_invoke(VM *v,Method *m,Value *args,unsigned count) {
    (void)count;Object *self=nonnull(v,args[0]);if(!self)return iv(0);Class *type=m->owner->annotation_type;
    if(self->kind!='@'||self->represented!=type)fail(v,"invalid annotation receiver");
    const char *name=m->name;
    if(!strcmp(name,"annotationType"))return rv(class_mirror(v,type));
    if(!strcmp(name,"equals")) {
        Object *other=obj(args[1]);if(other==self)return iv(1);if(!other||!subtype(other->cls,type))return iv(0);
        for(unsigned i=0;i<type->nm;i++)if(annotation_member(&type->methods[i])) {
            Value left=annotation_get(v,self,i,0);if(v->exception)return iv(0);
            Value call[]={rv(other)},right=annotation_virtual(v,other,type->methods[i].name,type->methods[i].desc,call,1);if(v->exception)return iv(0);
            if(right.tag==REF)root(v,obj(right));int equal=annotation_values_equal(v,left,right,type->methods[i].desc+2);if(right.tag==REF)v->nr--;
            if(!equal||v->exception)return iv(0);
        }return iv(1);
    }
    if(!strcmp(name,"hashCode")) {
        uint32_t hash=0;for(unsigned i=0;i<type->nm&&!v->exception;i++)if(annotation_member(&type->methods[i])) {
            Value value=annotation_get(v,self,i,0);if(v->exception)break;hash+=(127*annotation_string_hash(type->methods[i].name))^annotation_value_hash(v,value,type->methods[i].desc+2);
        }return iv((int32_t)hash);
    }
    if(!strcmp(name,"toString")) {
        ConcatBuffer b={0};concat_append(v,&b,"@",1);for(const char *p=type->name;*p;p++){char ch=*p=='/'?'.':*p;concat_append(v,&b,&ch,1);}concat_append(v,&b,"(",1);int any=0;
        for(unsigned i=0;i<type->nm&&!v->exception;i++)if(annotation_member(&type->methods[i])) {
            Value value=annotation_get(v,self,i,0);if(v->exception)break;if(any++)concat_append(v,&b,", ",2);
            concat_append(v,&b,type->methods[i].name,strlen(type->methods[i].name));concat_append(v,&b,"=",1);annotation_text(v,&b,value,type->methods[i].desc+2);
        }concat_append(v,&b,")",1);Object *result=v->exception?NULL:string(v,b.data);release(v,b.data);return rv(result);
    }
    for(unsigned i=0;i<type->nm;i++)if(!strcmp(type->methods[i].name,name)&&!strcmp(type->methods[i].desc,m->desc))return annotation_get(v,self,i,1);
    fail(v,"unknown generated annotation method");return iv(0);
}
static Object *annotation_single(VM *v,Class *owner,AnnotationData *data,Class *type,int inherited) {
    Object *values=annotation_declared(v,owner,data);if(v->exception)return NULL;
    Object *found=annotation_find(values,type);if(found||!inherited||!owner->super||(owner->access&0x200)||!annotation_inherited(v,type))return found;
    return annotation_single(v,owner->super,&owner->super->annotations,type,1);
}
static Object *annotation_all(VM *v,Class *owner,AnnotationData *data,int inherited) {
    Object *own=annotation_declared(v,owner,data);if(v->exception)return NULL;
    Object *parent=inherited&&owner->super&&!(owner->access&0x200)?annotation_all(v,owner->super,&owner->super->annotations,1):NULL;if(v->exception)return NULL;
    root(v,parent);size_t count=own->count;
    if(parent)for(size_t i=0;i<parent->count;i++){Class *type=obj(parent->data[i])->represented;if(annotation_inherited(v,type)&&!annotation_find(own,type))count++;}
    Object *result=array_new(v,"[Ljava/lang/annotation/Annotation;",(int32_t)count);size_t used=0;
    if(parent)for(size_t i=0;i<parent->count;i++){Class *type=obj(parent->data[i])->represented;if(annotation_inherited(v,type)&&!annotation_find(own,type))result->data[used++]=parent->data[i];}
    memcpy(result->data+used,own->data,own->count*sizeof(Value));v->nr--;return result;
}
static Object *annotation_by_type(VM *v,Class *owner,AnnotationData *data,Class *type,int inherited) {
    Object *own=annotation_declared(v,owner,data);if(v->exception)return NULL;
    Class *repeatable=load(v,"java/lang/annotation/Repeatable");Object *marker=annotation_single(v,type,&type->annotations,repeatable,0);if(v->exception)return NULL;
    Class *container=NULL;if(marker){Method *m=method(marker->cls,"value","()Ljava/lang/Class;");Value a=rv(marker);Value value=execute(v,m,&a,1);if(v->exception)return NULL;container=obj(value)->represented;}
    Object *items=NULL,*direct=annotation_find(own,type),*wrapped=container?annotation_find(own,container):NULL;
    if(wrapped){Method *m=NULL;for(unsigned i=0;i<wrapped->cls->nm;i++)if(!strcmp(wrapped->cls->methods[i].name,"value")){m=&wrapped->cls->methods[i];break;}
        if(!m){throwing(v,"java/lang/annotation/AnnotationFormatError");return NULL;}Value a=rv(wrapped);Value value=execute(v,m,&a,1);if(v->exception)return NULL;items=obj(value);
        if(!items||items->kind!='a'||items->cls->component!=type){throwing(v,"java/lang/annotation/AnnotationFormatError");return NULL;}}
    root(v,items);size_t count=(direct?1:0)+(items?items->count:0);
    if(!count&&inherited&&owner->super&&!(owner->access&0x200)&&annotation_inherited(v,type)){v->nr--;return annotation_by_type(v,owner->super,&owner->super->annotations,type,1);}
    char *desc=lambda_descriptor(v,type);char *array=(char *)alloc(v,strlen(desc)+2);array[0]='[';strcpy(array+1,desc);
    Object *result=array_new(v,array,(int32_t)count);release(v,array);release(v,desc);size_t used=0;
    /* A directly present annotation and its container can coexist. Preserve
     * their order in the classfile when expanding the container. */
    for(size_t i=0;i<own->count;i++){Object *o=obj(own->data[i]);if(o==direct)result->data[used++]=rv(o);else if(o==wrapped&&items){memcpy(result->data+used,items->data,items->count*sizeof(Value));used+=items->count;}}
    v->nr--;return result;
}
static Value annotation_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int stat,int *handled) {
    (void)c;*handled=0;if(stat)return iv(0);Object *self=obj(args[0]);Class *owner=NULL;AnnotationData *data=NULL;
    if(self->kind=='c'){owner=self->represented;data=&owner->annotations;}
    else if(self->kind=='f'){owner=self->represented;data=&self->represented_field->annotations;}
    else if((self->kind=='C'||self->kind=='M')&&self->represented_method){owner=self->represented;data=&self->represented_method->annotations;}
    if(!data)return iv(0);
    if(self->kind=='c'&&!strcmp(name,"isAnnotation")&&!strcmp(desc,"()Z")){*handled=1;return iv(!!(owner->access&0x2000));}
    int declared=!strncmp(name,"getDeclared",11),inherit=self->kind=='c'&&!declared;
    if((!strcmp(name,"getAnnotation")||!strcmp(name,"getDeclaredAnnotation")||!strcmp(name,"isAnnotationPresent"))&&
       (!strcmp(desc,"(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;")||!strcmp(desc,"(Ljava/lang/Class;)Z"))) {
        *handled=1;Object *type=nonnull(v,args[1]);if(!type)return iv(0);if(type->kind!='c')fail(v,"annotation type must be Class");
        Object *value=annotation_single(v,owner,data,type->represented,inherit);return !strcmp(name,"isAnnotationPresent")?iv(value!=NULL):rv(value);
    }
    if((!strcmp(name,"getAnnotations")||!strcmp(name,"getDeclaredAnnotations"))&&!strcmp(desc,"()[Ljava/lang/annotation/Annotation;")) {
        *handled=1;return rv(annotation_all(v,owner,data,inherit));
    }
    if((!strcmp(name,"getAnnotationsByType")||!strcmp(name,"getDeclaredAnnotationsByType"))&&!strcmp(desc,"(Ljava/lang/Class;)[Ljava/lang/annotation/Annotation;")) {
        *handled=1;Object *type=nonnull(v,args[1]);if(!type)return iv(0);if(type->kind!='c')fail(v,"annotation type must be Class");return rv(annotation_by_type(v,owner,data,type->represented,inherit));
    }
    if(self->kind=='M') {
        Method *m=self->represented_method;
        if(!strcmp(name,"getName")&&!strcmp(desc,"()Ljava/lang/String;")){*handled=1;return rv(string(v,m->name));}
        if(!strcmp(name,"getDeclaringClass")&&!strcmp(desc,"()Ljava/lang/Class;")){*handled=1;return rv(class_mirror(v,owner));}
    }
    return iv(0);
}
