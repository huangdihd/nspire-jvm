/* The CharSequence append path calls Java length/charAt overrides. Conversion
 * through toString would change observable behavior for custom sequences. */
static Value sequence_call(VM *v,Object *source,const char *name,const char *desc,Value *args,unsigned count) {
    Method *m=method(source->cls,name,desc);if(!m)m=default_method(v,source->cls,name,desc);
    if(v->exception)return iv(0);
    return m?execute(v,m,args,count):native_call(v,source->cls,name,desc,args,count,0);
}
static void builder_sequence(VM *v,Object *builder,Object *source,int32_t start,int32_t end,int entire) {
    if(!source)source=string(v,"null");root(v,source);
    Value args[]={rv(source),iv(0)};int32_t length=integer(sequence_call(v,source,"length","()I",args,1));
    if(v->exception){v->nr--;return;}
    if(entire){start=0;end=length;}
    if(start<0||end<start||end>length){v->nr--;throwing(v,"java/lang/IndexOutOfBoundsException");return;}
    size_t prefix=builder->text?strlen(builder->text):0,size=(size_t)(end-start);
    if(prefix>=META_LIMIT||size>(META_LIMIT-prefix-1)/3)fail(v,"StringBuilder append exceeds metadata limit");
    char *text=(char *)alloc(v,prefix+size*3+1),*out=text+prefix;if(prefix)memcpy(text,builder->text,prefix);
    for(int32_t i=start;i<end&&!v->exception;i++) {
        args[1]=iv(i);Value ch=sequence_call(v,source,"charAt","(I)C",args,2);
        if(!v->exception)out+=write_unit(out,(uint16_t)integer(ch));
    }
    if(!v->exception){*out=0;set_text(v,builder,text);}release(v,text);v->nr--;
}
static void builder_length(VM *v,Object *builder,int32_t length) {
    if(length<0){throwing(v,"java/lang/StringIndexOutOfBoundsException");return;}
    const unsigned char *start=(const unsigned char *)(builder->text?builder->text:""),*p=start;int32_t count=0;
    while(*p&&count<length){utf_unit(&p);count++;}
    size_t prefix=(size_t)(p-start),zeros=(size_t)(length-count);
    if(prefix>=META_LIMIT||zeros>(META_LIMIT-prefix-1)/2)fail(v,"StringBuilder length exceeds metadata limit");
    char *text=(char *)alloc(v,prefix+zeros*2+1),*out=text+prefix;memcpy(text,start,prefix);
    while(zeros--)out+=write_unit(out,0);*out=0;set_text(v,builder,text);release(v,text);
}
