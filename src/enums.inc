/* Enum metadata and the OpenJDK shared enum-universe service. Values come from
 * the actual compiled values() method; public callers receive defensive copies. */
static int enum_type(Class *c) {return (c->access&0x4000)&&c->super&&!strcmp(c->super->name,"java/lang/Enum");}
static Object *enum_constants(VM *v,Class *c) {
    if(c->enum_constants)return c->enum_constants;
    if(!enum_type(c))return NULL;
    initialize(v,c);if(v->exception)return NULL;
    size_t len=strlen(c->name);char *desc=(char *)alloc(v,len+6);snprintf(desc,len+6,"()[L%s;",c->name);
    Method *values=method(c,"values",desc);release(v,desc);
    if(!values||!(values->flags&STATIC))return NULL;
    Value result=execute(v,values,NULL,0);
    /* OpenJDK treats an invocation failure (e.g. values queried while $VALUES
     * is still null during initialization) as an unavailable enum universe. */
    if(v->exception){v->exception=NULL;return NULL;}
    Object *array=obj(result);if(!array)return NULL;
    if(array->kind!='a'||array->cls->component!=c)fail(v,"invalid enum values() result");
    c->enum_constants=array;return array;
}
static Object *enum_directory(VM *v,Class *c) {
    if(c->enum_directory)return c->enum_directory;
    Object *values=enum_constants(v,c);if(v->exception)return NULL;
    if(!values){throwing(v,"java/lang/IllegalArgumentException");return NULL;}
    Class *mapclass=load(v,"java/util/HashMap");initialize(v,mapclass);if(v->exception)return NULL;
    Object *map=new_object(v,mapclass,'o',mapclass->slots);root(v,map);
    Value init=rv(map);Method *ctor=method(mapclass,"<init>","()V");if(!ctor)fail(v,"HashMap constructor is missing");
    execute(v,ctor,&init,1);
    Method *name=method(c,"name","()Ljava/lang/String;"),*put=method(mapclass,"put","(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if(!name||!put)fail(v,"enum directory methods are missing");
    for(size_t i=0;i<values->count&&!v->exception;i++) {
        Value key=execute(v,name,&values->data[i],1);if(v->exception)break;
        Value args[]={rv(map),key,values->data[i]};execute(v,put,args,3);
    }
    if(!v->exception)c->enum_directory=map;v->nr--;return v->exception?NULL:map;
}
static Object *canonical_name(VM *v,Class *c) {
    if(c->local_class||(c->simple_name&&!*c->simple_name))return NULL;
    if(c->component||c->declaring_name) {
        Object *prefix=canonical_name(v,c->component?c->component:load(v,c->declaring_name));
        if(!prefix)return NULL;root(v,prefix);
        const char *suffix=c->component?"[]":c->simple_name;size_t length=strlen(prefix->text)+strlen(suffix)+2;
        char *text=(char *)alloc(v,length);snprintf(text,length,"%s%s%s",prefix->text,c->component?"":".",suffix);
        Object *result=string(v,text);release(v,text);v->nr--;return result;
    }
    return class_name_string(v,c,0);
}
