/* Platform timezone discovery and CRC32 used by the original TZDB decoder.
 * Time arithmetic, rules and formatting execute the OpenJDK Java code. */
#include "strictmath.h"
static Object *system_offset(VM *v) {
#ifdef _TINSPIRE
    /* No device timezone service is exposed. Its configured default is UTC. */
    (void)v;return NULL;
#else
    time_t now=time(NULL);struct tm local;char offset[32],id[40];tzset();
    if(!localtime_r(&now,&local)||strftime(offset,sizeof offset,"%z",&local)!=5)return NULL;
    if(offset[0]!='+'&&offset[0]!='-')return NULL;
    for(unsigned i=1;i<5;i++)if(offset[i]<'0'||offset[i]>'9')return NULL;
    snprintf(id,sizeof id,"GMT%c%c%c:%c%c",offset[0],offset[1],offset[2],offset[3],offset[4]);return string(v,id);
#endif
}
static Object *system_timezone(VM *v) {
#ifdef _TINSPIRE
    (void)v;return NULL;
#else
    const char *zone=getenv("TZ");
    if(zone){if(*zone==':')zone++;const char *part=strstr(zone,"zoneinfo/");if(part)zone=part+9;return string(v,*zone?zone:"GMT");}
    char path[PATH_MAX+1];ssize_t len=readlink("/etc/localtime",path,sizeof path-1);
    if(len>0){path[len]=0;const char *part=strstr(path,"zoneinfo/");if(part)return string(v,part+9);}
    FILE *file=fopen("/etc/timezone","rb");int found=0;
    if(file){found=fgets(path,sizeof path,file)!=NULL;fclose(file);}
    if(found){path[strcspn(path,"\r\n \t")]=0;if(*path)return string(v,path);}
    return system_offset(v);
#endif
}
static int platform_native_bound(Method *method) {
    if(!(method->flags&NATIVE))return 0;const char *owner=method->owner->name;
    return !strcmp(owner,"sun/nio/fs/NspireNative")||!strcmp(owner,"java/io/UnixFileSystem")||!strcmp(owner,"java/util/TimeZone")||!strcmp(owner,"java/util/zip/CRC32")||
        (!strcmp(owner,"java/lang/Shutdown")&&(method->flags&STATIC)&&
            ((!strcmp(method->name,"beforeHalt")&&!strcmp(method->desc,"()V"))||
             (!strcmp(method->name,"halt0")&&!strcmp(method->desc,"(I)V"))))||
        !strcmp(owner,"java/io/FileDescriptor")||!strcmp(owner,"java/io/FileInputStream")||!strcmp(owner,"java/io/FileOutputStream")||
        (!strcmp(owner,"java/util/concurrent/atomic/AtomicLong")&&!strcmp(method->name,"VMSupportsCS8")&&!strcmp(method->desc,"()Z")&&(method->flags&STATIC))||
        (!strcmp(owner,"nspire/xml/ExpatReader")&&!strcmp(method->name,"parse0")&&!strcmp(method->desc,"(Lorg/xml/sax/InputSource;ZZ)V")&&!(method->flags&STATIC))||
        (!strcmp(owner,"java/lang/StrictMath")&&(!strcmp(method->name,"sqrt")||!strcmp(method->name,"log"))&&!strcmp(method->desc,"(D)D"));
}
static Value time_native(VM *v,Class *c,const char *name,const char *desc,Value *a,int stat,int *handled) {
    *handled=0;Value result=iv(0);if(!stat)return result;
    if(!strcmp(c->name,"java/util/TimeZone")) {
        if(!strcmp(name,"getSystemTimeZoneID")&&!strcmp(desc,"(Ljava/lang/String;)Ljava/lang/String;")){*handled=1;return rv(system_timezone(v));}
        if(!strcmp(name,"getSystemGMTOffsetID")&&!strcmp(desc,"()Ljava/lang/String;")){*handled=1;return rv(system_offset(v));}
    }
    if(!strcmp(c->name,"java/lang/StrictMath")&&!strcmp(desc,"(D)D")) {
        if(!strcmp(name,"sqrt")){*handled=1;return dv(nspire_fdlibm_sqrt(dbl(a[0])));}
        if(!strcmp(name,"log")){*handled=1;return dv(nspire_fdlibm_log(dbl(a[0])));}
    }
    if(!strcmp(c->name,"java/util/zip/CRC32")) {
        uint32_t crc=(uint32_t)a[0].bits;
        if(!strcmp(name,"update")&&!strcmp(desc,"(II)I")){unsigned char byte=(unsigned char)integer(a[1]);*handled=1;return iv((int32_t)mz_crc32(crc,&byte,1));}
        if(!strcmp(name,"updateBytes")&&!strcmp(desc,"(I[BII)I")){
            *handled=1;Object *bytes=nonnull(v,a[1]);if(!bytes)return result;
            if(bytes->kind!='a'||strcmp(bytes->array_desc,"[B"))fail(v,"CRC32 requires byte[]");
            int32_t off=integer(a[2]),len=integer(a[3]);
            if(off<0||len<0||(size_t)off>bytes->count||(size_t)len>bytes->count-(size_t)off){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return result;}
            unsigned char buffer[512];while(len){int count=len>(int)sizeof buffer?(int)sizeof buffer:len;
                for(int i=0;i<count;i++)buffer[i]=(unsigned char)integer(bytes->data[off+i]);
                crc=(uint32_t)mz_crc32(crc,buffer,(size_t)count);off+=count;len-=count;
            }return iv((int32_t)crc);
        }
    }
    return result;
}
