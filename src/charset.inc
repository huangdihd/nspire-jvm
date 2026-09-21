/* Native Strings delegate to the real Java charset/coder/buffer implementation. */
static Value charset_native(VM *v,Class *c,const char *name,const char *desc,Value *args,unsigned count,int stat,int *handled) {
    *handled=0;if(stat||strcmp(c->name,"java/lang/String"))return iv(0);
    const char *signature=NULL,*operation=NULL;Value call[4];unsigned n=0;int ctor=0;
    if(!strcmp(name,"getBytes")) {
        if(!strcmp(desc,"()[B"))signature="(Ljava/lang/String;)[B";
        else if(!strcmp(desc,"(Ljava/lang/String;)[B"))signature="(Ljava/lang/String;Ljava/lang/String;)[B";
        else if(!strcmp(desc,"(Ljava/nio/charset/Charset;)[B"))signature="(Ljava/lang/String;Ljava/nio/charset/Charset;)[B";
        if(signature){operation="encode";n=count;memcpy(call,args,n*sizeof(Value));}
    } else if(!strcmp(name,"<init>")) {
        int slice=0,extra=0;
        if(!strcmp(desc,"([B)V")){}
        else if(!strcmp(desc,"([BII)V"))slice=1;
        else if(!strcmp(desc,"([BLjava/lang/String;)V"))extra=1;
        else if(!strcmp(desc,"([BLjava/nio/charset/Charset;)V"))extra=2;
        else if(!strcmp(desc,"([BIILjava/lang/String;)V")){slice=1;extra=1;}
        else if(!strcmp(desc,"([BIILjava/nio/charset/Charset;)V")){slice=1;extra=2;}
        else return iv(0);
        *handled=1;Object *bytes=nonnull(v,args[1]);if(!bytes)return iv(0);
        if(bytes->kind!='a'||strcmp(bytes->array_desc,"[B"))fail(v,"String decoding requires byte[]");
        signature=extra==1?"([BIILjava/lang/String;)Ljava/lang/String;":extra==2?"([BIILjava/nio/charset/Charset;)Ljava/lang/String;":"([BII)Ljava/lang/String;";
        call[0]=args[1];call[1]=slice?args[2]:iv(0);call[2]=slice?args[3]:iv((int32_t)bytes->count);n=3;
        if(extra)call[n++]=args[slice?4:2];operation="decode";ctor=1;
    }
    if(!signature)return iv(0);
    *handled=1;Class *bridge=load(v,"nspire/charset/StringCoding");initialize(v,bridge);if(v->exception)return iv(0);
    Method *m=method(bridge,operation,signature);if(!m)fail(v,"missing StringCoding bridge");
    Value result=execute(v,m,call,n);
    if(ctor&&!v->exception){Object *converted=nonnull(v,result);if(!converted)return iv(0);root(v,converted);obj(args[0])->kind='s';set_text(v,obj(args[0]),converted->text);v->nr--;return iv(0);}
    return result;
}
