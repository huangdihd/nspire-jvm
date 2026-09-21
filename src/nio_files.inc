/* NIO platform operations. OpenJDK Files and adapted UnixPath execute Java. */
static void nio_error(VM *v,Object *path,int error) {
    const char *name=error==ENOENT?"java/nio/file/NoSuchFileException":
        error==EACCES||error==EPERM?"java/nio/file/AccessDeniedException":
        error==EEXIST?"java/nio/file/FileAlreadyExistsException":
        error==ENOTEMPTY?"java/nio/file/DirectoryNotEmptyException":"java/nio/file/FileSystemException";
    unsigned saved=v->nr;root(v,path);Class *type=load(v,name);initialize(v,type);if(v->exception){v->nr=saved;return;}
    Object *exception=file_instance(v,type);root(v,exception);
    Object *reason=file_string(v,strerror(error));root(v,reason);
    Value args[]={rv(exception),rv(path),rv(NULL),rv(reason)};
    int short_ctor=error==ENOTEMPTY;
    execute(v,method(type,"<init>",short_ctor?"(Ljava/lang/String;)V":"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V"),args,short_ctor?2:4);
    if(!v->exception)v->exception=exception;v->nr=saved;
}
static Object *nio_open(VM *v,Object *name,int flags,Object *display) {
    char *path=file_utf8(v,name);if(!path)return NULL;
#ifdef _TINSPIRE
    /* The pinned SDK reverses O_EXCL's existence check, and no atomic-create
     * primitive is verified. Never silently replace CREATE_NEW with truncation. */
    if((flags&2)&&(flags&8)) {release(v,path);file_error(v,"java/lang/UnsupportedOperationException","Ndless atomic CREATE_NEW is not implemented");return NULL;}
    if((flags&(64|128|256|512))||((flags&3)==3&&(flags&16))) {
        release(v,path);file_error(v,"java/lang/UnsupportedOperationException","Ndless open option is not implemented");return NULL;
    }
#endif
    unsigned saved=v->nr;Class *type=load(v,"java/io/FileDescriptor");initialize(v,type);
    if(v->exception){release(v,path);return NULL;}
    Object *descriptor=file_instance(v,type);root(v,descriptor);Value ctor=rv(descriptor);
    execute(v,method(type,"<init>","()V"),&ctor,1);
    if(v->exception){v->nr=saved;release(v,path);return NULL;}
    int mode=(flags&3)==3?O_RDWR:(flags&2)?O_WRONLY:O_RDONLY;
#ifdef _TINSPIRE
    if((flags&2)&&!(flags&(16|32)))mode=O_RDWR; /* SDK O_WRONLY always truncates. */
#endif
    if(flags&2){if(flags&8)mode|=O_CREAT|O_EXCL;else if(flags&4)mode|=O_CREAT;if(flags&16)mode|=O_TRUNC;if(flags&32)mode|=O_APPEND;}
#ifndef _TINSPIRE
    if(flags&(64|128))mode|=O_NOFOLLOW;
    if(flags&256)mode|=O_SYNC;if(flags&512)mode|=O_DSYNC;
#endif
    struct stat info;memset(&info,0,sizeof info);
    int directory=!stat(path,&info)&&S_ISDIR(info.st_mode);
    int fd=directory?-1:open(path,mode,0666),error=directory?((flags&2)&&(flags&8)?EEXIST:EISDIR):errno;
    if(fd<0) {
        release(v,path);
#ifndef _TINSPIRE
        if(error==ELOOP&&(flags&(64|128))&&!(flags&8))file_error(v,"java/io/IOException","Symbolic link cannot be followed with NOFOLLOW_LINKS");
        else
#endif
        nio_error(v,display,error);
        v->nr=saved;return NULL;
    }
    /* Register ownership before calling any allocating Java code. */
    descriptor->file_fd=fd;descriptor->file_open=descriptor->file_owned=1;
    descriptor->data[descriptor_field(v)->slot]=iv(fd);
#ifndef _TINSPIRE
    if(fstat(fd,&info)||S_ISDIR(info.st_mode)) {
        error=S_ISDIR(info.st_mode)?EISDIR:errno;descriptor_close(v,descriptor);nio_error(v,display,error);
    } else if((flags&64)&&unlink(path)) {
        error=errno;descriptor_close(v,descriptor);nio_error(v,display,error);
    }
#endif
    release(v,path);v->nr=saved;return v->exception?NULL:descriptor;
}
static Value nio_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int *handled) {
    *handled=!strcmp(c->name,"sun/nio/fs/NspireNative");if(!*handled)return iv(0);
    if(!strcmp(name,"fileKeysSupported")&&!strcmp(desc,"()Z")) {
#ifdef _TINSPIRE
        return iv(0);
#else
        return iv(1);
#endif
    }
    if(!strcmp(name,"open")&&!strcmp(desc,"(Ljava/lang/String;ILjava/lang/String;)Ljava/io/FileDescriptor;"))return rv(nio_open(v,obj(args[0]),integer(args[1]),obj(args[2])));
    if(!strcmp(name,"position")||!strcmp(name,"size")||!strcmp(name,"truncate")) {
        Object *descriptor=nonnull(v,args[0]);if(!descriptor)return val(0,LONG);
        int fd=descriptor_number(v,descriptor);if(v->exception)return val(0,LONG);
        if(fd<0){throwing(v,"java/nio/channels/ClosedChannelException");return val(0,LONG);}
        off_t result=-1;
        if(!strcmp(name,"position")&&!strcmp(desc,"(Ljava/io/FileDescriptor;JZ)J")) {
            int64_t offset=(int64_t)args[1].bits;off_t value=(off_t)offset;
            if(integer(args[2])&&(offset<0||(int64_t)value!=offset))file_error(v,"java/io/IOException","Position outside platform range");
            else result=lseek(fd,integer(args[2])?value:0,integer(args[2])?SEEK_SET:SEEK_CUR);
        }else if(!strcmp(name,"size")&&!strcmp(desc,"(Ljava/io/FileDescriptor;)J")) {
#ifdef _TINSPIRE
            off_t current=lseek(fd,0,SEEK_CUR);if(current>=0){result=lseek(fd,0,SEEK_END);if(lseek(fd,current,SEEK_SET)<0)result=-1;}
#else
            struct stat st;if(!fstat(fd,&st))result=st.st_size;
#endif
        }else if(!strcmp(name,"truncate")&&!strcmp(desc,"(Ljava/io/FileDescriptor;J)V")) {
#ifdef _TINSPIRE
            file_error(v,"java/lang/UnsupportedOperationException","Ndless channel truncation is not implemented");
#else
            int64_t size=(int64_t)args[1].bits;off_t value=(off_t)size;struct stat st;
            if(size<0||(int64_t)value!=size)file_error(v,"java/io/IOException","Size outside platform range");
            else if(!fstat(fd,&st)) {
                result=0;if(value<st.st_size) {
                    off_t position=lseek(fd,0,SEEK_CUR);
                    if(position<0||ftruncate(fd,value))result=-1;
                    else if(position>value&&lseek(fd,value,SEEK_SET)<0)result=-1;
                }
            }
#endif
        }else fail(v,"unbound NIO descriptor method: %s%s",name,desc);
        if(result<0&&!v->exception)file_error(v,"java/io/IOException",strerror(errno));
        return val((uint64_t)result,LONG);
    }
    Object *text=obj(args[0]),*display=obj(args[!strcmp(name,"mkdir")||!strcmp(name,"realPath")?1:2]);char *path=file_utf8(v,text);if(!path)return iv(0);Value result=iv(0);int error=0;
    if(!strcmp(name,"delete")&&!strcmp(desc,"(Ljava/lang/String;ZLjava/lang/String;)Z")) {
        if(!remove(path))result=iv(1);else if(errno!=ENOENT||!integer(args[1]))error=errno;
    }else if(!strcmp(name,"mkdir")&&!strcmp(desc,"(Ljava/lang/String;Ljava/lang/String;)V")) {if(mkdir(path,0777))error=errno;}
    else if(!strcmp(name,"access")&&!strcmp(desc,"(Ljava/lang/String;ILjava/lang/String;)V")) {
#ifdef _TINSPIRE
        if(integer(args[1]))file_error(v,"java/lang/UnsupportedOperationException","Ndless access modes are not implemented");
        else {struct stat st;if(stat(path,&st))error=errno;}
#else
        if(access(path,integer(args[1])))error=errno;
#endif
    }else if(!strcmp(name,"attributes")&&!strcmp(desc,"(Ljava/lang/String;ZLjava/lang/String;)[J")) {
        struct stat st;memset(&st,0,sizeof st);int status;
#ifdef _TINSPIRE
        if(!integer(args[1])){file_error(v,"java/lang/UnsupportedOperationException","Ndless link attributes are not implemented");status=-1;}else status=stat(path,&st);
#else
        status=integer(args[1])?stat(path,&st):lstat(path,&st);
#endif
        if(status)error=errno;
        else {
            int kind=S_ISREG(st.st_mode)?1:S_ISDIR(st.st_mode)?2:S_ISLNK(st.st_mode)?4:8;
            Object *data=array_new(v,"[J",8);data->data[0]=val(kind,LONG);data->data[1]=val((uint64_t)st.st_size,LONG);
#if defined(_TINSPIRE) && (__NEWLIB__ < 3 || (__NEWLIB__ == 3 && __NEWLIB_MINOR__ < 2))
            data->data[2]=val((uint64_t)st.st_mtime,LONG);data->data[4]=val((uint64_t)st.st_atime,LONG);
#else
            data->data[2]=val((uint64_t)st.st_mtim.tv_sec,LONG);data->data[4]=val((uint64_t)st.st_atim.tv_sec,LONG);
#endif
#ifndef _TINSPIRE
            data->data[3]=val(st.st_mtim.tv_nsec,LONG);data->data[5]=val(st.st_atim.tv_nsec,LONG);
#endif
            data->data[6]=val(st.st_dev,LONG);data->data[7]=val(st.st_ino,LONG);result=rv(data);
        }
    }else if(!strcmp(name,"realPath")&&!strcmp(desc,"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;")) {
        struct stat st;char resolved[PATH_MAX+1];
        if(stat(path,&st)||canonicalize(path,resolved,sizeof resolved))error=errno;
        else result=rv(file_string(v,resolved));
    }else fail(v,"unbound NIO path method: %s%s",name,desc);
    release(v,path);if(error&&!v->exception)nio_error(v,display,error);return result;
}
