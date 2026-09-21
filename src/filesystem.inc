/* Actual filesystem operations. File's path algorithms execute upstream Java.
 * Host and Ndless use UTF-8 paths. Unsupported SDK operations fail explicitly. */
static void file_error(VM *v,const char *type,const char *message) {
    throwing(v,type);set_text(v,v->exception,message);
}
static char *file_utf8(VM *v,Object *value) {
    if(!value){throwing(v,"java/lang/NullPointerException");return NULL;}
    if(value->kind!='s')fail(v,"filesystem path must be String");
    char *text=(char *)alloc(v,strlen(value->text)+1),*out=text;
    const unsigned char *p=(const unsigned char *)value->text;
    while(*p) {
        unsigned ch=utf_unit(&p);
        if(!ch){release(v,text);file_error(v,"java/io/IOException","Invalid file path");return NULL;}
        if(character_high(ch)&&*p){const unsigned char *next=p;unsigned low=utf_unit(&next);if(character_low(low)){ch=character_pair(ch,low);p=next;}}
        if(ch>=0xd800&&ch<=0xdfff)ch='?';
        if(ch>=0x10000){*out++=(char)(0xf0|(ch>>18));*out++=(char)(0x80|((ch>>12)&63));*out++=(char)(0x80|((ch>>6)&63));*out++=(char)(0x80|(ch&63));}
        else out+=write_unit(out,ch);
    }
    *out=0;return text;
}
static Object *file_string(VM *v,const char *raw) {
    size_t len=strlen(raw);if(len>INT32_MAX)fail(v,"filesystem name is too large");
    unsigned saved=v->nr;Object *bytes=array_new(v,"[B",(int32_t)len);root(v,bytes);
    for(size_t i=0;i<len;i++)bytes->data[i]=iv((int8_t)raw[i]);
    Object *encoding=string(v,"UTF-8");root(v,encoding);
    Class *bridge=load(v,"nspire/charset/StringCoding");initialize(v,bridge);
    Value a[]={rv(bytes),iv(0),iv((int32_t)len),rv(encoding)},result=rv(NULL);
    if(!v->exception)result=execute(v,method(bridge,"decode","([BIILjava/lang/String;)Ljava/lang/String;"),a,4);
    v->nr=saved;return obj(result);
}
static void filesystem_start(VM *v) {
    /* Initialize lazily through the existing UTF-8 reader: startup of a tiny
     * program should not need to load the entire charset library. */
    char cwd[PATH_MAX+1];if(!getcwd(cwd,sizeof cwd))fail(v,"cannot read working directory: %s",strerror(errno));
    size_t len=strlen(cwd);Object *stream=new_object(v,load(v,"java/io/ByteArrayInputStream"),'b',0);root(v,stream);
    stream_buffer(v,stream,len);memcpy(stream->buffer,cwd,len);
    Object *reader=new_object(v,load(v,"java/io/InputStreamReader"),'r',1);root(v,reader);reader->data[0]=rv(stream);reader->pending=-1;
    char *text=(char *)alloc(v,len*3+1),*out=text;int ch;
    while(!v->exception&&(ch=reader_read(v,reader))>=0)out+=write_unit(out,(unsigned)ch);
    *out=0;if(v->exception)fail(v,"cannot decode working directory");
    Property *p=property(v,"user.dir",1);p->value=copy(v,text);p->initial=copy(v,text);release(v,text);v->nr-=2;
}
static char *file_path(VM *v,Value value) {
    Object *file=nonnull(v,value);if(!file)return NULL;
    Class *base=load(v,"java/io/File");Field *path=field(v,&base,"path","Ljava/lang/String;");
    if(!subtype(file->cls,base)||path->slot>=file->count)fail(v,"invalid File object");
    return file_utf8(v,obj(file->data[path->slot]));
}
static void directory_close(VM *v,NativeDir *entry) {
    NativeDir **p=&v->directories;while(*p&&*p!=entry)p=&(*p)->next;
    if(*p){*p=entry->next;closedir(entry->handle);release(v,entry);}
}
static Object *file_list(VM *v,const char *path) {
    typedef struct Name {struct Name *next;char *text;} Name;
    NativeDir *entry=(NativeDir *)alloc(v,sizeof *entry);entry->handle=opendir(path);
    if(!entry->handle){release(v,entry);return NULL;}
    entry->next=v->directories;v->directories=entry;
    Name *head=NULL,**tail=&head;size_t count=0;struct dirent *item;
    while((item=readdir(entry->handle))) {
        if(!strcmp(item->d_name,".")||!strcmp(item->d_name,".."))continue;
        Name *name=(Name *)alloc(v,sizeof *name);name->text=copy(v,item->d_name);*tail=name;tail=&name->next;count++;
    }
    directory_close(v,entry);
    Object *result=array_new(v,"[Ljava/lang/String;",(int32_t)count);root(v,result);
    size_t i=0;while(head){Name *name=head;head=head->next;if(!v->exception)result->data[i++]=rv(file_string(v,name->text));release(v,name->text);release(v,name);}
    v->nr--;return v->exception?NULL:result;
}
static Value filesystem_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int *handled) {
    *handled=!strcmp(c->name,"java/io/UnixFileSystem");if(!*handled)return iv(0);
    Value result=iv(0);
    if(!strcmp(name,"initIDs")&&!strcmp(desc,"()V")) {
        Class *file=load(v,"java/io/File");(void)field(v,&file,"path","Ljava/lang/String;");return result;
    }
    int canonical=!strcmp(name,"canonicalize0"),create=!strcmp(name,"createFileExclusively");
    char *path=canonical||create?file_utf8(v,obj(args[1])):file_path(v,args[1]);if(!path)return result;
    struct stat st;memset(&st,0,sizeof st);
    if(canonical){char resolved[PATH_MAX+1];if(canonicalize(path,resolved,sizeof resolved))file_error(v,"java/io/IOException",strerror(errno));else result=rv(file_string(v,resolved));}
    else if(!strcmp(name,"getBooleanAttributes0")){if(!stat(path,&st))result=iv(1|(S_ISREG(st.st_mode)?2:0)|(S_ISDIR(st.st_mode)?4:0));}
    else if(!strcmp(name,"getLength"))result=val(!stat(path,&st)?(uint64_t)st.st_size:0,LONG);
    else if(!strcmp(name,"getLastModifiedTime")) {
        int64_t millis=0;if(!stat(path,&st)){
#if defined(_TINSPIRE) && (__NEWLIB__ < 3 || (__NEWLIB__ == 3 && __NEWLIB_MINOR__ < 2))
            millis=(int64_t)st.st_mtime*1000;
#else
            millis=(int64_t)st.st_mtim.tv_sec*1000;
#endif
#ifndef _TINSPIRE
            millis+=st.st_mtim.tv_nsec/1000000;
#endif
        }result=val((uint64_t)millis,LONG);
    }
    else if(!strcmp(name,"list"))result=rv(file_list(v,path));
    else if(!strcmp(name,"createDirectory"))result=iv(mkdir(path,0777)==0);
    else if(!strcmp(name,"delete0"))result=iv(remove(path)==0);
    else if(!strcmp(name,"rename0")){char *to=file_path(v,args[2]);if(to){result=iv(rename(path,to)==0);release(v,to);}}
    else {
#ifdef _TINSPIRE
        /* SDK _open's O_EXCL path is incorrect and not atomic; fstat/chmod/
         * utimes/statvfs/access have no verified device implementation yet. */
        fail(v,"Ndless filesystem operation is not implemented: %s%s",name,desc);
#else
        if(!strcmp(name,"checkAccess"))result=iv(access(path,integer(args[2]))==0);
        else if(create){int fd=open(path,O_RDWR|O_CREAT|O_EXCL,0666);if(fd>=0){close(fd);result=iv(1);}else if(errno!=EEXIST)file_error(v,"java/io/IOException",strerror(errno));}
        else if(!strcmp(name,"setPermission")||!strcmp(name,"setReadOnly")) {
            if(!stat(path,&st)){
                mode_t mask;
                if(!strcmp(name,"setReadOnly"))mask=0222;
                else {int mode=integer(args[2]);mask=mode==4?0400:mode==2?0200:0100;if(!integer(args[4]))mask|=(mask>>3)|(mask>>6);}
                int enable=!strcmp(name,"setPermission")&&integer(args[3]);
                result=iv(chmod(path,enable?st.st_mode|mask:st.st_mode&~mask)==0);
            }
        }
        else if(!strcmp(name,"setLastModifiedTime")) {
            int64_t millis=(int64_t)args[2].bits;time_t seconds=(time_t)(millis/1000);
            if((int64_t)seconds==millis/1000&&!stat(path,&st)){
                struct timeval times[2];times[0].tv_sec=st.st_atime;times[0].tv_usec=st.st_atim.tv_nsec/1000;
                times[1].tv_sec=seconds;times[1].tv_usec=(suseconds_t)((millis%1000)*1000);result=iv(utimes(path,times)==0);
            }
        }
        else if(!strcmp(name,"getSpace")) {
            struct statvfs space;uint64_t bytes=0;
            if(!statvfs(path,&space)){uint64_t blocks=integer(args[2])==0?space.f_blocks:integer(args[2])==1?space.f_bfree:space.f_bavail;
                uint64_t unit=space.f_frsize;bytes=unit&&blocks>(uint64_t)INT64_MAX/unit?INT64_MAX:blocks*unit;}
            result=val(bytes,LONG);
        }
        else fail(v,"unbound filesystem method: %s%s",name,desc);
#endif
    }
    release(v,path);return result;
}
static void filestream_class(VM *v,Class *c) {
    static const char *input[]={"<init>","(Ljava/lang/String;)V","<init>","(Ljava/io/File;)V","read","()I","read","([B)I","read","([BII)I","skip","(J)J","available","()I","close","()V","markSupported","()Z","mark","(I)V","reset","()V"};
    static const char *output[]={"<init>","(Ljava/lang/String;)V","<init>","(Ljava/io/File;)V","<init>","(Ljava/lang/String;Z)V","<init>","(Ljava/io/File;Z)V","write","(I)V","write","([B)V","write","([BII)V","close","()V"};
    int in=!strcmp(c->name,"java/io/FileInputStream");const char **spec=in?input:output;
    c->nm=(uint16_t)((in?sizeof input:sizeof output)/sizeof input[0]/2);c->methods=(Method *)alloc(v,c->nm*sizeof(Method));
    for(unsigned i=0;i<c->nm;i++){Method *m=&c->methods[i];m->owner=c;m->name=(char *)spec[2*i];m->desc=(char *)spec[2*i+1];m->flags=1|NATIVE;}
}
static void file_open_stream(VM *v,Object *self,Value arg,int by_string,int output,int append) {
    unsigned saved=v->nr;root(v,self);Object *file=nonnull(v,arg);if(!file)goto done;
    Class *base=load(v,"java/io/File");initialize(v,base);if(v->exception)goto done;
    if(by_string){file=new_object(v,base,'o',base->slots);root(v,file);Value a[]={rv(file),arg};execute(v,method(base,"<init>","(Ljava/lang/String;)V"),a,2);if(v->exception)goto done;}
    Value receiver=rv(file),path_value=execute(v,method(file->cls,"getPath","()Ljava/lang/String;"),&receiver,1);
    if(v->exception)goto done;root(v,obj(path_value));
    Value invalid=execute(v,method(base,"isInvalid","()Z"),&receiver,1);if(v->exception)goto done;
    if(integer(invalid)){file_error(v,"java/io/FileNotFoundException","Invalid file path");goto done;}
    char *path=file_utf8(v,obj(path_value));if(!path)goto done;
    struct stat info;memset(&info,0,sizeof info);
    /* The SDK has no fstat. Reject directories before its fopen-based open;
     * host also checks the opened descriptor, avoiding the host path race. */
    int directory=!stat(path,&info)&&S_ISDIR(info.st_mode);
    int fd=directory?-1:open(path,output?(O_WRONLY|O_CREAT|(append?O_APPEND:O_TRUNC)):O_RDONLY,0666);
    int error=directory?EISDIR:errno;release(v,path);
    if(fd<0){file_error(v,"java/io/FileNotFoundException",strerror(error));goto done;}
    self->file_fd=fd;self->file_open=1;
#ifndef _TINSPIRE
    if(fstat(fd,&info)||S_ISDIR(info.st_mode)){error=S_ISDIR(info.st_mode)?EISDIR:errno;self->file_open=0;close(fd);file_error(v,"java/io/FileNotFoundException",strerror(error));}
#endif
done:v->nr=saved;
}
static Value filestream_native(VM *v,Class *c,const char *name,const char *desc,Value *args,unsigned count,int *handled) {
    int input=!strcmp(c->name,"java/io/FileInputStream"),output=!strcmp(c->name,"java/io/FileOutputStream");
    *handled=input||output;if(!*handled)return iv(0);Object *self=obj(args[0]);Value result=iv(0);
    int known=0;for(unsigned i=0;i<c->nm;i++)if(!strcmp(c->methods[i].name,name)&&!strcmp(c->methods[i].desc,desc))known=1;
    if(!known)fail(v,"file stream method is not implemented: %s.%s%s",c->name,name,desc);
    if(!strcmp(name,"<init>")){file_open_stream(v,self,args[1],strstr(desc,"Ljava/lang/String;")!=NULL,output,count==3&&integer(args[2]));return result;}
    if(!strcmp(name,"markSupported")||!strcmp(name,"mark"))return result;
    if(!strcmp(name,"reset")){file_error(v,"java/io/IOException","mark/reset not supported");return result;}
    if(!strcmp(name,"close")){if(self->file_open){self->file_open=0;if(close(self->file_fd))file_error(v,"java/io/IOException",strerror(errno));}return result;}
    Object *bytes=NULL;int32_t off=0,len=0;
    if(desc[1]=='['){bytes=nonnull(v,args[1]);if(!bytes)return result;
        if(bytes->kind!='a'||strcmp(bytes->array_desc,"[B"))fail(v,"file stream requires byte[]");
        off=count==4?integer(args[2]):0;len=count==4?integer(args[3]):(int32_t)bytes->count;
        if(off<0||len<0||(size_t)off>bytes->count||(size_t)len>bytes->count-(size_t)off){throwing(v,"java/lang/IndexOutOfBoundsException");return result;}
        if(!len)return result;
    }
    if(!self->file_open){file_error(v,"java/io/IOException","Stream Closed");return result;}
    if(!strcmp(name,"read")) {
        unsigned char buffer[4096];size_t size=bytes?(size_t)len:1;if(size>sizeof buffer)size=sizeof buffer;ssize_t got;
        do{got=read(self->file_fd,buffer,size);}while(got<0&&errno==EINTR);
        if(got<0)file_error(v,"java/io/IOException",strerror(errno));
        else if(!got)result=iv(-1);
        else if(!bytes)result=iv(buffer[0]);
        else {for(ssize_t i=0;i<got;i++)bytes->data[off+i]=iv((int8_t)buffer[i]);result=iv((int32_t)got);}
    }
    else if(!strcmp(name,"write")) {
        int32_t used=0,total=bytes?len:1;unsigned char buffer[4096];
        while(used<total&&!v->exception){size_t size=(size_t)(total-used);if(size>sizeof buffer)size=sizeof buffer;
            for(size_t i=0;i<size;i++)buffer[i]=(unsigned char)integer(bytes?bytes->data[off+used+i]:args[1]);
            size_t sent=0;while(sent<size){ssize_t n=write(self->file_fd,buffer+sent,size-sent);if(n<0&&errno==EINTR)continue;
                if(n<=0){file_error(v,"java/io/IOException",n<0?strerror(errno):"Write made no progress");break;}sent+=(size_t)n;}used+=(int32_t)sent;
        }
    }
    else if(!strcmp(name,"skip")) {
        int64_t amount=(int64_t)args[1].bits;off_t delta=(off_t)amount,current=lseek(self->file_fd,0,SEEK_CUR);result=val(0,LONG);
        if((int64_t)delta!=amount||current<0||(amount>0&&(uint64_t)current>(uint64_t)INT64_MAX-(uint64_t)amount)||
            (amount<0&&(uint64_t)current<(uint64_t)(-(amount+1))+1))file_error(v,"java/io/IOException","Invalid seek offset");
        else {off_t after=lseek(self->file_fd,delta,SEEK_CUR);if(after<0)file_error(v,"java/io/IOException",strerror(errno));else result=val((uint64_t)after-(uint64_t)current,LONG);}
    }
    else if(!strcmp(name,"available")) {
        off_t current=lseek(self->file_fd,0,SEEK_CUR),end=-1,restored=-1;
        if(current>=0){end=lseek(self->file_fd,0,SEEK_END);restored=lseek(self->file_fd,current,SEEK_SET);}
        if(current<0||end<0||restored<0)file_error(v,"java/io/IOException",strerror(errno));
        else {uint64_t left=end>current?(uint64_t)end-(uint64_t)current:0;result=iv(left>INT32_MAX?INT32_MAX:(int32_t)left);}
    }
    return result;
}
