/* PrintStream adapter. Other output streams execute their OpenJDK bytecode.
 * UTF-8 is the supported text encoding. The console borrows the host/SDK FILE
 * handles: Java close ends that Java stream, without closing the VM's own UI. */
enum { OUTPUT_ERROR=1,OUTPUT_AUTOFLUSH=2,OUTPUT_CLOSING=4 };
static void printstream_class(VM *v,Class *c) {
    static const char *spec[]={
        "<init>","(Ljava/io/OutputStream;)V","<init>","(Ljava/io/OutputStream;Z)V","<init>","(Ljava/io/OutputStream;ZLjava/lang/String;)V",
        "write","(I)V","write","([BII)V","flush","()V","close","()V","checkError","()Z","setError","()V","clearError","()V",
        "print","(Z)V","print","(C)V","print","(I)V","print","(J)V","print","(F)V","print","(D)V","print","([C)V","print","(Ljava/lang/String;)V","print","(Ljava/lang/Object;)V",
        "println","()V","println","(Z)V","println","(C)V","println","(I)V","println","(J)V","println","(F)V","println","(D)V","println","([C)V","println","(Ljava/lang/String;)V","println","(Ljava/lang/Object;)V",
        "append","(C)Ljava/io/PrintStream;","append","(Ljava/lang/CharSequence;)Ljava/io/PrintStream;","append","(Ljava/lang/CharSequence;II)Ljava/io/PrintStream;",
        "append","(C)Ljava/lang/Appendable;","append","(Ljava/lang/CharSequence;)Ljava/lang/Appendable;","append","(Ljava/lang/CharSequence;II)Ljava/lang/Appendable;",
        "format","(Ljava/lang/String;[Ljava/lang/Object;)Ljava/io/PrintStream;","printf","(Ljava/lang/String;[Ljava/lang/Object;)Ljava/io/PrintStream;"
    };
    c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,"java/lang/Appendable");
    c->nm=(uint16_t)(sizeof spec/sizeof spec[0]/2);c->methods=(Method *)alloc(v,c->nm*sizeof(Method));
    for(unsigned i=0;i<c->nm;i++) {
        Method *m=&c->methods[i];m->owner=c;m->name=(char *)spec[2*i];m->desc=(char *)spec[2*i+1];m->flags=NATIVE|1;
        if(!strcmp(m->name,"setError")||!strcmp(m->name,"clearError"))m->flags=NATIVE|4;
    }
}
static Value output_invoke(VM *v,Object *out,const char *name,const char *desc,Value *args,unsigned count) {
    if(!out){throwing(v,"java/lang/NullPointerException");return iv(0);}
    Method *m=method(out->cls,name,desc);if(!m)fail(v,"output method missing: %s.%s%s",out->cls->name,name,desc);
    return execute(v,m,args,count);
}
static void output_catch(VM *v,Object *self,int writing) {
    if(!v->exception)return;
    if(!named_type(v->exception->cls,"java/io/IOException"))return;
    if(writing&&named_type(v->exception->cls,"java/io/InterruptedIOException"))v->current->interrupted=1;
    else self->output_flags|=OUTPUT_ERROR;
    v->exception=NULL;
}
static int output_open(Object *self) {
    if(self->closed||(!self->output_fd&&!obj(self->data[0]))){self->output_flags|=OUTPUT_ERROR;return 0;}return 1;
}
static void output_flush(VM *v,Object *self) {
    if(!output_open(self))return;
    if(self->output_fd){if(fflush(self->output_fd==1?stdout:stderr))self->output_flags|=OUTPUT_ERROR;}
    else {Value a=self->data[0];output_invoke(v,obj(a),"flush","()V",&a,1);}
}
static void output_bytes(VM *v,Object *self,Object *bytes,int32_t offset,int32_t length) {
    if(!output_open(self))return;
    if(!bytes){throwing(v,"java/lang/NullPointerException");return;}
    if(bytes->kind!='a'||strcmp(bytes->array_desc,"[B"))fail(v,"PrintStream.write requires byte[]");
    if(offset<0||length<0||(size_t)offset>bytes->count||(size_t)length>bytes->count-(size_t)offset){throwing(v,"java/lang/IndexOutOfBoundsException");return;}
    if(self->output_fd) {
        FILE *out=self->output_fd==1?stdout:stderr;unsigned char chunk[512];
        for(int32_t i=0;i<length;) {int32_t n=length-i;if(n>(int32_t)sizeof chunk)n=(int32_t)sizeof chunk;
            for(int32_t j=0;j<n;j++)chunk[j]=(unsigned char)integer(bytes->data[offset+i+j]);
            if(fwrite(chunk,1,(size_t)n,out)!=(size_t)n){self->output_flags|=OUTPUT_ERROR;break;}i+=n;}
    } else {Value a[]={self->data[0],rv(bytes),iv(offset),iv(length)};output_invoke(v,obj(a[0]),"write","([BII)V",a,4);}
    if(!v->exception&&(self->output_flags&OUTPUT_AUTOFLUSH))output_flush(v,self);
}
static void output_text_byte(VM *v,ConcatBuffer *buffer,unsigned point) {
    char bytes[4];size_t n;
    if(point<128){bytes[0]=(char)point;n=1;}
    else if(point<2048){bytes[0]=(char)(0xc0|(point>>6));bytes[1]=(char)(0x80|(point&63));n=2;}
    else if(point<65536){bytes[0]=(char)(0xe0|(point>>12));bytes[1]=(char)(0x80|((point>>6)&63));bytes[2]=(char)(0x80|(point&63));n=3;}
    else {bytes[0]=(char)(0xf0|(point>>18));bytes[1]=(char)(0x80|((point>>12)&63));bytes[2]=(char)(0x80|((point>>6)&63));bytes[3]=(char)(0x80|(point&63));n=4;}
    concat_append(v,buffer,bytes,n);
}
static void output_text(VM *v,Object *self,const char *text,int char_array) {
    if(!output_open(self))return;
    ConcatBuffer buffer={0};const unsigned char *p=(const unsigned char *)text;int newline=0;
    while(*p) {
        unsigned unit=utf_unit(&p);if(unit=='\n')newline=char_array?newline+1:1;
        if(self->output_pending) {
            unsigned high=self->output_pending;self->output_pending=0;
            if(character_low(unit)){output_text_byte(v,&buffer,character_pair(high,unit));continue;}
            output_text_byte(v,&buffer,'?');
        }
        if(character_high(unit))self->output_pending=unit;
        else output_text_byte(v,&buffer,character_low(unit)?'?':unit);
    }
    for(size_t i=0;i<buffer.size&&!v->exception;) {
        size_t size=buffer.size-i;if(size>1024)size=1024;
        Object *bytes=array_new(v,"[B",(int32_t)size);root(v,bytes);
        for(size_t j=0;j<size;j++)bytes->data[j]=iv((int8_t)buffer.data[i+j]);
        Value a[]={rv(self),rv(bytes),iv(0),iv((int32_t)size)};output_invoke(v,self,"write","([BII)V",a,4);v->nr--;i+=size;
    }
    release(v,buffer.data);
    if(self->output_flags&OUTPUT_AUTOFLUSH)for(int i=0;i<newline&&!v->exception;i++)output_flush(v,self);
    output_catch(v,self,1);
}
static Object *output_chars_string(VM *v,Object *chars) {
    if(chars->kind!='a'||strcmp(chars->array_desc,"[C"))fail(v,"PrintStream.print requires char[]");
    if(chars->count>(META_LIMIT-1)/3)fail(v,"printed character array exceeds metadata limit");
    char *text=(char *)alloc(v,chars->count*3+1),*end=text;
    for(size_t i=0;i<chars->count;i++)end+=write_unit(end,(uint16_t)integer(chars->data[i]));
    *end=0;Object *result=string(v,text);release(v,text);return result;
}
static Value printstream_native(VM *v,Object *self,const char *name,const char *desc,Value *args,unsigned count) {
    Value result=iv(0);
    Class *base=load(v,"java/io/PrintStream");int supported=0;
    for(unsigned i=0;i<base->nm;i++)if(!strcmp(base->methods[i].name,name)&&!strcmp(base->methods[i].desc,desc)){supported=1;break;}
    if(!supported)fail(v,"PrintStream method is not implemented: %s%s",name,desc);
    if(!strcmp(name,"<init>")) {
        Object *out=nonnull(v,args[1]);if(!out)return result;
        if(count==4){Object *encoding=nonnull(v,args[3]);if(!encoding)return result;
            const unsigned char *e=(const unsigned char *)encoding->text;
            if(case_compare(e,(const unsigned char *)"UTF-8")&&case_compare(e,(const unsigned char *)"UTF8")&&case_compare(e,(const unsigned char *)"unicode-1-1-utf-8")){throwing(v,"java/io/UnsupportedEncodingException");return result;}}
        self->kind='P';self->data[0]=rv(out);self->output_flags=count>=3&&integer(args[2])?OUTPUT_AUTOFLUSH:0;return result;
    }
    /* Object conversion may run user code; preserve its result across writes,
     * which may in turn collect, yield, throw or reenter this stream. */
    Object *text=NULL;unsigned saved=v->nr;int printing=!strcmp(name,"print")||!strcmp(name,"println"),appending=!strcmp(name,"append");
    if(printing&&count==2) {
        char value[128];
        if(desc[1]=='['){Object *chars=obj(args[1]);if(chars)text=output_chars_string(v,chars);}
        else if(desc[1]=='C'){size_t n=write_unit(value,(uint16_t)integer(args[1]));value[n]=0;text=string(v,value);}
        else if(desc[1]=='Z')text=string(v,integer(args[1])?"true":"false");
        else if(!strcmp(desc,"(Ljava/lang/Object;)V")){Value val=object_string(v,args[1]);if(!v->exception)text=obj(val);}
        else text=string(v,as_text(v,args[1],value));
    } else if(appending) {
        if(desc[1]=='C'){char value[4];size_t n=write_unit(value,(uint16_t)integer(args[1]));value[n]=0;text=string(v,value);}
        else {
            Object *seq=obj(args[1]);if(!seq){seq=string(v,"null");root(v,seq);}
            if(count==4){Value a[]={rv(seq),args[2],args[3]};Method *m=method(seq->cls,"subSequence","(II)Ljava/lang/CharSequence;");Value s=m?execute(v,m,a,3):native_call(v,seq->cls,"subSequence","(II)Ljava/lang/CharSequence;",a,3,0);if(!v->exception){seq=obj(s);root(v,seq);}}
            if(!v->exception){Value s=object_string(v,rv(seq));if(!v->exception)text=obj(s);}
        }
        result=rv(self);
    } else if(!strcmp(name,"format")||!strcmp(name,"printf")){text=format_string(v,obj(args[1]),obj(args[2]));result=rv(self);}
    if(v->exception){v->nr=saved;return result;}
    if(text)root(v,text);
    if(!monitor_enter(v,self)){v->nr=saved;return result;}
    if(printing||appending||text) {
        if(text)output_text(v,self,text->text,printing&&desc[1]=='[');else if(count>1&&output_open(self))throwing(v,"java/lang/NullPointerException");
        if(!v->exception&&!strcmp(name,"println"))output_text(v,self,"\n",0);
    } else if(!strcmp(name,"write")) {
        if(count==2) {
            if(output_open(self)) {
                if(self->output_fd){if(fputc(integer(args[1])&255,self->output_fd==1?stdout:stderr)==EOF)self->output_flags|=OUTPUT_ERROR;}
                else {Value a[]={self->data[0],args[1]};output_invoke(v,obj(a[0]),"write","(I)V",a,2);}
                if(!v->exception&&integer(args[1])=='\n'&&(self->output_flags&OUTPUT_AUTOFLUSH))output_flush(v,self);
            }
        } else output_bytes(v,self,obj(args[1]),integer(args[2]),integer(args[3]));
        output_catch(v,self,1);
    } else if(!strcmp(name,"flush")){output_flush(v,self);output_catch(v,self,0);}
    else if(!strcmp(name,"close")) {
        if(!(self->output_flags&OUTPUT_CLOSING)) {
            self->output_flags|=OUTPUT_CLOSING;
            if(self->output_pending){self->output_pending=0;output_text(v,self,"?",0);}
            if(!v->exception)output_flush(v,self);
            output_catch(v,self,0);
            if(!v->exception&&!self->output_fd){Value a=self->data[0];output_invoke(v,obj(a),"close","()V",&a,1);output_catch(v,self,0);}
            self->closed=1;self->data[0]=rv(NULL);
        }
    } else if(!strcmp(name,"checkError")) {
        if(!self->closed){output_flush(v,self);output_catch(v,self,0);}
        Object *out=obj(self->data[0]);
        if(!v->exception&&out&&named_type(out->cls,"java/io/PrintStream")){Value a=rv(out);result=output_invoke(v,out,"checkError","()Z",&a,1);}
        else result=iv(!!(self->output_flags&OUTPUT_ERROR));
    } else if(!strcmp(name,"setError"))self->output_flags|=OUTPUT_ERROR;
    else if(!strcmp(name,"clearError"))self->output_flags&=~OUTPUT_ERROR;
    else fail(v,"PrintStream method is not implemented: %s%s",name,desc);
    monitor_exit(v,self);v->nr=saved;return result;
}
