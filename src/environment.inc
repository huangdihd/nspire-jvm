/* Native process environment lookup. Java strings are converted to UTF-8;
 * the existing UTF-8 reader handles returned bytes, including surrogate pairs. */
static Object *environment_get(VM *v,Object *key) {
    if(!key){throwing(v,"java/lang/NullPointerException");return NULL;}
    size_t size=strlen(key->text);char *name=(char *)alloc(v,size+1),*out=name;const unsigned char *p=(const unsigned char *)key->text;
    while(*p) {
        unsigned ch=utf_unit(&p);
        if(!ch||ch=='='){release(v,name);return NULL;}
        if(character_high(ch)&&*p){const unsigned char *next=p;unsigned low=utf_unit(&next);if(character_low(low)){ch=character_pair(ch,low);p=next;}}
        if(ch>=0xd800&&ch<=0xdfff)ch='?';
        if(ch>=0x10000){*out++=(char)(0xf0|(ch>>18));*out++=(char)(0x80|((ch>>12)&63));*out++=(char)(0x80|((ch>>6)&63));*out++=(char)(0x80|(ch&63));}
        else out+=write_unit(out,ch);
    }
    *out=0;const char *raw=getenv(name);release(v,name);if(!raw)return NULL;
    size_t length=strlen(raw);if(length>(META_LIMIT-1)/3)fail(v,"environment value exceeds metadata limit");
    Object *stream=new_object(v,load(v,"java/io/ByteArrayInputStream"),'b',0);root(v,stream);stream_buffer(v,stream,length);memcpy(stream->buffer,raw,length);
    Object *reader=new_object(v,load(v,"java/io/InputStreamReader"),'r',1);root(v,reader);reader->data[0]=rv(stream);reader->pending=-1;
    char *text=(char *)alloc(v,length*3+1);out=text;int ch;
    while(!v->exception&&(ch=reader_read(v,reader))>=0)out+=write_unit(out,(unsigned)ch);
    *out=0;Object *result=v->exception?NULL:string(v,text);release(v,text);v->nr-=2;return result;
}
