/* FileDescriptor and stream control flow execute the preserved Java classes.
 * This bridge owns only platform handles; shared close callbacks stay in Java. */
static Field *descriptor_field(VM *v) {
    Class *type=load(v,"java/io/FileDescriptor");return field(v,&type,"fd","I");
}
static int descriptor_number(VM *v,Object *descriptor) {
    if(!descriptor){throwing(v,"java/io/IOException");return -1;}
    return integer(descriptor->data[descriptor_field(v)->slot]);
}
static Object *stream_descriptor(VM *v,Class *type,Object *stream) {
    Field *f=field(v,&type,"fd","Ljava/io/FileDescriptor;");return obj(stream->data[f->slot]);
}
static void descriptor_close(VM *v,Object *descriptor) {
    int fd=descriptor_number(v,descriptor);if(v->exception||fd<0)return;
    descriptor->data[descriptor_field(v)->slot]=iv(-1);descriptor->file_open=0;
    /* End the shared Java descriptor, retaining the embedding process/SDK UI.
     * All Java aliases (including System.in/out/err) observe fd == -1. */
    if(descriptor->file_owned&&close(fd))file_error(v,"java/io/IOException",strerror(errno));
}
static void descriptor_open(VM *v,Object *descriptor,Object *name,int output,int append) {
    char *path=file_utf8(v,name);if(!path)return;
    struct stat info;memset(&info,0,sizeof info);
    int directory=!stat(path,&info)&&S_ISDIR(info.st_mode);
    int fd=directory?-1:open(path,output?(O_WRONLY|O_CREAT|(append?O_APPEND:O_TRUNC)):O_RDONLY,0666);
    int error=directory?EISDIR:errno;release(v,path);
    if(fd<0){file_error(v,"java/io/FileNotFoundException",strerror(error));return;}
    /* Register ownership before anything can allocate, throw, or abort. */
    descriptor->file_fd=fd;descriptor->file_owned=1;descriptor->file_open=1;
    descriptor->data[descriptor_field(v)->slot]=iv(fd);
#ifndef _TINSPIRE
    if(fstat(fd,&info)||S_ISDIR(info.st_mode)) {
        error=S_ISDIR(info.st_mode)?EISDIR:errno;descriptor_close(v,descriptor);
        file_error(v,"java/io/FileNotFoundException",strerror(error));
    }
#endif
}
static ssize_t descriptor_read(VM *v,Object *descriptor,int fd,unsigned char *out,size_t size) {
#ifdef _TINSPIRE
    if(fd==0&&!descriptor->file_owned) {
        /* SDK _read(stdin, ..., 1) is unsafe: it appends a newline after
         * nio_fgets. Always request a real line buffer and serve byte reads
         * from it; aliases share its cursor on the descriptor. */
        if(descriptor->cursor==descriptor->mark_cursor) {
            if(!descriptor->buffer)stream_buffer(v,descriptor,4096);
            ssize_t got=read(0,descriptor->buffer,descriptor->buffer_size);
            if(got<=0)return got;descriptor->cursor=0;descriptor->mark_cursor=(size_t)got;
        }
        size_t left=descriptor->mark_cursor-descriptor->cursor;if(size>left)size=left;
        memcpy(out,descriptor->buffer+descriptor->cursor,size);descriptor->cursor+=size;return (ssize_t)size;
    }
#else
    (void)v;(void)descriptor;
#endif
    ssize_t got;do{got=read(fd,out,size);}while(got<0&&errno==EINTR);return got;
}
static Value descriptor_available(VM *v,Object *descriptor,int fd) {
#ifdef _TINSPIRE
    if(fd==0&&!descriptor->file_owned)return iv((int32_t)(descriptor->mark_cursor-descriptor->cursor));
    off_t current=lseek(fd,0,SEEK_CUR),end=-1,restored=-1;
    if(current>=0){end=lseek(fd,0,SEEK_END);restored=lseek(fd,current,SEEK_SET);}
    if(current<0||end<0||restored<0){file_error(v,"java/io/IOException",strerror(errno));return iv(0);}
    uint64_t left=end>current?(uint64_t)end-(uint64_t)current:0;
#else
    (void)descriptor;struct stat st;
    if(fstat(fd,&st)){file_error(v,"java/io/IOException",strerror(errno));return iv(0);}
    uint64_t left=0;
    if(S_ISREG(st.st_mode)||S_ISDIR(st.st_mode)) {
        off_t current=lseek(fd,0,SEEK_CUR);if(current<0){file_error(v,"java/io/IOException",strerror(errno));return iv(0);}
        if(st.st_size>current)left=(uint64_t)st.st_size-(uint64_t)current;
    }else {
        int ready=0;if(ioctl(fd,FIONREAD,&ready)<0){file_error(v,"java/io/IOException",strerror(errno));return iv(0);}
        left=ready>0?(unsigned)ready:0;
    }
#endif
    return iv(left>INT32_MAX?INT32_MAX:(int32_t)left);
}
static Value filestream_native(VM *v,Class *c,const char *name,const char *desc,Value *args,unsigned count,int *handled) {
    int input=!strcmp(c->name,"java/io/FileInputStream"),output=!strcmp(c->name,"java/io/FileOutputStream"),fd_class=!strcmp(c->name,"java/io/FileDescriptor");
    *handled=input||output||fd_class;if(!*handled)return iv(0);Value result=iv(0);
    if(!strcmp(name,"initIDs")&&!strcmp(desc,"()V")) {
        (void)descriptor_field(v);if(!fd_class)(void)field(v,&c,"fd","Ljava/io/FileDescriptor;");return result;
    }
    Object *self=obj(args[0]),*descriptor=fd_class?self:stream_descriptor(v,c,self);
    if(fd_class&& !strcmp(name,"sync")&&!strcmp(desc,"()V")) {
#ifdef _TINSPIRE
        /* The SDK has no verified durable-storage synchronization primitive. */
        file_error(v,"java/io/SyncFailedException","Ndless cannot guarantee storage synchronization");
#else
        int fd=descriptor_number(v,descriptor);
        if(!v->exception&&(fd<0||fsync(fd)))file_error(v,"java/io/SyncFailedException",fd<0?"Invalid file descriptor":strerror(errno));
#endif
        return result;
    }
    if(!strcmp(name,"open0")){descriptor_open(v,descriptor,obj(args[1]),output,output&&integer(args[2]));return result;}
    if(!strcmp(name,"close0")){descriptor_close(v,descriptor);return result;}
    Object *bytes=NULL;int32_t off=0,len=0;
    if(!strcmp(name,"readBytes")||!strcmp(name,"writeBytes")) {
        bytes=nonnull(v,args[1]);if(!bytes)return result;
        if(bytes->kind!='a'||strcmp(bytes->array_desc,"[B"))fail(v,"file stream requires byte[]");
        off=integer(args[2]);len=integer(args[3]);
        if(off<0||len<0||(size_t)off>bytes->count||(size_t)len>bytes->count-(size_t)off){throwing(v,"java/lang/IndexOutOfBoundsException");return result;}
        if(!len)return result;
    }
    int fd=descriptor_number(v,descriptor);if(v->exception)return result;
    if(fd<0){file_error(v,"java/io/IOException","Stream Closed");return result;}
    if(input&&(!strcmp(name,"read0")||!strcmp(name,"readBytes"))) {
        unsigned char buffer[4096];size_t size=bytes?(size_t)len:1;if(size>sizeof buffer)size=sizeof buffer;
        ssize_t got=descriptor_read(v,descriptor,fd,buffer,size);
        if(got<0)file_error(v,"java/io/IOException",strerror(errno));else if(!got)result=iv(-1);else if(!bytes)result=iv(buffer[0]);
        else {for(ssize_t i=0;i<got;i++)bytes->data[off+i]=iv((int8_t)buffer[i]);result=iv((int32_t)got);}
    }else if(output&&(!strcmp(name,"write")||!strcmp(name,"writeBytes"))) {
        int32_t used=0,total=bytes?len:1;unsigned char buffer[4096];
        while(used<total&&!v->exception) {
            size_t size=(size_t)(total-used);if(size>sizeof buffer)size=sizeof buffer;
            for(size_t i=0;i<size;i++)buffer[i]=(unsigned char)integer(bytes?bytes->data[off+used+i]:args[1]);
            size_t sent=0;while(sent<size) {
                ssize_t n=write(fd,buffer+sent,size-sent);if(n<0&&errno==EINTR)continue;
                if(n<=0){file_error(v,"java/io/IOException",n<0?strerror(errno):"Write made no progress");break;}sent+=(size_t)n;
            }used+=(int32_t)sent;
        }
    }else if(input&&!strcmp(name,"skip0")) {
        int64_t amount=(int64_t)args[1].bits;off_t delta=(off_t)amount,current=lseek(fd,0,SEEK_CUR);result=val(0,LONG);
        if((int64_t)delta!=amount||current<0||(amount>0&&(uint64_t)current>(uint64_t)INT64_MAX-(uint64_t)amount)||
            (amount<0&&(uint64_t)current<(uint64_t)(-(amount+1))+1))file_error(v,"java/io/IOException","Invalid seek offset");
        else {off_t after=lseek(fd,delta,SEEK_CUR);if(after<0)file_error(v,"java/io/IOException",strerror(errno));else result=val((uint64_t)after-(uint64_t)current,LONG);}
    }else if(input&&!strcmp(name,"available0"))result=descriptor_available(v,descriptor,fd);
    else fail(v,"unbound file native: %s.%s%s",c->name,name,desc);
    (void)count;return result;
}
static Object *file_instance(VM *v,Class *type) {
    Object *object=new_object(v,type,'o',type->slots);
    for(Class *c=type;c;c=c->super)for(unsigned i=0;i<c->nf;i++)if(!(c->fields[i].flags&STATIC))object->data[c->fields[i].slot]=zero(c->fields[i].desc);
    return object;
}
static void system_streams(VM *v,Class *system) {
    unsigned saved=v->nr;Class *descriptors=load(v,"java/io/FileDescriptor");initialize(v,descriptors);if(v->exception)return;
    for(unsigned i=0;i<3&&!v->exception;i++) {
        Class *base=descriptors;Field *f=field(v,&base,i==2?"in":i?"err":"out","Ljava/io/FileDescriptor;");
        Class *type=load(v,i==2?"java/io/FileInputStream":"java/io/FileOutputStream");initialize(v,type);if(v->exception)break;
        Object *stream=file_instance(v,type);root(v,stream);Value a[]={rv(stream),f->value};
        execute(v,method(type,"<init>","(Ljava/io/FileDescriptor;)V"),a,2);if(v->exception)break;
        if(i==2)system->fields[i].value=rv(stream);
        else {
            Class *ps=load(v,"java/io/PrintStream");initialize(v,ps);if(v->exception)break;
            Object *out=new_object(v,ps,'P',ps->slots);out->output_flags=OUTPUT_AUTOFLUSH;out->data[0]=rv(stream);system->fields[i].value=rv(out);
        }
        v->nr=saved;
    }
    v->nr=saved;
}
