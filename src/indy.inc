/* Intrinsic implementation of the standard StringConcatFactory bootstrap.
 * Other bootstraps, including LambdaMetafactory, still fail explicitly.
 */
typedef struct { char *data;size_t size,capacity; } ConcatBuffer;
static void concat_append(VM *v,ConcatBuffer *b,const char *text,size_t len) {
    if(len>META_LIMIT-b->size-1)fail(v,"concatenated string exceeds metadata limit");
    size_t needed=b->size+len+1;
    if(needed>b->capacity) {
        size_t cap=b->capacity?b->capacity:128;
        while(cap<needed){if(cap>META_LIMIT/2){cap=needed;break;}cap*=2;}
        char *next=(char *)alloc(v,cap);if(b->size)memcpy(next,b->data,b->size);release(v,b->data);b->data=next;b->capacity=cap;
    }
    memcpy(b->data+b->size,text,len);b->size+=len;b->data[b->size]=0;
}
static void concat_value(VM *v,ConcatBuffer *b,Value value,char type) {
    char text[128];const char *s;
    if(type=='Z')s=integer(value)?"true":"false";
    else if(type=='C'){size_t n=write_unit(text,(uint16_t)integer(value));text[n]=0;s=text;}
    else if(type=='F'||type=='D')fail(v,"floating-point formatting for invokedynamic concatenation is not implemented");
    else if(value.tag==REF&&obj(value)) {
        Object *o=obj(value);Method *to_string=method(o->cls,"toString","()Ljava/lang/String;");
        if(to_string) {
            Value result=execute(v,to_string,&value,1);if(v->exception)return;
            if(result.tag!=REF||(obj(result)&&obj(result)->kind!='s'))fail(v,"toString did not return String");
            s=obj(result)?obj(result)->text:"null";
        } else s=as_text(v,value,text);
    } else s=as_text(v,value,text);
    concat_append(v,b,s,strlen(s));
}
static Value invoke_dynamic(VM *v,Class *owner,CP *dynamic,const char *desc,Value *args,unsigned count) {
    if(dynamic->a>=owner->nbootstraps)fail(v,"invalid invokedynamic bootstrap index");
    Bootstrap *bootstrap=&owner->bootstraps[dynamic->a];CP *handle=cp(v,owner,bootstrap->handle,15);
    if(handle->a!=6)fail(v,"invokedynamic bootstrap handle kind is not implemented");
    CP *ref=cp(v,owner,handle->b,10),*nt=cp(v,owner,ref->b,12);
    const char *cl=classname(v,owner,ref->a),*name=utf(v,owner,nt->a);
    if(strcmp(cl,"java/lang/invoke/StringConcatFactory")||(strcmp(name,"makeConcat")&&strcmp(name,"makeConcatWithConstants")))
        fail(v,"invokedynamic bootstrap not implemented: %s.%s",cl,name);
    const char *result=strchr(desc,')');
    if(!result||strcmp(result+1,"Ljava/lang/String;"))fail(v,"invalid StringConcatFactory return type");
    char types[256];if(count>sizeof types)fail(v,"too many concatenation arguments");
    const char *p=desc+1;
    for(unsigned i=0;i<count;i++) {
        types[i]=*p;while(*p=='[')p++;
        if(*p=='L'){p=strchr(p,';');if(!p)fail(v,"invalid concatenation descriptor");}p++;
    }
    ConcatBuffer buffer={0};unsigned arg=0,constant_arg=1;
    if(!strcmp(name,"makeConcat")) {
        if(bootstrap->nargs)fail(v,"unexpected makeConcat bootstrap arguments");
        for(;arg<count&&!v->exception;arg++)concat_value(v,&buffer,args[arg],types[arg]);
    } else {
        if(!bootstrap->nargs)fail(v,"missing concatenation recipe");
        CP *recipe_constant=cp(v,owner,bootstrap->args[0],8);
        const unsigned char *recipe=(const unsigned char *)utf(v,owner,recipe_constant->a);
        for(;*recipe&&!v->exception;recipe++) {
            if(*recipe==1) {
                if(arg==count)fail(v,"too few concatenation arguments");
                concat_value(v,&buffer,args[arg],types[arg]);arg++;
            } else if(*recipe==2) {
                if(constant_arg==bootstrap->nargs)fail(v,"too few concatenation constants");
                Value c=constant(v,owner,bootstrap->args[constant_arg++]);
                concat_value(v,&buffer,c,c.tag==FLOAT?'F':c.tag==DOUBLE?'D':c.tag==LONG?'J':c.tag==INT?'I':'L');
            } else concat_append(v,&buffer,(const char *)recipe,1);
        }
        if(!v->exception&&(arg!=count||constant_arg!=bootstrap->nargs))fail(v,"unused concatenation arguments");
    }
    Object *out=v->exception?NULL:string(v,buffer.data?buffer.data:"");release(v,buffer.data);return rv(out);
}
