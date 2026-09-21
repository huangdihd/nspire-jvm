/* Built-in bootstrap/application loaders and read-only classpath resources.
 * User-defined loader namespaces/defineClass remain explicitly unsupported.
 */
static Object *system_loader(VM *v) {
    if(!v->app_loader)v->app_loader=new_object(v,load(v,"java/lang/ClassLoader"),'L',0);
    return v->app_loader;
}
static int loader_scope(VM *v,Object *o) {
    if(!o)return 0;
    if(o!=system_loader(v))fail(v,"custom ClassLoader dispatch is not implemented");
    return 1;
}
static Class *named_class(VM *v,Object *s,int app,int init) {
    if(!s){throwing(v,"java/lang/NullPointerException");return NULL;}
    if(s->kind!='s')fail(v,"class name must be String");
    char *name=copy(v,s->text);int valid=strchr(name,'/')==NULL;
    for(char *p=name;*p;p++)if(*p=='.')*p='/';
    /* Restrict the lookup itself, including array component probing, so a
     * bootstrap lookup cannot see an already loaded application class. */
    unsigned paths=v->npaths;if(!app)v->npaths=v->nbootpaths;
    int exists=valid&&class_available(v,name);v->npaths=paths;
    if(!exists){release(v,name);throwing(v,"java/lang/ClassNotFoundException");return NULL;}
    Class *c=load(v,name);release(v,name);
    if(!app&&c->app_loader){throwing(v,"java/lang/ClassNotFoundException");return NULL;}
    if(init)initialize(v,c);return v->exception?NULL:c;
}
static Object *reflect_new(VM *v,Class *c) {
    if(c->primitive||c->component||(c->access&0x600)){throwing(v,"java/lang/InstantiationException");return NULL;}
    Method *ctor=method(c,"<init>","()V");
    if((!ctor||ctor->owner!=c)&&strcmp(c->name,"java/lang/Object")){throwing(v,"java/lang/InstantiationException");return NULL;}
    if(!(c->access&1)||(ctor&&!(ctor->flags&1))){throwing(v,"java/lang/IllegalAccessException");return NULL;}
    initialize(v,c);if(v->exception)return NULL;
    Object *o=new_object(v,c,'o',c->slots);root(v,o);
    for(Class *k=c;k;k=k->super)for(unsigned i=0;i<k->nf;i++)if(!(k->fields[i].flags&STATIC))o->data[k->fields[i].slot]=zero(k->fields[i].desc);
    Value arg=rv(o);if(ctor)execute(v,ctor,&arg,1);v->nr--;return v->exception?NULL:o;
}
static int resource_valid(const char *name) {
    if(!*name||*name=='/'||strchr(name,'\\')||strchr(name,':'))return 0;
    const char *p=name;
    while(*p){const char *end=strchr(p,'/');size_t n=end?(size_t)(end-p):strlen(p);
        if(!n||(n==1&&*p=='.')||(n==2&&p[0]=='.'&&p[1]=='.'))return 0;
        if(!end)break;p=end+1;
    }
    return 1;
}
static int resource_find(VM *v,unsigned path,const char *name) {
    ClassPath *entry=&v->paths[path];
    if(entry->is_zip)return mz_zip_reader_locate_file(&entry->zip,name,NULL,0);
    char file[2048];struct stat st;
    if(snprintf(file,sizeof file,"%s/%s",entry->path,name)>=(int)sizeof file)fail(v,"resource path too long");
    return !stat(file,&st)&&S_ISREG(st.st_mode)?0:-1;
}
static char *url_escape(VM *v,const char *s) {
    const char *hex="0123456789ABCDEF";char *out=(char *)alloc(v,strlen(s)*3+1),*q=out;
    for(const unsigned char *p=(const unsigned char *)s;*p;p++) {
        unsigned c=*p;
        if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||strchr("/-._~",c))*q++=(char)c;
        else{*q++='%';*q++=hex[c>>4];*q++=hex[c&15];}
    }
    *q=0;return out;
}
static Object *resource_url(VM *v,unsigned path,int index,const char *name) {
    ClassPath *entry=&v->paths[path];char absolute[2048],cwd[1024];
    if(entry->path[0]=='/')snprintf(absolute,sizeof absolute,"%s",entry->path);
    else {if(!getcwd(cwd,sizeof cwd))fail(v,"cannot resolve classpath working directory");
        if(snprintf(absolute,sizeof absolute,"%s/%s",cwd,entry->path)>=(int)sizeof absolute)fail(v,"resource path too long");}
    char *base=url_escape(v,absolute),*tail=url_escape(v,name);
    size_t n=strlen(base)+strlen(tail)+16;char *full=(char *)alloc(v,n);
    snprintf(full,n,entry->is_zip?"jar:file:%s!/%s":"file:%s/%s",base,tail);
    Object *o=new_object(v,load(v,"java/net/URL"),'u',0);root(v,o);set_text(v,o,full);
    size_t len=strlen(name)+1;heap_room(v,len);o->resource_name=(char *)malloc(len);
    if(!o->resource_name)fail(v,"out of native memory");memcpy(o->resource_name,name,len);
    o->bytes+=len;v->heap+=len;o->resource_path=(int)path;o->resource_index=index;
    v->nr--;release(v,base);release(v,tail);release(v,full);return o;
}
static Object *resources(VM *v,const char *name,int app,int all) {
    Object *result=NULL;unsigned end=app?v->npaths:v->nbootpaths;
    if(all){result=new_object(v,load(v,"nspire/ResourceEnumeration"),'e',end);root(v,result);}
    if(resource_valid(name))for(unsigned i=0;i<end;i++) {
        int index=resource_find(v,i,name);if(index<0)continue;
        Object *url=resource_url(v,i,index,name);if(!all)return url;
        result->data[result->cursor++]=rv(url);
    }
    if(all){result->buffer_size=result->cursor;result->cursor=0;v->nr--;}
    return result;
}
static void stream_buffer(VM *v,Object *o,size_t n) {
    root(v,o);heap_room(v,n);v->nr--;
    unsigned char *data=(unsigned char *)malloc(n?n:1);if(!data)fail(v,"out of native memory");
    free(o->buffer);o->bytes-=o->buffer_size;v->heap-=o->buffer_size;
    o->buffer=data;o->buffer_size=n;o->bytes+=n;v->heap+=n;o->cursor=o->mark_cursor=0;
}
static Object *resource_stream(VM *v,Object *url) {
    if(!url||url->kind!='u'||!url->resource_name)fail(v,"unsupported URL stream");
    root(v,url);ClassPath *entry=&v->paths[url->resource_path];size_t len;
    char file[2048];mz_zip_archive_file_stat st;struct stat info;
    if(entry->is_zip) {
        if(!mz_zip_reader_file_stat(&entry->zip,(mz_uint)url->resource_index,&st)||st.m_uncomp_size>8U*1024U*1024U)fail(v,"invalid/oversized resource");
        len=(size_t)st.m_uncomp_size;
    } else {
        if(snprintf(file,sizeof file,"%s/%s",entry->path,url->resource_name)>=(int)sizeof file)fail(v,"resource path too long");
        if(stat(file,&info)||info.st_size<0||(uint64_t)info.st_size>8U*1024U*1024U){v->nr--;throwing(v,"java/io/FileNotFoundException");return NULL;}
        len=(size_t)info.st_size;
    }
    Object *o=new_object(v,load(v,"nspire/ResourceInputStream"),'j',1);root(v,o);stream_buffer(v,o,len);
    int ok;
    if(entry->is_zip)ok=mz_zip_reader_extract_to_mem(&entry->zip,(mz_uint)url->resource_index,o->buffer,len,0);
    else {FILE *f=fopen(file,"rb");ok=f&&(fread(o->buffer,1,len,f)==len);if(f)fclose(f);}
    v->nr-=2;if(!ok){throwing(v,"java/io/IOException");return NULL;}return o;
}
enum { CONNECTION_CONNECTED=1,CONNECTION_CACHES=2,CONNECTION_INPUT=4,CONNECTION_OUTPUT=8,CONNECTION_CLOSED=16 };
static int resource_closed(Object *o) {
    Object *owner=o->count?obj(o->data[0]):NULL;
    return o->closed||(owner&&owner->kind=='U'&&(integer(owner->data[2])&CONNECTION_CLOSED));
}
static int stream_read(VM *v,Object *o) {
    if(o->kind=='j'&&resource_closed(o)){throwing(v,"java/io/IOException");return -1;}
    if(o->kind=='b'||o->kind=='j') {
        if(o->cursor>=o->buffer_size)return -1;
        Object *array=o->kind=='b'&&o->count&&o->data[0].tag==REF?obj(o->data[0]):NULL;
        return array?(int)(array->data[o->cursor++].bits&255):o->buffer[o->cursor++];
    }
    Method *m=method(o->cls,"read","()I");if(!m)fail(v,"InputStream.read is not implemented");
    Value a=rv(o);return integer(execute(v,m,&a,1));
}
static int reader_read(VM *v,Object *o) {
    if(o->closed){throwing(v,"java/io/IOException");return -1;}
    if(o->kind=='R') {
        int c=reader_read(v,obj(o->data[0]));
        if(o->skip_lf){o->skip_lf=0;if(c=='\n')c=reader_read(v,obj(o->data[0]));}return c;
    }
    if(o->kind!='r'){Method *m=method(o->cls,"read","()I");if(!m)fail(v,"Reader.read is not implemented");Value a=rv(o);return integer(execute(v,m,&a,1));}
    if(o->pending>=0){int c=o->pending;o->pending=-1;return c;}
    Object *stream=obj(o->data[0]);int first=stream_read(v,stream);if(first<0||first<128)return first;
    unsigned cp;int extra;
    if(first>=0xc2&&first<=0xdf){cp=(unsigned)first&31;extra=1;}
    else if(first>=0xe0&&first<=0xef){cp=(unsigned)first&15;extra=2;}
    else if(first>=0xf0&&first<=0xf4){cp=(unsigned)first&7;extra=3;}
    else return 0xfffd;
    for(int i=0;i<extra;i++) {
        int b=stream_read(v,stream);
        if(b<0)return 0xfffd;
        if((b&0xc0)!=0x80){if(stream->kind=='b'||stream->kind=='j')stream->cursor--;else fail(v,"UTF-8 pushback for custom streams is not implemented");return 0xfffd;}
        cp=(cp<<6)|((unsigned)b&63);
    }
    if((extra==1&&cp<0x80)||(extra==2&&cp<0x800)||(extra==3&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return 0xfffd;
    if(cp>=0x10000){cp-=0x10000;o->pending=(int)(0xdc00|(cp&1023));return (int)(0xd800|(cp>>10));}
    return (int)cp;
}
static void reader_close(VM *v,Object *o) {
    if(o->kind=='b')return; /* ByteArrayInputStream.close has no effect. */
    if(o->kind=='j') {
        o->closed=1;Object *owner=o->count?obj(o->data[0]):NULL;
        if(owner&&owner->kind=='U'&&!(integer(owner->data[2])&CONNECTION_CACHES))owner->data[2]=iv(integer(owner->data[2])|CONNECTION_CLOSED);
        return;
    }
    if(o->kind=='r'||o->kind=='R') {
        if(!o->closed){o->closed=1;reader_close(v,obj(o->data[0]));o->data[0]=rv(NULL);}return;
    }
    Method *m=method(o->cls,"close","()V");if(!m)fail(v,"stream close is not implemented");Value a=rv(o);execute(v,m,&a,1);
}
/* Classpaths are fixed for a VM run. Connections never cache decompressed JAR
 * entry bytes: each JAR getInputStream call reads a fresh bounded snapshot.
 * File connections retain one stream, matching the file protocol handler. */
static Object *resource_connection(VM *v,Object *url) {
    if(!url||url->kind!='u'||!url->resource_name)fail(v,"unsupported URL connection");
    Object *o=new_object(v,load(v,v->paths[url->resource_path].is_zip?"nspire/JarConnection":"nspire/FileConnection"),'U',6);
    o->data[0]=rv(url);o->data[1]=rv(NULL);o->data[2]=iv(CONNECTION_CACHES|CONNECTION_INPUT);
    o->data[3]=iv(0);o->data[4]=iv(0);o->data[5]=val(0,LONG);return o;
}
static void connection_connect(VM *v,Object *o) {
    int flags=integer(o->data[2]);if(flags&CONNECTION_CONNECTED)return;
    Object *url=obj(o->data[0]);ClassPath *path=&v->paths[url->resource_path];
    if(path->is_zip) {
        mz_zip_archive_file_stat st;
        if(!mz_zip_reader_file_stat(&path->zip,(mz_uint)url->resource_index,&st)){throwing(v,"java/io/FileNotFoundException");return;}
        o->data[5]=val(st.m_uncomp_size,LONG);
    } else {
        Object *stream=resource_stream(v,url);if(v->exception)return;
        o->data[1]=rv(stream);o->data[5]=val(stream->buffer_size,LONG);
    }
    o->data[2]=iv(flags|CONNECTION_CONNECTED);
}
static Value loader_native(VM *v,Class *c,const char *n,const char *d,Value *a,unsigned na,int stat,int *handled) {
    const char *cl=c->name;Object *self=stat?NULL:obj(a[0]);Value none=iv(0);*handled=1;
    if(stat&&!strcmp(cl,"java/lang/reflect/Array")&&!strcmp(n,"newInstance")&&!strcmp(d,"(Ljava/lang/Class;I)Ljava/lang/Object;")) {
        Object *mirror=nonnull(v,a[0]);if(!mirror)return none;
        Class *type=mirror->represented;if(mirror->kind!='c'||!type)fail(v,"Array.newInstance requires Class");
        unsigned rank=0;for(Class *component=type;component->component;component=component->component)rank++;
        if(type->primitive=='V'||rank>=255){throwing(v,"java/lang/IllegalArgumentException");return none;}
        char desc[1024];
        if(type->primitive){desc[0]='[';desc[1]=(char)type->primitive;desc[2]=0;}
        else if(snprintf(desc,sizeof desc,type->component?"[%s":"[L%s;",type->name)>=(int)sizeof desc)fail(v,"array descriptor too long");
        return rv(array_new(v,desc,integer(a[1])));
    }
    if(!strcmp(cl,"java/lang/ClassLoader")) {
        if(stat&&!strcmp(n,"getSystemClassLoader")&&!strcmp(d,"()Ljava/lang/ClassLoader;"))return rv(system_loader(v));
        if(!stat&&self!=system_loader(v))fail(v,"custom ClassLoader dispatch is not implemented");
        if(!stat&&!strcmp(n,"getParent")&&!strcmp(d,"()Ljava/lang/ClassLoader;"))return rv(NULL);
        if(!stat&&!strcmp(n,"loadClass")&&(!strcmp(d,"(Ljava/lang/String;)Ljava/lang/Class;")||!strcmp(d,"(Ljava/lang/String;Z)Ljava/lang/Class;"))) {
            Class *target=named_class(v,obj(a[1]),1,0);return rv(target?class_mirror(v,target):NULL);
        }
        const char *name=stat?n+(!strncmp(n,"getSystem",9)?9:0):n+(!strncmp(n,"get",3)?3:0);
        int all=!strcmp(name,"Resources"),stream=!strcmp(name,"ResourceAsStream"),one=!strcmp(name,"Resource");
        if((all&&!strcmp(d,"(Ljava/lang/String;)Ljava/util/Enumeration;"))||
           (stream&&!strcmp(d,"(Ljava/lang/String;)Ljava/io/InputStream;"))||
           (one&&!strcmp(d,"(Ljava/lang/String;)Ljava/net/URL;"))) {
            Object *s=nonnull(v,a[stat?0:1]);if(!s)return none;Object *r=resources(v,s->text,1,all);
            return rv(stream&&r?resource_stream(v,r):r);
        }
    }
    if(!strcmp(cl,"java/lang/Class")&&!stat) {
        Class *target=self->represented;
        if(!strcmp(n,"getClassLoader")&&!strcmp(d,"()Ljava/lang/ClassLoader;"))return rv(target->app_loader?system_loader(v):NULL);
        if(!strcmp(n,"newInstance")&&!strcmp(d,"()Ljava/lang/Object;"))return rv(reflect_new(v,target));
        if((!strcmp(n,"getResource")&&!strcmp(d,"(Ljava/lang/String;)Ljava/net/URL;"))||
           (!strcmp(n,"getResourceAsStream")&&!strcmp(d,"(Ljava/lang/String;)Ljava/io/InputStream;"))) {
            Object *s=nonnull(v,a[1]);if(!s)return none;const char *resource=s->text;char *joined=NULL;
            if(*resource=='/')resource++;
            else {Class *base=target;while(base->component)base=base->component;const char *slash=strrchr(base->name,'/');
                if(slash){size_t prefix=(size_t)(slash-base->name)+1;joined=(char *)alloc(v,prefix+strlen(resource)+1);memcpy(joined,base->name,prefix);strcpy(joined+prefix,resource);resource=joined;}}
            Object *r=resources(v,resource,target->app_loader,0);release(v,joined);
            return rv(!strcmp(n,"getResourceAsStream")&&r?resource_stream(v,r):r);
        }
    }
    if(!strcmp(cl,"nspire/ResourceEnumeration")) {
        if(!strcmp(n,"hasMoreElements")&&!strcmp(d,"()Z"))return iv(self->cursor<self->buffer_size);
        if(!strcmp(n,"nextElement")&&!strcmp(d,"()Ljava/lang/Object;")) {
            if(self->cursor==self->buffer_size){throwing(v,"java/util/NoSuchElementException");return none;}
            return self->data[self->cursor++];
        }
    }
    if(!strcmp(cl,"java/net/URL")) {
        if(!strcmp(n,"openConnection")&&!strcmp(d,"()Ljava/net/URLConnection;"))return rv(resource_connection(v,self));
        if(!strcmp(n,"openStream")&&!strcmp(d,"()Ljava/io/InputStream;"))return rv(resource_stream(v,self));
        if((!strcmp(n,"toString")||!strcmp(n,"toExternalForm"))&&!strcmp(d,"()Ljava/lang/String;"))return rv(string(v,self->text));
        if(!strcmp(n,"getProtocol")&&!strcmp(d,"()Ljava/lang/String;"))return rv(string(v,v->paths[self->resource_path].is_zip?"jar":"file"));
    }
    if(self&&self->kind=='U') {
        int flags=integer(self->data[2]);Object *url=obj(self->data[0]);
        if(!strcmp(n,"getURL")&&!strcmp(d,"()Ljava/net/URL;"))return self->data[0];
        if(!strcmp(n,"connect")&&!strcmp(d,"()V")){connection_connect(v,self);return none;}
        if(!strcmp(n,"getInputStream")&&!strcmp(d,"()Ljava/io/InputStream;")) {
            connection_connect(v,self);if(v->exception)return none;
            if(flags&CONNECTION_CLOSED){throwing(v,"java/lang/IllegalStateException");return none;}
            if(v->paths[url->resource_path].is_zip) {
                Object *stream=resource_stream(v,url);if(stream)stream->data[0]=rv(self);return rv(stream);
            }
            return self->data[1];
        }
        if(!strcmp(n,"getOutputStream")&&!strcmp(d,"()Ljava/io/OutputStream;")){throwing(v,"java/net/UnknownServiceException");return none;}
        if(!strcmp(n,"getEntryName")&&!strcmp(d,"()Ljava/lang/String;")&&v->paths[url->resource_path].is_zip)return rv(string(v,url->resource_name));
        if((!strcmp(n,"getContentLength")&&!strcmp(d,"()I"))||(!strcmp(n,"getContentLengthLong")&&!strcmp(d,"()J"))) {
            connection_connect(v,self);if(v->exception)return none;
            return d[2]=='J'?self->data[5]:iv(self->data[5].bits>INT_MAX?-1:(int32_t)self->data[5].bits);
        }
        const char *getters[]={"getUseCaches","getDoInput","getDoOutput"};
        const char *setters[]={"setUseCaches","setDoInput","setDoOutput"};
        const int bits[]={CONNECTION_CACHES,CONNECTION_INPUT,CONNECTION_OUTPUT};
        for(unsigned i=0;i<3;i++) {
            if(!strcmp(n,getters[i])&&!strcmp(d,"()Z"))return iv(!!(flags&bits[i]));
            if(!strcmp(n,setters[i])&&!strcmp(d,"(Z)V")) {
                if((flags&CONNECTION_CONNECTED)&&!(i==0&&v->paths[url->resource_path].is_zip)){throwing(v,"java/lang/IllegalStateException");return none;}
                self->data[2]=iv(integer(a[1])?flags|bits[i]:flags&~bits[i]);return none;
            }
        }
        if((!strcmp(n,"getConnectTimeout")||!strcmp(n,"getReadTimeout"))&&!strcmp(d,"()I"))return self->data[!strcmp(n,"getConnectTimeout")?3:4];
        if((!strcmp(n,"setConnectTimeout")||!strcmp(n,"setReadTimeout"))&&!strcmp(d,"(I)V")) {
            if(integer(a[1])<0){throwing(v,"java/lang/IllegalArgumentException");return none;}
            self->data[!strcmp(n,"setConnectTimeout")?3:4]=a[1];return none;
        }
    }
    int byte_stream=!strcmp(cl,"java/io/InputStream")||!strcmp(cl,"java/io/ByteArrayInputStream")||(self&&(self->kind=='b'||self->kind=='j'));
    int reader=!strcmp(cl,"java/io/Reader")||!strcmp(cl,"java/io/InputStreamReader")||!strcmp(cl,"java/io/BufferedReader")||(self&&(self->kind=='r'||self->kind=='R'));
    if((byte_stream||reader)&&!stat) {
        if(!strcmp(n,"<init>")) {
            if(!strcmp(cl,"java/io/ByteArrayInputStream")&&(!strcmp(d,"([B)V")||!strcmp(d,"([BII)V"))) {
                Object *bytes=nonnull(v,a[1]);if(!bytes)return none;
                int32_t off=na==4?integer(a[2]):0,len=na==4?integer(a[3]):(int32_t)bytes->count;
                if(off<0||len<0||(size_t)off>bytes->count){throwing(v,"java/lang/IndexOutOfBoundsException");return none;}
                if((size_t)len>bytes->count-(size_t)off)len=(int32_t)(bytes->count-(size_t)off);
                self->kind='b';self->data[0]=rv(bytes);self->cursor=self->mark_cursor=(size_t)off;self->buffer_size=(size_t)off+(size_t)len;return none;
            }
            if(!strcmp(cl,"java/io/InputStreamReader")&&(!strcmp(d,"(Ljava/io/InputStream;)V")||!strcmp(d,"(Ljava/io/InputStream;Ljava/lang/String;)V"))) {
                Object *in=nonnull(v,a[1]);if(!in)return none;
                if(na==3){Object *enc=nonnull(v,a[2]);if(!enc)return none;
                    if(strcmp(enc->text,"UTF-8")&&strcmp(enc->text,"utf-8")&&strcmp(enc->text,"UTF8")&&strcmp(enc->text,"utf8")){throwing(v,"java/io/UnsupportedEncodingException");return none;}}
                self->kind='r';self->pending=-1;self->data[0]=rv(in);return none;
            }
            if(!strcmp(cl,"java/io/BufferedReader")&&(!strcmp(d,"(Ljava/io/Reader;)V")||!strcmp(d,"(Ljava/io/Reader;I)V"))) {
                Object *in=nonnull(v,a[1]);if(!in)return none;if(na==3&&integer(a[2])<=0){throwing(v,"java/lang/IllegalArgumentException");return none;}
                self->kind='R';self->data[0]=rv(in);return none;
            }
        }
        if(!strcmp(n,"close")&&!strcmp(d,"()V")){reader_close(v,self);return none;}
        if(!strcmp(n,"read")&&!strcmp(d,"()I"))return iv(byte_stream?stream_read(v,self):reader_read(v,self));
        if(!strcmp(n,"read")&&((byte_stream&&(!strcmp(d,"([B)I")||!strcmp(d,"([BII)I")))||(reader&&(!strcmp(d,"([C)I")||!strcmp(d,"([CII)I"))))) {
            Object *out=nonnull(v,a[1]);if(!out)return none;
            int32_t off=na==4?integer(a[2]):0,len=na==4?integer(a[3]):(int32_t)out->count;
            if(off<0||len<0||(size_t)off>out->count||(size_t)len>out->count-(size_t)off){throwing(v,"java/lang/IndexOutOfBoundsException");return none;}
            if((reader&&self->closed)||(self->kind=='j'&&resource_closed(self))){throwing(v,"java/io/IOException");return none;}
            if(byte_stream&&self->kind=='b'&&self->cursor>=self->buffer_size)return iv(-1);
            int32_t i=0;for(;i<len&&!v->exception;i++){int ch=byte_stream?stream_read(v,self):reader_read(v,self);if(ch<0)break;out->data[off+i]=iv(byte_stream?(int8_t)ch:ch);}
            return iv(i?i:len?-1:0);
        }
        if(byte_stream&&(self->kind=='b'||self->kind=='j')) {
            if(self->kind=='j'&&resource_closed(self)){throwing(v,"java/io/IOException");return none;}
            if(!strcmp(n,"available")&&!strcmp(d,"()I"))return iv((int32_t)(self->buffer_size-self->cursor));
            if(!strcmp(n,"markSupported")&&!strcmp(d,"()Z"))return iv(1);
            if(!strcmp(n,"mark")&&!strcmp(d,"(I)V")){self->mark_cursor=self->cursor;return none;}
            if(!strcmp(n,"reset")&&!strcmp(d,"()V")){self->cursor=self->mark_cursor;return none;}
            if(!strcmp(n,"skip")&&!strcmp(d,"(J)J")){int64_t want=(int64_t)a[1].bits;size_t skip=want<0?0:(uint64_t)want>self->buffer_size-self->cursor?self->buffer_size-self->cursor:(size_t)want;self->cursor+=skip;return val(skip,LONG);}
        }
        if(!strcmp(cl,"java/io/BufferedReader")&&!strcmp(n,"readLine")&&!strcmp(d,"()Ljava/lang/String;")) {
            size_t cap=128,used=0;char *buf=(char *)alloc(v,cap);int ch;
            while((ch=reader_read(v,self))>=0&&!v->exception) {
                if(ch=='\n'||ch=='\r'){self->skip_lf=ch=='\r';break;}
                if(used+4>cap){size_t next=cap*2;char *p=(char *)alloc(v,next);memcpy(p,buf,used);release(v,buf);buf=p;cap=next;}
                used+=write_unit(buf+used,(unsigned)ch);
            }
            buf[used]=0;Object *r=(!v->exception&&(ch>=0||used))?string(v,buf):NULL;release(v,buf);return rv(r);
        }
    }
    *handled=0;return none;
}
