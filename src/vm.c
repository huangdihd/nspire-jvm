/* Nspire JVM: small, portable interpreter. MIT license; see LICENSE.
 * It deliberately fails on missing runtime features instead of faking them.
 * Class files must be trusted: this release is not a bytecode security sandbox.
 */
#include "vm.h"
#include "context.h"
#include "miniz.h"
#include "expat.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CLASSES 512
#define MAX_DEPTH 128
#define CLASS_LIMIT (2U * 1024U * 1024U)
#define META_LIMIT (16U * 1024U * 1024U)
#define STATIC 0x0008
#define NATIVE 0x0100
typedef struct Class Class;
typedef struct Object Object;
typedef struct Method Method;
typedef struct VM VM;
typedef struct VmThread VmThread;
typedef struct XmlParse XmlParse;
typedef struct XmlMem XmlMem;
typedef struct LocalEntry { struct LocalEntry *next;Object *key,*value; } LocalEntry;
typedef struct Property { struct Property *next;char *key,*value,*initial; } Property;
typedef struct { uint64_t bits; unsigned char tag; } Value;
enum { INT=1, LONG, FLOAT, DOUBLE, REF };
typedef struct Mem { struct Mem *next; size_t size; } Mem;
typedef struct { uint8_t tag; uint16_t a, b; uint64_t bits; char *text; Object *intern; Class *lambda_class; } CP;
typedef struct { uint16_t start, end, handler, type; } Handler;
typedef struct { uint16_t handle,nargs;uint16_t *args; } Bootstrap;
typedef struct {
    char *name, *desc; uint16_t flags, constant; size_t slot; Value value;
} Field;
struct Method {
    Class *owner; char *name, *desc; uint16_t flags, locals, stack;
    unsigned char *code; uint32_t length; Handler *handlers; uint16_t nh;
};
struct Class {
    char *name; Class *super; CP *cp; uint16_t nc, nf, nm;
    Field *fields; Method *methods; size_t slots; int builtin, init, loading;
    Class **interfaces; uint16_t ni;
    Object *mirror,*enum_constants,*enum_directory; Class *component;
    char *simple_name,*declaring_name; uint16_t access;int local_class;
    unsigned primitive;
    VmThread *init_owner;
    int app_loader;
    Bootstrap *bootstraps;unsigned nbootstraps;
};
struct Object {
    Object *next, *grey; Class *cls; unsigned mark; char kind;
    size_t count, bytes; Value *data; char *text; char *array_desc;
    Class *represented;
    Field *represented_field;unsigned interned;
    Method *represented_method;int accessible;
    VmThread *thread, *monitor_owner; unsigned monitor_depth;
    Object *thread_target;
    Object *cause;
    unsigned char *buffer;size_t buffer_size,cursor,mark_cursor;
    int resource_path,resource_index,closed,pending,skip_lf;
    char *resource_name;
};
typedef struct Frame {
    struct Frame *prev; Method *method; Value *locals, *stack;
    size_t sp; uint32_t pc, ip;
    Object *method_lock;
} Frame;
enum { T_NEW,T_RUN,T_MONITOR,T_WAIT,T_SLEEP,T_JOIN,T_CLASS,T_DRAIN,T_PARK,T_DONE };
struct VmThread {
    VmThread *next;VM *vm;VmContext context;void *stack;
    Object *object,*waiting,*exception;Class *waiting_class;VmThread *joining;
    Frame *frame;Object *roots[256];unsigned nr;int depth,state,interrupted,daemon;
    unsigned id;uint64_t deadline;int finished,permit;
    LocalEntry *locals_map;
    Object *context_loader;int context_loader_set;
};
typedef struct { char *path; mz_zip_archive zip; int is_zip; } ClassPath;
struct VM {
    VmOptions opt; jmp_buf abort; Mem *mem; size_t metadata;
    Class *classes[MAX_CLASSES]; int nclasses, depth;
    Object *objects, *exception; size_t heap, threshold; Frame *frame;
    Object *roots[256]; unsigned nr;
    ClassPath paths[32]; unsigned npaths,nbootpaths; uint64_t steps;
    VmThread *threads,*current,*main_thread;unsigned next_thread_id,live_threads;int fatal;
    Object *unsafe_instance,*runtime_instance;
    Object *app_loader,*java_lang_access;
    Object *integer_cache[256],*long_cache[256];
    Property *properties;
    XmlParse *xml_parsers;XmlMem *xml_mem;size_t xml_bytes;
};
static void abort_vm(VM *v);
static void fail(VM *v, const char *fmt, ...) {
    va_list a; va_start(a, fmt); fputs("VM error: ", stderr); vfprintf(stderr, fmt, a);
    va_end(a); fputc('\n', stderr);
    for (Frame *f=v->frame; f; f=f->prev)
        fprintf(stderr, "  at %s.%s%s pc=%u\n", f->method->owner->name,
                f->method->name, f->method->desc, (unsigned)f->ip);
    abort_vm(v);
}
static void *alloc(VM *v, size_t n) {
    if (n > META_LIMIT || v->metadata > META_LIMIT-n) fail(v, "metadata limit exceeded");
    Mem *m=(Mem *)calloc(1, sizeof(Mem)+n);
    if (!m) fail(v, "out of native memory");
    m->size=n; m->next=v->mem; v->mem=m; v->metadata+=n;
    return m+1;
}
static void release(VM *v, void *p) {
    if (!p) return;
    Mem *m=((Mem *)p)-1, **q=&v->mem;
    while (*q && *q!=m) q=&(*q)->next;
    if (*q) { *q=m->next; v->metadata-=m->size; free(m); }
}
static void clear_locals(VM *v,VmThread *t) {
    while(t->locals_map){LocalEntry *e=t->locals_map;t->locals_map=e->next;release(v,e);}
}
static char *copy(VM *v, const char *s) {
    size_t n=strlen(s)+1; char *r=(char *)alloc(v,n); memcpy(r,s,n); return r;
}
static Property *property(VM *v,const char *key,int create) {
    for(Property *p=v->properties;p;p=p->next)if(!strcmp(p->key,key))return p;
    if(!create)return NULL;
    Property *p=(Property *)alloc(v,sizeof *p);p->key=copy(v,key);p->next=v->properties;v->properties=p;return p;
}
static void init_properties(VM *v) {
    const char *pairs[]={"java.vm.name","Nspire JVM","java.vm.version","0.1",
        "java.vm.vendor","Nspire JVM contributors","java.class.version","61.0",
        "file.separator","/","path.separator",";","line.separator","\n","file.encoding","UTF-8",
        "user.language","en","user.country","",
#ifdef _TINSPIRE
        "os.name","Ndless","os.arch","arm",
#else
        "os.name","Linux",
#if UINTPTR_MAX > UINT32_MAX
        "os.arch","amd64",
#else
        "os.arch","x86",
#endif
#endif
        NULL};
    for(unsigned i=0;pairs[i];i+=2){Property *p=property(v,pairs[i],1);p->value=copy(v,pairs[i+1]);p->initial=copy(v,pairs[i+1]);}
}
static Value val(uint64_t x, int t) { Value a; a.bits=x; a.tag=(unsigned char)t; return a; }
static Value iv(int32_t x) { return val((uint32_t)x,INT); }
static Value rv(Object *o) { return val((uintptr_t)o,REF); }
static Object *obj(Value x) { return (Object *)(uintptr_t)x.bits; }
static int32_t integer(Value x) { return (int32_t)(uint32_t)x.bits; }
static float flt(Value x) { uint32_t u=(uint32_t)x.bits; float f; memcpy(&f,&u,4); return f; }
static double dbl(Value x) { double d; memcpy(&d,&x.bits,8); return d; }
static Value fv(float f) { uint32_t u; memcpy(&u,&f,4); return val(u,FLOAT); }
static Value dv(double d) { uint64_t u; memcpy(&u,&d,8); return val(u,DOUBLE); }
static int wide(Value x) { return x.tag==LONG || x.tag==DOUBLE; }
static Value zero(const char *d) {
    return val(0,*d=='J'?LONG:*d=='D'?DOUBLE:*d=='F'?FLOAT:(*d=='L'||*d=='[')?REF:INT);
}
static void mark(Object *o, Object **grey) {
    if (o && !o->mark) { o->mark=1; o->grey=*grey; *grey=o; }
}
static void collect(VM *v) {
    Object *grey=NULL;
    mark(v->exception,&grey);
    mark(v->unsafe_instance,&grey);mark(v->runtime_instance,&grey);
    mark(v->app_loader,&grey);mark(v->java_lang_access,&grey);
    for(unsigned i=0;i<256;i++){mark(v->integer_cache[i],&grey);mark(v->long_cache[i],&grey);}
    for (unsigned i=0;i<v->nr;i++) mark(v->roots[i],&grey);
    for(VmThread *t=v->threads;t;t=t->next)if(t->state!=T_NEW&&t->state!=T_DONE) {
        mark(t->object,&grey);mark(t->waiting,&grey);
        if(t->joining)mark(t->joining->object,&grey);
        if(t!=v->current) {
            mark(t->exception,&grey);
            for(unsigned i=0;i<t->nr;i++)mark(t->roots[i],&grey);
            for(Frame *f=t->frame;f;f=f->prev) {
                mark(f->method_lock,&grey);
                for(unsigned i=0;i<f->method->locals;i++)if(f->locals[i].tag==REF)mark(obj(f->locals[i]),&grey);
                for(size_t i=0;i<f->sp;i++)if(f->stack[i].tag==REF)mark(obj(f->stack[i]),&grey);
            }
        }
    }
    for(Object *o=v->objects;o;o=o->next)if(o->monitor_owner)mark(o,&grey);
    for (Frame *f=v->frame;f;f=f->prev) {
        mark(f->method_lock,&grey);
        for (unsigned i=0;i<f->method->locals;i++) if(f->locals[i].tag==REF) mark(obj(f->locals[i]),&grey);
        for (size_t i=0;i<f->sp;i++) if(f->stack[i].tag==REF) mark(obj(f->stack[i]),&grey);
    }
    for (int i=0;i<v->nclasses;i++) {
        Class *c=v->classes[i];
        mark(c->mirror,&grey);mark(c->enum_constants,&grey);mark(c->enum_directory,&grey);
        for (unsigned j=1;j<c->nc;j++) mark(c->cp[j].intern,&grey);
        for (unsigned j=0;j<c->nf;j++) if(c->fields[j].value.tag==REF) mark(obj(c->fields[j].value),&grey);
    }
    while (grey) {
        Object *o=grey; grey=o->grey;
        mark(o->thread_target,&grey);
        mark(o->cause,&grey);
        if(o->thread)mark(o->thread->context_loader,&grey);
        if(o->thread)for(LocalEntry *e=o->thread->locals_map;e;e=e->next)mark(e->value,&grey);
        for(size_t i=0;i<o->count;i++) if(o->data[i].tag==REF) mark(obj(o->data[i]),&grey);
    }
    /* ThreadLocal keys are weak. Retain values for reachable Thread objects,
     * then discard stale entries before their key objects are freed. */
    for(VmThread *t=v->threads;t;t=t->next) {
        LocalEntry **entry=&t->locals_map;
        while(*entry){LocalEntry *e=*entry;if(!e->key->mark){*entry=e->next;release(v,e);}else entry=&e->next;}
    }
    Object **p=&v->objects;
    while(*p) {
        Object *o=*p;
        if(o->mark) { o->mark=0; p=&o->next; }
        else { *p=o->next; v->heap-=o->bytes; if(o->thread)o->thread->object=NULL; free(o->data); free(o->text); free(o->array_desc); free(o->buffer);free(o->resource_name);free(o); }
    }
    VmThread **tp=&v->threads;
    while(*tp) {
        VmThread *t=*tp;
        if(t!=v->current&&t!=v->main_thread&&!t->object&&(t->state==T_NEW||t->state==T_DONE)) {
            *tp=t->next;clear_locals(v,t);release(v,t->stack);release(v,t);
        } else tp=&t->next;
    }
    v->threshold=v->heap+v->heap/2+65536;
    if(v->threshold>v->opt.heap_limit) v->threshold=v->opt.heap_limit;
}
static void heap_room(VM *v, size_t n) {
    if(n>v->opt.heap_limit) fail(v,"Java heap exhausted (%zu bytes requested)",n);
    if(v->heap+n>v->threshold) collect(v);
    if(v->heap>v->opt.heap_limit-n) fail(v,"Java heap exhausted (limit %zu)",v->opt.heap_limit);
}
static Object *new_object(VM *v, Class *cls, char kind, size_t count) {
    if(count>v->opt.heap_limit/sizeof(Value)) fail(v,"array/object too large");
    size_t n=sizeof(Object)+(count?count:1)*sizeof(Value); heap_room(v,n);
    Object *o=(Object *)calloc(1,sizeof(Object));
    if(!o) fail(v,"out of native memory");
    o->data=(Value *)calloc(count?count:1,sizeof(Value));
    if(!o->data) { free(o); fail(v,"out of native memory"); }
    o->cls=cls; o->kind=kind; o->count=count; o->bytes=n;
    o->next=v->objects; v->objects=o; v->heap+=n;
    return o;
}
static void root(VM *v,Object *o) { if(v->nr==256) fail(v,"native root stack overflow"); v->roots[v->nr++]=o; }
static void set_text(VM *v,Object *o,const char *s) {
    size_t n=strlen(s)+1, old=o->text?strlen(o->text)+1:0;
    root(v,o); heap_room(v,n); v->nr--;
    char *t=(char *)malloc(n); if(!t) fail(v,"out of native memory"); memcpy(t,s,n);
    free(o->text); o->text=t; o->bytes+=n-old; v->heap+=n; v->heap-=old;
}
typedef struct { VM *v; unsigned char *p; size_t n,pos; } Reader;
static uint32_t readn(Reader *r,unsigned n) {
    if(n>r->n-r->pos) fail(r->v,"truncated class file");
    uint32_t a=0; while(n--) a=(a<<8)|r->p[r->pos++]; return a;
}
static void skip(Reader *r,size_t n) { if(n>r->n-r->pos) fail(r->v,"truncated attribute"); r->pos+=n; }
static CP *cp(VM *v,Class *c,unsigned i,int tag) {
    if(!i||i>=c->nc||(tag&&c->cp[i].tag!=tag)) fail(v,"bad constant pool entry %u in %s",i,c->name);
    return &c->cp[i];
}
static char *utf(VM *v,Class *c,unsigned i) { return cp(v,c,i,1)->text; }
static char *classname(VM *v,Class *c,unsigned i) { return utf(v,c,cp(v,c,i,7)->a); }
static Class *load(VM *,const char *);
static void initialize(VM *,Class *);
static Value execute(VM *,Method *,Value *,unsigned);
static Value native_call(VM *,Class *,const char *,const char *,Value *,unsigned,int);
static Object *array_new(VM *,const char *,int32_t);
static void schedule(VM *);
static int monitor_enter(VM *,Object *);
static void monitor_exit(VM *,Object *);
static Object *class_mirror(VM *v,Class *c) {
    if(!c->mirror) {
        Class *meta=load(v,"java/lang/Class");
        Object *o=new_object(v,meta,'c',0);
        o->represented=c; c->mirror=o;
    }
    return c->mirror;
}
static Object *string(VM *v,const char *s) {
    Class *c=load(v,"java/lang/String"); Object *o=new_object(v,c,'s',0); set_text(v,o,s); return o;
}
static Value constant(VM *v,Class *c,unsigned idx) {
    CP *p=cp(v,c,idx,0);
    if(p->tag==3) return val(p->bits,INT);
    if(p->tag==4) return val(p->bits,FLOAT);
    if(p->tag==5) return val(p->bits,LONG);
    if(p->tag==6) return val(p->bits,DOUBLE);
    if(p->tag==7) return rv(class_mirror(v,load(v,utf(v,c,p->a))));
    if(p->tag==8) {
        const char *s=utf(v,c,p->a);
        if(!p->intern) {
            for(Object *o=v->objects;o;o=o->next)if(o->interned&&!strcmp(o->text,s)){p->intern=o;break;}
            if(!p->intern) {p->intern=string(v,s);p->intern->interned=1;}
        }
        return rv(p->intern);
    }
    fail(v,"unsupported ldc constant tag %u (MethodHandle/MethodType/condy not implemented)",p->tag); return iv(0);
}
static const char *primitive_name(char code) {
    switch(code) {
    case 'Z':return "boolean";case 'B':return "byte";case 'C':return "char";
    case 'S':return "short";case 'I':return "int";case 'J':return "long";
    case 'F':return "float";case 'D':return "double";case 'V':return "void";
    default:return NULL;
    }
}
static const char *wrapper_primitive(const char *name) {
    const char *wrappers[]={"Boolean","Byte","Character","Short","Integer","Long","Float","Double","Void"};
    const char *codes="ZBCSIJFDV";
    if(strncmp(name,"java/lang/",10))return NULL;
    for(unsigned i=0;i<sizeof wrappers/sizeof *wrappers;i++)if(!strcmp(name+10,wrappers[i]))return primitive_name(codes[i]);
    return NULL;
}
static const char *builtin_super(const char *n) {
    if(!strcmp(n,"java/lang/Object")) return "";
    if(!strcmp(n,"java/lang/AutoCloseable")||!strcmp(n,"java/io/Closeable")||!strcmp(n,"java/net/URLConnection"))return "java/lang/Object";
    if(!strcmp(n,"java/net/JarURLConnection")||!strcmp(n,"nspire/FileConnection"))return "java/net/URLConnection";
    if(!strcmp(n,"nspire/JarConnection"))return "java/net/JarURLConnection";
    if(!strcmp(n,"nspire/ResourceInputStream"))return "java/io/InputStream";
    if(!strcmp(n,"java/net/UnknownServiceException"))return "java/io/IOException";
    if(!strcmp(n,"java/lang/reflect/AccessibleObject"))return "java/lang/Object";
    if(!strcmp(n,"java/lang/reflect/Executable"))return "java/lang/reflect/AccessibleObject";
    if(!strcmp(n,"java/lang/reflect/Constructor"))return "java/lang/reflect/Executable";
    if(!strcmp(n,"java/lang/reflect/AnnotatedElement")||!strcmp(n,"java/lang/reflect/GenericDeclaration")||!strcmp(n,"java/lang/reflect/Member"))return "java/lang/Object";
    if(!strcmp(n,"java/lang/ReflectiveOperationException"))return "java/lang/Exception";
    if(!strcmp(n,"java/lang/NoSuchMethodException")||!strcmp(n,"java/lang/reflect/InvocationTargetException"))return "java/lang/ReflectiveOperationException";
    if(!strcmp(n,"java/lang/Comparable")||!strcmp(n,"java/lang/CharSequence"))return "java/lang/Object";
    if(!strcmp(n,"sun/misc/SharedSecrets")||!strcmp(n,"sun/misc/JavaLangAccess")||!strcmp(n,"nspire/JavaLangAccess"))return "java/lang/Object";
    const char *io_plain[]={"java/lang/ClassLoader","java/net/URL","java/io/InputStream","java/io/Reader","nspire/ResourceEnumeration","java/security/AccessController",NULL};
    for(unsigned i=0;io_plain[i];i++)if(!strcmp(n,io_plain[i]))return "java/lang/Object";
    if(!strcmp(n,"java/io/ByteArrayInputStream"))return "java/io/InputStream";
    if(!strcmp(n,"java/io/InputStreamReader")||!strcmp(n,"java/io/BufferedReader"))return "java/io/Reader";
    if(!strcmp(n,"java/io/IOException"))return "java/lang/Exception";
    if(!strcmp(n,"java/lang/InstantiationException")||!strcmp(n,"java/lang/IllegalAccessException"))return "java/lang/ReflectiveOperationException";
    if(!strcmp(n,"java/io/FileNotFoundException")||!strcmp(n,"java/io/UnsupportedEncodingException"))return "java/io/IOException";
    if(!strcmp(n,"sun/misc/Unsafe")||!strcmp(n,"sun/misc/VM")||!strcmp(n,"java/lang/Runtime")||!strcmp(n,"java/lang/reflect/Field")||!strcmp(n,"java/lang/reflect/Array"))return "java/lang/Object";
    if(!strcmp(n,"java/lang/ThreadLocal"))return "java/lang/Object";
    if(!strcmp(n,"java/lang/InheritableThreadLocal"))return "java/lang/ThreadLocal";
    const char *plain[]={"java/lang/Thread","java/lang/Runnable","java/lang/Class","java/lang/Cloneable","java/io/Serializable","java/lang/String","java/lang/StringBuilder","java/lang/System","java/io/PrintStream","java/lang/Math","java/lang/Number","java/lang/Boolean","java/lang/Character","java/lang/Void","java/lang/Throwable",NULL};
    for(int i=0;plain[i];i++) if(!strcmp(n,plain[i])) return "java/lang/Object";
    if(wrapper_primitive(n))return "java/lang/Number";
    if(!strcmp(n,"java/lang/ClassNotFoundException"))return "java/lang/ReflectiveOperationException";
    if(!strcmp(n,"java/lang/NoSuchFieldException"))return "java/lang/ReflectiveOperationException";
    if(!strcmp(n,"java/lang/CloneNotSupportedException"))return "java/lang/Exception";
    if(!strcmp(n,"java/lang/Error"))return "java/lang/Throwable";
    if(!strcmp(n,"java/lang/LinkageError"))return "java/lang/Error";
    if(!strcmp(n,"java/lang/IncompatibleClassChangeError"))return "java/lang/LinkageError";
    if(!strcmp(n,"java/lang/VirtualMachineError"))return "java/lang/Error";
    if(!strcmp(n,"java/lang/OutOfMemoryError"))return "java/lang/VirtualMachineError";
    if(!strcmp(n,"java/lang/AssertionError")||!strcmp(n,"java/lang/InternalError"))return "java/lang/Error";
    if(!strcmp(n,"java/lang/InterruptedException"))return "java/lang/Exception";
    if(!strcmp(n,"java/lang/IllegalThreadStateException"))return "java/lang/IllegalArgumentException";
    if(!strcmp(n,"java/lang/IllegalMonitorStateException"))return "java/lang/RuntimeException";
    if(!strcmp(n,"java/lang/IllegalStateException"))return "java/lang/RuntimeException";
    if(!strcmp(n,"java/lang/Exception")) return "java/lang/Throwable";
    if(!strcmp(n,"java/lang/RuntimeException")) return "java/lang/Exception";
    if(!strcmp(n,"java/lang/IndexOutOfBoundsException")) return "java/lang/RuntimeException";
    if(!strcmp(n,"java/lang/ArrayIndexOutOfBoundsException")||!strcmp(n,"java/lang/StringIndexOutOfBoundsException")) return "java/lang/IndexOutOfBoundsException";
    const char *runtime[]={"java/lang/NullPointerException","java/lang/ArithmeticException","java/lang/NegativeArraySizeException","java/lang/ClassCastException","java/lang/ArrayStoreException","java/lang/IllegalArgumentException","java/lang/UnsupportedOperationException",NULL};
    for(int i=0;runtime[i];i++) if(!strcmp(n,runtime[i])) return "java/lang/RuntimeException";
    return NULL;
}
static int valid_name(const char *name) {
    if(!*name||*name=='/'||strstr(name,"..")||strchr(name,'\\')||strchr(name,':')) return 0;
    return strlen(name)<480;
}
static void open_paths(VM *v,const char *list) {
    if(!list||!*list)return;
    const char *p=list;
    while(*p) {
        const char *end=strchr(p,';');size_t n=end?(size_t)(end-p):strlen(p);
        if(!n)fail(v,"empty classpath entry");
        if(v->npaths==32)fail(v,"classpath exceeds 32 entries");
        ClassPath *entry=&v->paths[v->npaths++];entry->path=(char *)alloc(v,n+1);memcpy(entry->path,p,n);
        FILE *f=fopen(entry->path,"rb");
        if(f) {
            unsigned char sig[4];size_t got=fread(sig,1,4,f);fclose(f);
            if(got==4&&sig[0]=='P'&&sig[1]=='K') {
                if(!mz_zip_reader_init_file(&entry->zip,entry->path,0))fail(v,"invalid classpath JAR: %s",entry->path);
                entry->is_zip=1;
            }
        }
        if(!end)break;p=end+1;
        if(!*p)fail(v,"empty classpath entry");
    }
}
static ClassPath *find_class(VM *v,const char *name,int *index,char path[1024]) {
    if(!valid_name(name))return NULL;
    for(unsigned i=0;i<v->npaths;i++) {
        ClassPath *entry=&v->paths[i];
        if(entry->is_zip) {
            snprintf(path,1024,"%s.class",name);
            *index=mz_zip_reader_locate_file(&entry->zip,path,NULL,0);
            if(*index>=0)return entry;
        } else {
            if(snprintf(path,1024,"%s/%s.class",entry->path,name)>=1024)fail(v,"class path too long");
            FILE *f=fopen(path,"rb");if(f){fclose(f);*index=-1;return entry;}
        }
    }
    return NULL;
}
static unsigned char *class_bytes(VM *v,Class *cls,const char *name,size_t *len) {
    char path[1024]; if(!valid_name(name)) fail(v,"invalid class name: %s",name);
    int i;ClassPath *entry=find_class(v,name,&i,path);
    if(!entry)fail(v,"class not found: %s",name);
    cls->app_loader=(unsigned)(entry-v->paths)>=v->nbootpaths;
    if(entry->is_zip) {
        mz_zip_archive_file_stat st;
        if(i<0) fail(v,"class not found: %s (minimal built-in library only)",name);
        if(!mz_zip_reader_file_stat(&entry->zip,(mz_uint)i,&st)||st.m_uncomp_size>CLASS_LIMIT) fail(v,"invalid/oversized class: %s",name);
        *len=(size_t)st.m_uncomp_size; unsigned char *b=(unsigned char *)alloc(v,*len);
        if(!mz_zip_reader_extract_to_mem(&entry->zip,(mz_uint)i,b,*len,0)) fail(v,"cannot decompress class: %s",name);
        return b;
    }
    FILE *f=fopen(path,"rb"); if(!f) fail(v,"class not found: %s",name);
    if(fseek(f,0,SEEK_END)) { fclose(f); fail(v,"cannot seek class"); }
    long n=ftell(f);
    if(n<0||(unsigned long)n>CLASS_LIMIT) { fclose(f); fail(v,"invalid class size"); }
    rewind(f); *len=(size_t)n; unsigned char *b=(unsigned char *)alloc(v,*len);
    size_t got=fread(b,1,*len,f); fclose(f); if(got!=*len) fail(v,"cannot read class"); return b;
}
static void attributes(Reader *r,Class *c,Method *m,Field *f) {
    unsigned n=readn(r,2);
    while(n--) {
        char *name=utf(r->v,c,readn(r,2)); uint32_t len=readn(r,4);
        if(len>r->n-r->pos) fail(r->v,"truncated attribute %s",name);
        size_t end=r->pos+len;
        if(m&&!strcmp(name,"Code")) {
            if(m->code) fail(r->v,"duplicate Code attribute");
            m->stack=(uint16_t)readn(r,2); m->locals=(uint16_t)readn(r,2); m->length=readn(r,4);
            if(!m->length||m->length>65535) fail(r->v,"invalid code length");
            m->code=r->p+r->pos; skip(r,m->length); m->nh=(uint16_t)readn(r,2);
            m->handlers=(Handler *)alloc(r->v,m->nh*sizeof(Handler));
            for(unsigned i=0;i<m->nh;i++) {
                Handler *h=&m->handlers[i]; h->start=(uint16_t)readn(r,2); h->end=(uint16_t)readn(r,2);
                h->handler=(uint16_t)readn(r,2); h->type=(uint16_t)readn(r,2);
                if(h->start>=h->end||h->end>m->length||h->handler>=m->length) fail(r->v,"invalid exception table");
            }
            attributes(r,c,NULL,NULL);
        } else if(f&&!strcmp(name,"ConstantValue")) f->constant=(uint16_t)readn(r,2);
        else if(!m&&!f&&!strcmp(name,"BootstrapMethods")) {
            if(c->bootstraps)fail(r->v,"duplicate BootstrapMethods attribute");
            c->nbootstraps=readn(r,2);c->bootstraps=(Bootstrap *)alloc(r->v,c->nbootstraps*sizeof(Bootstrap));
            for(unsigned i=0;i<c->nbootstraps;i++) {
                Bootstrap *b=&c->bootstraps[i];b->handle=(uint16_t)readn(r,2);b->nargs=(uint16_t)readn(r,2);
                b->args=(uint16_t *)alloc(r->v,b->nargs*sizeof(uint16_t));
                for(unsigned j=0;j<b->nargs;j++)b->args[j]=(uint16_t)readn(r,2);
            }
        }
        else if(!m&&!f&&!strcmp(name,"InnerClasses")) {
            unsigned entries=readn(r,2);
            while(entries--) {
                unsigned inner=readn(r,2),outer=readn(r,2);
                unsigned simple=readn(r,2); (void)readn(r,2);
                if(inner&&!strcmp(classname(r->v,c,inner),c->name)) {
                    c->simple_name=simple?utf(r->v,c,simple):"";
                    c->declaring_name=outer?classname(r->v,c,outer):NULL;
                }
            }
        }
        else if(!m&&!f&&!strcmp(name,"EnclosingMethod"))c->local_class=1;
        if(r->pos>end) fail(r->v,"invalid attribute size: %s",name);
        r->pos=end;
    }
}
static Class *load(VM *v,const char *name) {
    for(int i=0;i<v->nclasses;i++) if(!strcmp(v->classes[i]->name,name)) {
        if(v->classes[i]->loading) fail(v,"circular class hierarchy: %s",name);
        return v->classes[i];
    }
    if(v->nclasses==MAX_CLASSES) fail(v,"class count limit (%d)",MAX_CLASSES);
    Class *c=(Class *)alloc(v,sizeof(Class)); c->name=copy(v,name); c->loading=1;
    v->classes[v->nclasses++]=c;
    for(const char *p="ZBCSIJFDV";*p;p++)if(!strcmp(name,primitive_name(*p))) {
        c->primitive=(unsigned)*p;c->builtin=1;c->access=0x411;c->loading=0;c->init=2;return c;
    }
    if(name[0]=='[') {
        const char *d=name+1;
        if(*d=='[')c->component=load(v,d);
        else if(*d=='L') {
            size_t n=strlen(d);if(n<3||n>480||d[n-1]!=';')fail(v,"invalid array class name: %s",name);
            char component[512];memcpy(component,d+1,n-2);component[n-2]=0;c->component=load(v,component);
        } else {
            const char *pn=primitive_name(*d);
            if(!pn||d[1]||*d=='V')fail(v,"invalid array class name: %s",name);
            c->component=load(v,pn);
        }
        c->super=load(v,"java/lang/Object");c->ni=2;c->interfaces=(Class **)alloc(v,2*sizeof(Class *));
        c->app_loader=c->component->app_loader;
        c->interfaces[0]=load(v,"java/lang/Cloneable");c->interfaces[1]=load(v,"java/io/Serializable");
        c->access=(c->component->access&1)|0x410;c->builtin=1;c->loading=0;c->init=2;return c;
    }
    const char *base=builtin_super(name);
    if(base) {
        c->builtin=1;c->access=1; if(*base) c->super=load(v,base); c->loading=0;
        if(!strcmp(name,"java/lang/AutoCloseable")||!strcmp(name,"java/io/Closeable"))c->access=0x601;
        if(!strcmp(name,"java/io/Closeable")||!strcmp(name,"java/io/InputStream")||!strcmp(name,"java/io/Reader")) {
            c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,!strcmp(name,"java/io/Closeable")?"java/lang/AutoCloseable":"java/io/Closeable");
        }
        if(!strcmp(name,"java/net/URLConnection")||!strcmp(name,"java/net/JarURLConnection"))c->access=0x401;
        if(!strcmp(name,"nspire/FileConnection")||!strcmp(name,"nspire/JarConnection"))c->slots=6;
        if(!strcmp(name,"java/lang/reflect/AnnotatedElement")||!strcmp(name,"java/lang/reflect/GenericDeclaration")||!strcmp(name,"java/lang/reflect/Member"))c->access=0x601;
        if(!strcmp(name,"java/lang/reflect/AccessibleObject")||!strcmp(name,"java/lang/reflect/GenericDeclaration")) {
            c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,"java/lang/reflect/AnnotatedElement");
        }
        if(!strcmp(name,"java/lang/reflect/Executable")) {
            c->ni=2;c->interfaces=(Class **)alloc(v,2*sizeof(Class *));c->interfaces[0]=load(v,"java/lang/reflect/Member");c->interfaces[1]=load(v,"java/lang/reflect/GenericDeclaration");
        }
        if(!strcmp(name,"java/lang/Cloneable")||!strcmp(name,"java/io/Serializable")||!strcmp(name,"java/lang/Runnable")||!strcmp(name,"java/lang/Comparable")||!strcmp(name,"java/lang/CharSequence"))c->access=0x601;
        if(!strcmp(name,"sun/misc/JavaLangAccess"))c->access=0x601;
        if(!strcmp(name,"nspire/JavaLangAccess")) {
            c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,"sun/misc/JavaLangAccess");
        }
        if(!strcmp(name,"java/lang/String")) {
            c->ni=3;c->interfaces=(Class **)alloc(v,3*sizeof(Class *));
            c->interfaces[0]=load(v,"java/io/Serializable");c->interfaces[1]=load(v,"java/lang/Comparable");c->interfaces[2]=load(v,"java/lang/CharSequence");
        }
        if(!strcmp(name,"java/lang/StringBuilder")) {
            c->ni=2;c->interfaces=(Class **)alloc(v,2*sizeof(Class *));c->interfaces[0]=load(v,"java/io/Serializable");c->interfaces[1]=load(v,"java/lang/CharSequence");
        }
        if(!strcmp(name,"java/lang/Thread")) {
            c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,"java/lang/Runnable");
            const char *names[]={"parkBlocker","threadLocalRandomSeed","threadLocalRandomProbe","threadLocalRandomSecondarySeed"};
            const char *descs[]={"Ljava/lang/Object;","J","I","I"};
            c->nf=4;c->slots=4;c->fields=(Field *)alloc(v,4*sizeof(Field));
            for(unsigned i=0;i<4;i++){c->fields[i].name=(char *)names[i];c->fields[i].desc=(char *)descs[i];c->fields[i].slot=i;c->fields[i].flags=0x42;}
        }
        if(!strcmp(name,"nspire/ResourceEnumeration")) {
            c->ni=1;c->interfaces=(Class **)alloc(v,sizeof(Class *));c->interfaces[0]=load(v,"java/util/Enumeration");
        }
        if(!strcmp(name,"java/io/InputStreamReader")||!strcmp(name,"java/io/BufferedReader")||!strcmp(name,"java/io/ByteArrayInputStream"))c->slots=1;
        if(!strcmp(name,"java/lang/Class")||!strcmp(name,"java/lang/String")||wrapper_primitive(name))c->access=0x11;
        if(wrapper_primitive(name)) {
            int boolean=!strcmp(name,"java/lang/Boolean");
            int number=!strcmp(name,"java/lang/Integer")||!strcmp(name,"java/lang/Long")||!strcmp(name,"java/lang/Double");
            c->nf=boolean?4:number?2:1;c->fields=(Field *)alloc(v,c->nf*sizeof(Field));
            c->fields[0].name="TYPE";c->fields[0].desc="Ljava/lang/Class;";c->fields[0].flags=STATIC|0x11;c->fields[0].value=rv(NULL);
            if(c->nf>=2){c->slots=1;c->fields[1].name="value";c->fields[1].desc=boolean?"Z":!strcmp(name,"java/lang/Long")?"J":!strcmp(name,"java/lang/Double")?"D":"I";c->fields[1].flags=0x12;c->fields[1].slot=0;
                c->ni=2;c->interfaces=(Class **)alloc(v,2*sizeof(Class *));c->interfaces[0]=load(v,"java/lang/Comparable");c->interfaces[1]=load(v,"java/io/Serializable");}
            if(boolean)for(unsigned i=2;i<4;i++){c->fields[i].name=i==2?"TRUE":"FALSE";c->fields[i].desc="Ljava/lang/Boolean;";c->fields[i].flags=STATIC|0x11;c->fields[i].value=rv(NULL);}
        }
        if(!strcmp(name,"java/lang/System")) {
            c->nf=2; c->fields=(Field *)alloc(v,2*sizeof(Field));
            for(unsigned i=0;i<2;i++) { c->fields[i].name=i?"err":"out"; c->fields[i].desc="Ljava/io/PrintStream;"; c->fields[i].flags=STATIC; c->fields[i].value=rv(NULL); }
        }
        return c;
    }
    size_t length; unsigned char *data=class_bytes(v,c,name,&length);
    Reader r={v,data,length,0}; if(readn(&r,4)!=0xcafebabeU) fail(v,"not a class file: %s",name);
    unsigned minor=readn(&r,2), major=readn(&r,2);
    if(major<45||major>61||minor==65535) fail(v,"unsupported class version %u.%u in %s (up to 61, no preview)",major,minor,name);
    c->nc=(uint16_t)readn(&r,2); if(!c->nc) fail(v,"empty constant pool");
    c->cp=(CP *)alloc(v,c->nc*sizeof(CP));
    for(unsigned i=1;i<c->nc;i++) {
        CP *p=&c->cp[i]; p->tag=(uint8_t)readn(&r,1);
        switch(p->tag) {
        case 1: { unsigned n=readn(&r,2); p->text=(char *)alloc(v,n+1); if(n>r.n-r.pos) fail(v,"truncated UTF8"); memcpy(p->text,r.p+r.pos,n); skip(&r,n); break; }
        case 3: case 4: p->bits=readn(&r,4); break;
        case 5: case 6: { uint64_t hi=readn(&r,4); p->bits=(hi<<32)|readn(&r,4); if(++i>=c->nc) fail(v,"invalid wide constant"); break; }
        case 7: case 8: case 16: case 19: case 20: p->a=(uint16_t)readn(&r,2); break;
        case 9: case 10: case 11: case 12: case 17: case 18: p->a=(uint16_t)readn(&r,2); p->b=(uint16_t)readn(&r,2); break;
        case 15: p->a=(uint16_t)readn(&r,1); p->b=(uint16_t)readn(&r,2); break;
        default: fail(v,"invalid constant tag %u",p->tag);
        }
    }
    c->access=(uint16_t)readn(&r,2); unsigned self=readn(&r,2), parent=readn(&r,2);
    if(strcmp(classname(v,c,self),name)) fail(v,"class name mismatch: %s",name);
    if(parent) c->super=load(v,classname(v,c,parent));
    c->slots=c->super?c->super->slots:0;
    c->ni=(uint16_t)readn(&r,2); c->interfaces=(Class **)alloc(v,c->ni*sizeof(Class *));
    for(unsigned i=0;i<c->ni;i++) c->interfaces[i]=load(v,classname(v,c,readn(&r,2)));
    c->nf=(uint16_t)readn(&r,2); c->fields=(Field *)alloc(v,c->nf*sizeof(Field));
    for(unsigned i=0;i<c->nf;i++) {
        Field *f=&c->fields[i]; f->flags=(uint16_t)readn(&r,2); f->name=utf(v,c,readn(&r,2)); f->desc=utf(v,c,readn(&r,2));
        f->value=zero(f->desc); if(!(f->flags&STATIC)) f->slot=c->slots++; attributes(&r,c,NULL,f);
    }
    c->nm=(uint16_t)readn(&r,2); c->methods=(Method *)alloc(v,c->nm*sizeof(Method));
    for(unsigned i=0;i<c->nm;i++) {
        Method *m=&c->methods[i]; m->owner=c; m->flags=(uint16_t)readn(&r,2); m->name=utf(v,c,readn(&r,2)); m->desc=utf(v,c,readn(&r,2)); attributes(&r,c,m,NULL);
    }
    attributes(&r,c,NULL,NULL); if(r.pos!=r.n) fail(v,"trailing data in class %s",name);
    c->loading=0; return c;
}
static Method *method(Class *c,const char *n,const char *d) {
    for(;c;c=c->super) for(unsigned i=0;i<c->nm;i++) if(!strcmp(c->methods[i].name,n)&&!strcmp(c->methods[i].desc,d)) return &c->methods[i];
    return NULL;
}
static int subtype(Class *c,Class *target) {
    if(!c) return 0; if(c==target) return 1;
    if(c->primitive||target->primitive)return 0;
    if(c->component&&target->component)return subtype(c->component,target->component);
    for(unsigned i=0;i<c->ni;i++) if(subtype(c->interfaces[i],target)) return 1;
    return subtype(c->super,target);
}
static void default_candidates(Class *c,const char *n,const char *d,Class **seen,unsigned *visited,Method **found,unsigned *count) {
    if(!c)return;for(unsigned i=0;i<*visited;i++)if(seen[i]==c)return;seen[(*visited)++]=c;
    if(c->access&0x200)for(unsigned i=0;i<c->nm;i++) {
        Method *m=&c->methods[i];if(!(m->flags&(STATIC|2))&&!strcmp(m->name,n)&&!strcmp(m->desc,d))found[(*count)++]=m;
    }
    for(unsigned i=0;i<c->ni;i++)default_candidates(c->interfaces[i],n,d,seen,visited,found,count);
    default_candidates(c->super,n,d,seen,visited,found,count);
}
static Method *default_method(VM *v,Class *c,const char *n,const char *d) {
    int any=0;for(Class *p=c;p;p=p->super)if(p->ni){any=1;break;}if(!any)return NULL;
    Class **seen=(Class **)alloc(v,MAX_CLASSES*sizeof(Class *));Method **found=(Method **)alloc(v,MAX_CLASSES*sizeof(Method *));
    unsigned visited=0,count=0;default_candidates(c,n,d,seen,&visited,found,&count);Method *result=NULL;
    for(unsigned i=0;i<count;i++) {
        int shadowed=0;for(unsigned j=0;j<count;j++)if(i!=j&&found[i]->owner!=found[j]->owner&&subtype(found[j]->owner,found[i]->owner)){shadowed=1;break;}
        if(!shadowed&&!(found[i]->flags&0x400)) {
            if(result&&result!=found[i]){release(v,seen);release(v,found);v->exception=new_object(v,load(v,"java/lang/IncompatibleClassChangeError"),'o',0);return NULL;}
            result=found[i];
        }
    }
    release(v,seen);release(v,found);return result;
}
static Field *field(VM *v,Class **owner,const char *n,const char *d) {
    for(Class *c=*owner;c;c=c->super) for(unsigned i=0;i<c->nf;i++)
        if(!strcmp(c->fields[i].name,n)&&!strcmp(c->fields[i].desc,d)) { *owner=c; return &c->fields[i]; }
    fail(v,"field not found: %s.%s:%s",(*owner)->name,n,d); return NULL;
}
static void initialize_default_interfaces(VM *v,Class *c) {
    for(unsigned i=0;i<c->ni&&!v->exception;i++)initialize_default_interfaces(v,c->interfaces[i]);
    for(unsigned i=0;i<c->nm&&!v->exception;i++)if(!(c->methods[i].flags&(STATIC|0x400|2))){initialize(v,c);break;}
}
static void initialize(VM *v,Class *c) {
    while(c->init==1&&c->init_owner!=v->current) {
        v->current->state=T_CLASS;v->current->waiting_class=c;schedule(v);v->current->waiting_class=NULL;
    }
    if(c->init==2||c->init==1) return;
    if(c->init==3) fail(v,"class initialization previously failed: %s",c->name);
    c->init=1;c->init_owner=v->current;
    if(c->super) initialize(v,c->super);
    if(!(c->access&0x200))for(unsigned i=0;i<c->ni&&!v->exception;i++)initialize_default_interfaces(v,c->interfaces[i]);
    if(v->exception) { c->init=3; return; }
    if(!strcmp(c->name,"java/lang/System")) {
        for(unsigned i=0;i<2;i++) c->fields[i].value=rv(new_object(v,load(v,"java/io/PrintStream"),'o',0));
    }
    const char *primitive=wrapper_primitive(c->name);
    if(primitive)c->fields[0].value=rv(class_mirror(v,load(v,primitive)));
    if(!strcmp(c->name,"java/lang/Boolean"))for(unsigned i=2;i<4;i++) {
        Object *o=new_object(v,c,'B',1);o->data[0]=iv(i==2);c->fields[i].value=rv(o);
    }
    for(unsigned i=0;i<c->nf;i++) if(c->fields[i].constant&&(c->fields[i].flags&STATIC)) c->fields[i].value=constant(v,c,c->fields[i].constant);
    Method *m=method(c,"<clinit>","()V"); if(m&&m->owner==c) execute(v,m,NULL,0);
    c->init=v->exception?3:2;
}
static void throwing(VM *v,const char *name) { v->exception=new_object(v,load(v,name),'o',0); }
static Object *nonnull(VM *v,Value a) {
    if(a.tag!=REF) fail(v,"reference expected");
    Object *o=obj(a); if(!o) throwing(v,"java/lang/NullPointerException"); return o;
}
#include "threads.inc"
static void push(VM *v,Frame *f,Value a) { if(f->sp>=f->method->stack) fail(v,"operand stack overflow"); f->stack[f->sp++]=a; }
static Value pop(VM *v,Frame *f) { if(!f->sp) fail(v,"operand stack underflow"); return f->stack[--f->sp]; }
static uint32_t code(VM *v,Frame *f,unsigned n) {
    if(n>f->method->length-f->pc) fail(v,"truncated instruction");
    uint32_t a=0; while(n--) a=(a<<8)|f->method->code[f->pc++]; return a;
}
static void branch(VM *v,Frame *f,int64_t offset) {
    int64_t dest=(int64_t)f->ip+offset;
    if(dest<0||dest>=f->method->length) fail(v,"invalid branch target"); f->pc=(uint32_t)dest;
}
static void local_set(VM *v,Frame *f,unsigned i,Value a) {
    if(i>=f->method->locals||(wide(a)&&i+1>=f->method->locals)) fail(v,"local index out of range");
    if(i && wide(f->locals[i-1])) f->locals[i-1]=iv(0);
    f->locals[i]=a; if(wide(a)) f->locals[i+1]=iv(0);
}
static Value local_get(VM *v,Frame *f,unsigned i) { if(i>=f->method->locals) fail(v,"local index out of range"); return f->locals[i]; }
static unsigned nargs(VM *v,const char *d) {
    if(*d++!='(') fail(v,"invalid method descriptor"); unsigned n=0;
    while(*d&&*d!=')') {
        while(*d=='[') d++;
        if(*d=='L') { d=strchr(d,';'); if(!d) fail(v,"invalid method descriptor"); }
        else if(!strchr("ZBCSIJFD",*d)) fail(v,"invalid method descriptor");
        d++; n++;
    }
    if(*d!=')'||!d[1]) fail(v,"invalid method descriptor"); return n;
}
static char return_type(VM *v,const char *d) { const char *p=strchr(d,')'); if(!p||!p[1]) fail(v,"invalid descriptor"); return p[1]; }
static char *as_text(VM *v,Value a,char buf[128]) {
    switch(a.tag) {
    case REF: if(!obj(a)) return "null"; if(obj(a)->text) return obj(a)->text;
        if(obj(a)->kind=='B')return integer(obj(a)->data[0])?"true":"false";
        if(obj(a)->kind=='w')return as_text(v,obj(a)->data[0],buf);
        snprintf(buf,128,"%s@%lx",obj(a)->cls->name,(unsigned long)(uintptr_t)obj(a)); return buf;
    case LONG: snprintf(buf,128,"%lld",(long long)(int64_t)a.bits); break;
    case FLOAT: snprintf(buf,128,"%.9g",(double)flt(a)); break;
    case DOUBLE: snprintf(buf,128,"%.17g",dbl(a)); break;
    default: snprintf(buf,128,"%ld",(long)integer(a)); break;
    }
    (void)v; return buf;
}
/* Java object conversion must invoke user overrides, including exceptions and
 * allocations in toString. The caller roots the returned string if it allocates. */
static Value object_string(VM *v,Value value) {
    if(!obj(value))return rv(string(v,"null"));
    if(obj(value)->kind=='w'&&obj(value)->data[0].tag==DOUBLE)fail(v,"Double object text formatting is not implemented");
    Method *m=method(obj(value)->cls,"toString","()Ljava/lang/String;");
    if(m) {
        Value result=execute(v,m,&value,1);
        if(!v->exception&&(result.tag!=REF||(obj(result)&&obj(result)->kind!='s')))
            fail(v,"toString did not return String");
        return result;
    }
    char text[128];return rv(string(v,as_text(v,value,text)));
}
static Object *class_name_string(VM *v,Class *c,int simple) {
    if(simple&&c->component) {
        Object *part=class_name_string(v,c->component,1);root(v,part);
        size_t n=strlen(part->text);char *s=(char *)alloc(v,n+3);
        memcpy(s,part->text,n);memcpy(s+n,"[]",3);Object *result=string(v,s);
        release(v,s);v->nr--;return result;
    }
    const char *s=c->name;
    if(simple) { if(c->simple_name)s=c->simple_name;else {const char *p=strrchr(s,'/');if(p)s=p+1;} }
    char *text=copy(v,s);
    for(char *p=text;*p;p++)if(*p=='/')*p='.';
    Object *result=string(v,text);release(v,text);return result;
}
/* Probe before forName so missing files become Java ClassNotFoundException.
 * Malformed/unsupported classes still produce explicit VM diagnostics. */
static int class_available(VM *v,const char *name) {
    if(builtin_super(name))return 1;
    if(name[0]=='[') {
        const char *p=name;while(*p=='[')p++;
        if(*p=='L') {size_t n=strlen(p);char component[512];if(n<3||n>=sizeof component||p[n-1]!=';')return 0;
            memcpy(component,p+1,n-2);component[n-2]=0;return class_available(v,component);}
        return *p&&*p!='V'&&primitive_name(*p)&&!p[1];
    }
    if(!valid_name(name))return 0;
    char path[1024];int index;return find_class(v,name,&index,path)!=NULL;
}
#include "unsafe.inc"
static LocalEntry *local_entry(VM *v,VmThread *t,Object *key,int create) {
    for(LocalEntry *e=t->locals_map;e;e=e->next)if(e->key==key)return e;
    if(!create)return NULL;
    LocalEntry *e=(LocalEntry *)alloc(v,sizeof *e);e->key=key;e->next=t->locals_map;t->locals_map=e;return e;
}
static void inherit_locals(VM *v,VmThread *child) {
    size_t count=0;Class *inheritable=load(v,"java/lang/InheritableThreadLocal");
    for(LocalEntry *e=v->current->locals_map;e;e=e->next)if(subtype(e->key->cls,inheritable))count++;
    if(!count)return;
    /* childValue may mutate the parent's map, allocate or yield: snapshot and
     * root all inputs before invoking any user code. */
    Object *snapshot=new_object(v,load(v,"java/lang/Object"),'o',2*count);root(v,snapshot);
    size_t used=0;
    for(LocalEntry *e=v->current->locals_map;e;e=e->next)if(subtype(e->key->cls,inheritable)) {
        snapshot->data[used++]=rv(e->key);snapshot->data[used++]=rv(e->value);
    }
    for(size_t i=0;i<used&&!v->exception;i+=2) {
        Object *key=obj(snapshot->data[i]);Method *m=method(key->cls,"childValue","(Ljava/lang/Object;)Ljava/lang/Object;");
        Value result=m?execute(v,m,&snapshot->data[i],2):snapshot->data[i+1];
        if(!v->exception)local_entry(v,child,key,1)->value=obj(result);
    }
    v->nr--;
}
static unsigned utf_unit(const unsigned char **input) {
    const unsigned char *p=*input;unsigned ch=*p++;
    if(ch>=0xe0&&p[0]&&p[1]){ch=((ch&15)<<12)|((p[0]&63)<<6)|(p[1]&63);p+=2;}
    else if(ch>=0xc0&&*p)ch=((ch&31)<<6)|(*p++&63);
    *input=p;return ch;
}
static size_t write_unit(char *p,unsigned ch) {
    if(ch&&ch<128){p[0]=(char)ch;return 1;}
    if(ch<2048){p[0]=(char)(0xc0|(ch>>6));p[1]=(char)(0x80|(ch&63));return 2;}
    p[0]=(char)(0xe0|(ch>>12));p[1]=(char)(0x80|((ch>>6)&63));p[2]=(char)(0x80|(ch&63));return 3;
}
#include "reflection.inc"
#include "loader.inc"
#include "identifiers.inc"
#include "enums.inc"
#include "case.inc"
#include "lambda.inc"
#include "indy.inc"
#include "format.inc"
#include "split.inc"
#include "search.inc"
#include "builder.inc"
#include "xml.inc"
static int parse_boolean(Object *o) {
    const char *s=o?o->text:NULL;if(!s||strlen(s)!=4)return 0;
    return (s[0]=='t'||s[0]=='T')&&(s[1]=='r'||s[1]=='R')&&(s[2]=='u'||s[2]=='U')&&(s[3]=='e'||s[3]=='E');
}
static Value native_call(VM *v,Class *c,const char *n,const char *d,Value *a,unsigned na,int isstatic) {
    const char *cl=c->name; Value none=iv(0); Object *self=NULL;
    if(!isstatic) { if(!na) fail(v,"missing receiver"); self=nonnull(v,a[0]); if(!self) return none; }
    int handled=0;Value loaded=reflection_native(v,c,n,d,a,isstatic,&handled);if(handled)return loaded;
    loaded=loader_native(v,c,n,d,a,na,isstatic,&handled);if(handled)return loaded;
    if(!isstatic&&!strcmp(cl,"nspire/xml/ExpatReader")&&!strcmp(n,"parse0")&&!strcmp(d,"(Lorg/xml/sax/InputSource;ZZ)V")) {
        xml_parse(v,self,obj(a[1]),integer(a[2]),integer(a[3]));return none;
    }
    if(isstatic&&!strcmp(cl,"java/security/AccessController")&&!strcmp(n,"doPrivileged")&&!strcmp(d,"(Ljava/security/PrivilegedAction;)Ljava/lang/Object;")) {
        /* This VM has no SecurityManager/protection-domain policy. Only the
         * no-context action overload is supported; the action really runs. */
        Object *action=nonnull(v,a[0]);if(!action)return none;
        Method *run=method(action->cls,"run","()Ljava/lang/Object;");if(!run)fail(v,"PrivilegedAction.run not found");
        return execute(v,run,a,1);
    }
    if(isstatic&&!strcmp(cl,"sun/misc/SharedSecrets")&&!strcmp(n,"getJavaLangAccess")&&!strcmp(d,"()Lsun/misc/JavaLangAccess;")) {
        if(!v->java_lang_access)v->java_lang_access=new_object(v,load(v,"nspire/JavaLangAccess"),'o',0);
        return rv(v->java_lang_access);
    }
    if(!isstatic&&(!strcmp(cl,"sun/misc/JavaLangAccess")||!strcmp(cl,"nspire/JavaLangAccess"))&&!strcmp(n,"getEnumConstantsShared")&&!strcmp(d,"(Ljava/lang/Class;)[Ljava/lang/Enum;")) {
        Object *mirror=nonnull(v,a[1]);if(!mirror)return none;
        if(mirror->kind!='c')fail(v,"enum universe requires Class");return rv(enum_constants(v,mirror->represented));
    }
    if(!strcmp(cl,"java/lang/Boolean")) {
        if(isstatic) {
            if(!strcmp(n,"parseBoolean")&&!strcmp(d,"(Ljava/lang/String;)Z"))return iv(parse_boolean(obj(a[0])));
            if(!strcmp(n,"valueOf")&&(!strcmp(d,"(Z)Ljava/lang/Boolean;")||!strcmp(d,"(Ljava/lang/String;)Ljava/lang/Boolean;")))return c->fields[(d[1]=='Z'?integer(a[0]):parse_boolean(obj(a[0])))?2:3].value;
            if(!strcmp(n,"toString")&&!strcmp(d,"(Z)Ljava/lang/String;"))return rv(string(v,integer(a[0])?"true":"false"));
            if(!strcmp(n,"compare")&&!strcmp(d,"(ZZ)I"))return iv(!!integer(a[0])-!!integer(a[1]));
            if(!strcmp(n,"hashCode")&&!strcmp(d,"(Z)I"))return iv(integer(a[0])?1231:1237);
            if(!strcmp(n,"getBoolean")&&!strcmp(d,"(Ljava/lang/String;)Z")) {
                Object *key=obj(a[0]);Property *p=key&&key->text?property(v,key->text,0):NULL;
                if(!p||!p->value)return iv(0);
                Object *text=string(v,p->value);return iv(parse_boolean(text));
            }
        } else {
            if(!strcmp(n,"<init>")&&(!strcmp(d,"(Z)V")||!strcmp(d,"(Ljava/lang/String;)V"))){self->kind='B';self->data[0]=iv(d[1]=='Z'?!!integer(a[1]):parse_boolean(obj(a[1])));return none;}
            int value=integer(self->data[0]);
            if(!strcmp(n,"booleanValue")&&!strcmp(d,"()Z"))return iv(value);
            if(!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;"))return rv(string(v,value?"true":"false"));
            if(!strcmp(n,"hashCode")&&!strcmp(d,"()I"))return iv(value?1231:1237);
            if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")){Object *other=obj(a[1]);return iv(other&&other->kind=='B'&&value==integer(other->data[0]));}
            if(!strcmp(n,"compareTo")&&(!strcmp(d,"(Ljava/lang/Boolean;)I")||!strcmp(d,"(Ljava/lang/Object;)I"))) {
                Object *other=nonnull(v,a[1]);if(!other)return none;if(other->kind!='B'){throwing(v,"java/lang/ClassCastException");return none;}return iv(value-integer(other->data[0]));
            }
        }
        goto missing;
    }
    if(!strcmp(cl,"sun/misc/Unsafe"))return unsafe_call(v,n,d,a,isstatic);
    if(!strcmp(cl,"sun/misc/VM")&&isstatic&&!strcmp(n,"getSavedProperty")&&!strcmp(d,"(Ljava/lang/String;)Ljava/lang/String;")) {
        Object *key=nonnull(v,a[0]);if(!key)return none;Property *p=property(v,key->text,0);
        return rv(p&&p->initial?string(v,p->initial):NULL);
    }
    if(!strcmp(cl,"java/lang/ThreadLocal")||!strcmp(cl,"java/lang/InheritableThreadLocal")) {
        if(!isstatic) {
            if(!strcmp(n,"<init>")&&!strcmp(d,"()V"))return none;
            if(!strcmp(n,"initialValue")&&!strcmp(d,"()Ljava/lang/Object;"))return rv(NULL);
            if(!strcmp(cl,"java/lang/InheritableThreadLocal")&&!strcmp(n,"childValue")&&!strcmp(d,"(Ljava/lang/Object;)Ljava/lang/Object;"))return a[1];
            if(!strcmp(n,"get")&&!strcmp(d,"()Ljava/lang/Object;")) {
                LocalEntry *e=local_entry(v,v->current,self,0);if(e)return rv(e->value);
                Method *m=method(self->cls,"initialValue","()Ljava/lang/Object;");
                Value result=m?execute(v,m,a,1):rv(NULL);
                if(!v->exception)local_entry(v,v->current,self,1)->value=obj(result);
                return result;
            }
            if(!strcmp(n,"set")&&!strcmp(d,"(Ljava/lang/Object;)V")){local_entry(v,v->current,self,1)->value=obj(a[1]);return none;}
            if(!strcmp(n,"remove")&&!strcmp(d,"()V")) {
                LocalEntry **p=&v->current->locals_map;
                while(*p){LocalEntry *e=*p;if(e->key==self){*p=e->next;release(v,e);break;}p=&e->next;}return none;
            }
        }
        goto missing;
    }
    if(!strcmp(cl,"java/lang/Runtime")) {
        if(isstatic&&!strcmp(n,"getRuntime")&&!strcmp(d,"()Ljava/lang/Runtime;")) {
            if(!v->runtime_instance)v->runtime_instance=new_object(v,c,'o',0);return rv(v->runtime_instance);
        }
        if(!isstatic&&!strcmp(n,"availableProcessors")&&!strcmp(d,"()I"))return iv(1);
    }
    if(!strcmp(cl,"java/lang/Integer")) {
        if(isstatic&&!strcmp(n,"valueOf")&&!strcmp(d,"(I)Ljava/lang/Integer;")) {
            int32_t value=integer(a[0]);int cached=value>=-128&&value<=127;
            if(cached&&v->integer_cache[value+128])return rv(v->integer_cache[value+128]);
            Object *o=new_object(v,c,'w',1);o->data[0]=iv(value);if(cached)v->integer_cache[value+128]=o;return rv(o);
        }
        if(!isstatic) {
            if(!strcmp(n,"<init>")&&!strcmp(d,"(I)V")){self->kind='w';self->data[0]=a[1];return none;}
            int32_t value=integer(self->data[0]);
            if((!strcmp(n,"intValue")&&!strcmp(d,"()I"))||(!strcmp(n,"hashCode")&&!strcmp(d,"()I")))return iv(value);
            if(!strcmp(n,"longValue")&&!strcmp(d,"()J"))return val((uint64_t)(int64_t)value,LONG);
            if(!strcmp(n,"floatValue")&&!strcmp(d,"()F"))return fv((float)value);
            if(!strcmp(n,"doubleValue")&&!strcmp(d,"()D"))return dv(value);
            if(!strcmp(n,"shortValue")&&!strcmp(d,"()S"))return iv((int16_t)value);
            if(!strcmp(n,"byteValue")&&!strcmp(d,"()B"))return iv((int8_t)value);
            if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")){Object *other=obj(a[1]);return iv(other&&other->cls==c&&integer(other->data[0])==value);}
            if(!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;")){char b[128];return rv(string(v,as_text(v,self->data[0],b)));}
            if(!strcmp(n,"compareTo")&&(!strcmp(d,"(Ljava/lang/Integer;)I")||!strcmp(d,"(Ljava/lang/Object;)I"))) {
                Object *other=nonnull(v,a[1]);if(!other)return none;if(other->cls!=c){throwing(v,"java/lang/ClassCastException");return none;}
                int32_t right=integer(other->data[0]);return iv(value<right?-1:value>right?1:0);
            }
        }
    }
    if(!strcmp(cl,"java/lang/Long")||!strcmp(cl,"java/lang/Double")) {
        int large=!strcmp(cl,"java/lang/Long");
        if(isstatic&&!strcmp(n,"valueOf")&&!strcmp(d,large?"(J)Ljava/lang/Long;":"(D)Ljava/lang/Double;")) {
            int64_t value=(int64_t)a[0].bits;int cached=large&&value>=-128&&value<=127;
            if(cached&&v->long_cache[value+128])return rv(v->long_cache[value+128]);
            Object *o=new_object(v,c,'w',1);o->data[0]=a[0];if(cached)v->long_cache[value+128]=o;return rv(o);
        }
        if(!isstatic) {
            if(!strcmp(n,"<init>")&&!strcmp(d,large?"(J)V":"(D)V")){self->kind='w';self->data[0]=a[1];return none;}
            Value value=self->data[0];double number=large?(double)(int64_t)value.bits:dbl(value);
            if(!strcmp(n,"doubleValue")&&!strcmp(d,"()D"))return dv(number);
            if(!strcmp(n,"floatValue")&&!strcmp(d,"()F"))return fv(large?(float)(int64_t)value.bits:(float)number);
            if(!strcmp(n,"longValue")&&!strcmp(d,"()J"))return large?value:val(isnan(number)?0:number>=9223372036854775808.0?INT64_MAX:number<=-9223372036854775808.0?(uint64_t)INT64_MIN:(uint64_t)(int64_t)number,LONG);
            if((!strcmp(n,"intValue")&&!strcmp(d,"()I"))||(!strcmp(n,"shortValue")&&!strcmp(d,"()S"))||(!strcmp(n,"byteValue")&&!strcmp(d,"()B"))) {
                int32_t x=large?(int32_t)(uint32_t)value.bits:isnan(number)?0:number>=INT_MAX?INT_MAX:number<=INT_MIN?INT_MIN:(int32_t)number;
                return iv(d[2]=='B'?(int8_t)x:d[2]=='S'?(int16_t)x:x);
            }
            uint64_t bits=!large&&isnan(number)?UINT64_C(0x7ff8000000000000):value.bits;
            if(!strcmp(n,"hashCode")&&!strcmp(d,"()I"))return iv((int32_t)(uint32_t)(bits^(bits>>32)));
            if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")) {
                Object *other=obj(a[1]);if(!other||other->cls!=c)return iv(0);uint64_t right=other->data[0].bits;
                if(!large&&isnan(dbl(other->data[0])))right=UINT64_C(0x7ff8000000000000);return iv(bits==right);
            }
            if(large&&!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;")){char text[128];return rv(string(v,as_text(v,value,text)));}
            if(!strcmp(n,"compareTo")&&(!strcmp(d,"(Ljava/lang/Object;)I")||!strcmp(d,large?"(Ljava/lang/Long;)I":"(Ljava/lang/Double;)I"))) {
                Object *other=nonnull(v,a[1]);if(!other)return none;if(other->cls!=c){throwing(v,"java/lang/ClassCastException");return none;}
                Value right=other->data[0];if(!large){double y=dbl(right);if(number<y)return iv(-1);if(number>y)return iv(1);if(isnan(y))right.bits=UINT64_C(0x7ff8000000000000);}
                return iv((int64_t)bits<(int64_t)right.bits?-1:(int64_t)bits>(int64_t)right.bits?1:0);
            }
        }
    }
    if(!strcmp(cl,"java/lang/Thread")) {
        if(isstatic) {
            if(!strcmp(n,"currentThread")&&!strcmp(d,"()Ljava/lang/Thread;"))return rv(v->current->object);
            if(!strcmp(n,"yield")&&!strcmp(d,"()V")){schedule(v);return none;}
            if(!strcmp(n,"sleep")&&(!strcmp(d,"(J)V")||!strcmp(d,"(JI)V"))){thread_sleep(v,(int64_t)a[0].bits,na==2?integer(a[1]):0);return none;}
            if(!strcmp(n,"interrupted")&&!strcmp(d,"()Z")){int x=v->current->interrupted;v->current->interrupted=0;return iv(x);}
            if(!strcmp(n,"holdsLock")&&!strcmp(d,"(Ljava/lang/Object;)Z")){Object *o=nonnull(v,a[0]);return iv(o&&o->monitor_owner==v->current);}
        } else {
            VmThread *t=thread_record(v,self);
            if(!strcmp(n,"<init>")) {
                Object *target=NULL,*name=NULL;
                if(!strcmp(d,"()V")){}
                else if(!strcmp(d,"(Ljava/lang/Runnable;)V"))target=obj(a[1]);
                else if(!strcmp(d,"(Ljava/lang/String;)V")){name=nonnull(v,a[1]);if(!name)return none;}
                else if(!strcmp(d,"(Ljava/lang/Runnable;Ljava/lang/String;)V")){target=obj(a[1]);name=nonnull(v,a[2]);if(!name)return none;}
                else goto missing;
                self->thread_target=target;
                t->context_loader=v->current->context_loader;t->context_loader_set=v->current->context_loader_set;
                inherit_locals(v,t);if(v->exception)return none;
                if(name)set_text(v,self,name->text);else {char buf[64];snprintf(buf,sizeof buf,"Thread-%u",t->id);set_text(v,self,buf);}
                return none;
            }
            if(!strcmp(n,"start")&&!strcmp(d,"()V")){thread_start(v,self);return none;}
            if(!strcmp(n,"run")&&!strcmp(d,"()V")) {
                Object *target=self->thread_target;if(!target)return none;
                Method *m=method(target->cls,"run","()V");if(!m)fail(v,"Runnable.run() not found");
                Value arg=rv(target);return execute(v,m,&arg,1);
            }
            if(!strcmp(n,"join")&&(!strcmp(d,"()V")||!strcmp(d,"(J)V")||!strcmp(d,"(JI)V"))){thread_join(v,self,na>=2?(int64_t)a[1].bits:0,na==3?integer(a[2]):0);return none;}
            if(!strcmp(n,"isAlive")&&!strcmp(d,"()Z"))return iv(thread_alive(t));
            if(!strcmp(n,"isDaemon")&&!strcmp(d,"()Z"))return iv(t->daemon);
            if(!strcmp(n,"setDaemon")&&!strcmp(d,"(Z)V")){if(thread_alive(t))throwing(v,"java/lang/IllegalThreadStateException");else t->daemon=!!integer(a[1]);return none;}
            if(!strcmp(n,"getName")&&!strcmp(d,"()Ljava/lang/String;"))return rv(string(v,self->text?self->text:""));
            if(!strcmp(n,"setName")&&!strcmp(d,"(Ljava/lang/String;)V")){Object *s=nonnull(v,a[1]);if(s)set_text(v,self,s->text);return none;}
            if(!strcmp(n,"getId")&&!strcmp(d,"()J"))return val(t->id,LONG);
            if(!strcmp(n,"isInterrupted")&&!strcmp(d,"()Z"))return iv(t->interrupted);
            if(!strcmp(n,"interrupt")&&!strcmp(d,"()V")){if(thread_alive(t))t->interrupted=1;return none;}
            if(!strcmp(n,"getContextClassLoader")&&!strcmp(d,"()Ljava/lang/ClassLoader;"))return rv(t->context_loader_set?t->context_loader:system_loader(v));
            if(!strcmp(n,"setContextClassLoader")&&!strcmp(d,"(Ljava/lang/ClassLoader;)V")){loader_scope(v,obj(a[1]));t->context_loader=obj(a[1]);t->context_loader_set=1;return none;}
        }
        goto missing;
    }
    if(!strcmp(n,"<init>")) {
        if((!strcmp(cl,"java/lang/Object")||!strcmp(cl,"java/lang/Number"))&&!strcmp(d,"()V")) return none;
        if((!strcmp(cl,"java/lang/StringBuilder")||subtype(c,load(v,"java/lang/Throwable"))) &&
           (!strcmp(d,"()V")||!strcmp(d,"(Ljava/lang/String;)V"))) {
            if(na==2&&obj(a[1])) set_text(v,self,obj(a[1])->text?obj(a[1])->text:"");
            else if(!strcmp(cl,"java/lang/StringBuilder")) set_text(v,self,"");
            return none;
        }
        if(subtype(c,load(v,"java/lang/Throwable"))&&!strcmp(d,"(Ljava/lang/String;Ljava/lang/Throwable;)V")) {
            self->cause=obj(a[2]);if(obj(a[1]))set_text(v,self,obj(a[1])->text);return none;
        }
    }
    if(!isstatic&&subtype(c,load(v,"java/lang/Throwable"))) {
        if(!strcmp(n,"getMessage")&&!strcmp(d,"()Ljava/lang/String;"))return rv(self->text?string(v,self->text):NULL);
        if(!strcmp(n,"getCause")&&!strcmp(d,"()Ljava/lang/Throwable;"))return rv(self->cause);
    }
    if(!strcmp(cl,"java/io/PrintStream")&&(!strcmp(n,"println")||!strcmp(n,"print"))) {
        const char *allowed[]={"()V","(I)V","(J)V","(F)V","(D)V","(Z)V","(C)V","(Ljava/lang/String;)V","(Ljava/lang/Object;)V",NULL};
        int ok=0; for(int i=0;allowed[i];i++) if(!strcmp(d,allowed[i])) ok=1;
        if(!ok) goto missing;
        FILE *out=stdout; Class *s=load(v,"java/lang/System"); if(s->nf==2&&obj(s->fields[1].value)==self) out=stderr;
        if(na==2) { char b[128];
            if(d[1]=='Z') fputs(integer(a[1])?"true":"false",out);
            else if(d[1]=='C') { unsigned ch=(uint16_t)integer(a[1]); if(ch<128) fputc((int)ch,out); else if(ch<2048) { fputc(0xc0|(ch>>6),out); fputc(0x80|(ch&63),out); } else { fputc(0xe0|(ch>>12),out); fputc(0x80|((ch>>6)&63),out); fputc(0x80|(ch&63),out); } }
            else if(!strcmp(d,"(Ljava/lang/Object;)V")) {
                Value text=object_string(v,a[1]);if(v->exception)return none;
                fputs(as_text(v,text,b),out);
            } else fputs(as_text(v,a[1],b),out);
        }
        if(!strcmp(n,"println")) fputc('\n',out); return none;
    }
    if(!strcmp(cl,"java/lang/Object")) {
        if(!strcmp(n,"clone")&&!strcmp(d,"()Ljava/lang/Object;")) {
            if(!subtype(self->cls,load(v,"java/lang/Cloneable"))||subtype(self->cls,load(v,"java/lang/Thread"))){throwing(v,"java/lang/CloneNotSupportedException");return none;}
            Object *o=self->kind=='a'?array_new(v,self->array_desc,(int32_t)self->count):new_object(v,self->cls,self->kind,self->count);
            root(v,o);memcpy(o->data,self->data,self->count*sizeof(Value));o->cause=self->cause;
            if(self->text)set_text(v,o,self->text);
            if(self->buffer){stream_buffer(v,o,self->buffer_size);memcpy(o->buffer,self->buffer,self->buffer_size);}
            else o->buffer_size=self->buffer_size;
            o->cursor=self->cursor;o->mark_cursor=self->mark_cursor;o->closed=self->closed;o->pending=self->pending;o->skip_lf=self->skip_lf;
            v->nr--;return rv(o);
        }
        if(!strcmp(n,"wait")&&(!strcmp(d,"()V")||!strcmp(d,"(J)V")||!strcmp(d,"(JI)V"))){object_wait(v,self,na>=2?(int64_t)a[1].bits:0,na==3?integer(a[2]):0);return none;}
        if((!strcmp(n,"notify")||!strcmp(n,"notifyAll"))&&!strcmp(d,"()V")){object_notify(v,self,!strcmp(n,"notifyAll"));return none;}
        if(!strcmp(n,"getClass")&&!strcmp(d,"()Ljava/lang/Class;"))return rv(class_mirror(v,self->cls));
        if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")) return iv(self==obj(a[1]));
        if(!strcmp(n,"hashCode")&&!strcmp(d,"()I")) return iv((int32_t)(uintptr_t)self);
        if(!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;")) { char b[128]; return rv(string(v,as_text(v,a[0],b))); }
    }
    if(!strcmp(cl,"java/lang/Class")) {
        if(isstatic&&!strcmp(n,"forName")&&(!strcmp(d,"(Ljava/lang/String;)Ljava/lang/Class;")||!strcmp(d,"(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;"))) {
            int app=na==3?loader_scope(v,obj(a[2])):v->frame?v->frame->method->owner->app_loader:1;
            Class *target=named_class(v,obj(a[0]),app,na==3?integer(a[1]):1);
            return rv(target?class_mirror(v,target):NULL);
        }
        if(!isstatic) {
            Class *target=self->represented;if(self->kind!='c'||!target)fail(v,"invalid Class receiver");
            if(!strcmp(n,"getDeclaredField")&&!strcmp(d,"(Ljava/lang/String;)Ljava/lang/reflect/Field;")) {
                Object *name=nonnull(v,a[1]);if(!name)return none;
                for(unsigned i=0;i<target->nf;i++)if(!strcmp(target->fields[i].name,name->text)) {
                    Object *o=new_object(v,load(v,"java/lang/reflect/Field"),'f',0);
                    o->represented=target;o->represented_field=&target->fields[i];return rv(o);
                }
                throwing(v,"java/lang/NoSuchFieldException");return none;
            }
            if(!strcmp(n,"desiredAssertionStatus")&&!strcmp(d,"()Z"))return iv(0);
            if(!strcmp(n,"getName")&&!strcmp(d,"()Ljava/lang/String;"))return rv(class_name_string(v,target,0));
            if(!strcmp(n,"getSimpleName")&&!strcmp(d,"()Ljava/lang/String;"))return rv(class_name_string(v,target,1));
            if(!strcmp(n,"getCanonicalName")&&!strcmp(d,"()Ljava/lang/String;"))return rv(canonical_name(v,target));
            if(!strcmp(n,"getDeclaringClass")&&!strcmp(d,"()Ljava/lang/Class;"))return rv(target->declaring_name?class_mirror(v,load(v,target->declaring_name)):NULL);
            if(!strcmp(n,"isEnum")&&!strcmp(d,"()Z"))return iv(enum_type(target));
            if((!strcmp(n,"getEnumConstants")||!strcmp(n,"getEnumConstantsShared"))&&!strcmp(d,"()[Ljava/lang/Object;")) {
                Object *values=enum_constants(v,target);if(v->exception||!values||!strcmp(n,"getEnumConstantsShared"))return rv(values);
                Object *clone=array_new(v,values->array_desc,(int32_t)values->count);memcpy(clone->data,values->data,values->count*sizeof(Value));return rv(clone);
            }
            if(!strcmp(n,"enumConstantDirectory")&&!strcmp(d,"()Ljava/util/Map;"))return rv(enum_directory(v,target));
            if(!strcmp(n,"isPrimitive")&&!strcmp(d,"()Z"))return iv(target->primitive!=0);
            if(!strcmp(n,"isArray")&&!strcmp(d,"()Z"))return iv(target->component!=NULL);
            if(!strcmp(n,"isInterface")&&!strcmp(d,"()Z"))return iv((target->access&0x200)!=0);
            if(!strcmp(n,"getModifiers")&&!strcmp(d,"()I"))return iv(target->access&0x7611);
            if(!strcmp(n,"getSuperclass")&&!strcmp(d,"()Ljava/lang/Class;"))return rv(target->super&&!(target->access&0x200)?class_mirror(v,target->super):NULL);
            if(!strcmp(n,"getComponentType")&&!strcmp(d,"()Ljava/lang/Class;"))return rv(target->component?class_mirror(v,target->component):NULL);
            if(!strcmp(n,"isInstance")&&!strcmp(d,"(Ljava/lang/Object;)Z"))return iv(obj(a[1])&&subtype(obj(a[1])->cls,target));
            if(!strcmp(n,"isAssignableFrom")&&!strcmp(d,"(Ljava/lang/Class;)Z")) {
                Object *other=nonnull(v,a[1]);if(!other)return none;
                if(other->kind!='c')fail(v,"Class argument expected");return iv(subtype(other->represented,target));
            }
            if(!strcmp(n,"cast")&&!strcmp(d,"(Ljava/lang/Object;)Ljava/lang/Object;")) {
                if(obj(a[1])&&!subtype(obj(a[1])->cls,target)){throwing(v,"java/lang/ClassCastException");return none;}return a[1];
            }
        }
    }
    if(!strcmp(cl,"java/lang/String")) {
        if(!isstatic&&(!strcmp(n,"toLowerCase")||!strcmp(n,"toUpperCase"))&&(!strcmp(d,"()Ljava/lang/String;")||!strcmp(d,"(Ljava/util/Locale;)Ljava/lang/String;")))
            return rv(case_string(v,self,na==2?obj(a[1]):NULL,na==1,!strcmp(n,"toUpperCase")));
        if(!isstatic&&!strcmp(n,"split")&&(!strcmp(d,"(Ljava/lang/String;)[Ljava/lang/String;")||!strcmp(d,"(Ljava/lang/String;I)[Ljava/lang/String;")))return rv(split_string(v,self,obj(a[1]),na==3?integer(a[2]):0));
        const char *s=self&&self->text?self->text:"";
        if(!strcmp(n,"contains")&&!strcmp(d,"(Ljava/lang/CharSequence;)Z")) {
            if(!nonnull(v,a[1]))return none;
            Value converted=object_string(v,a[1]);if(v->exception)return none;
            Object *needle=nonnull(v,converted);if(!needle)return none;
            return iv(string_search(s,needle->text,0,0)>=0);
        }
        if((!strcmp(n,"indexOf")||!strcmp(n,"lastIndexOf"))&&(!strcmp(d,"(Ljava/lang/String;)I")||!strcmp(d,"(Ljava/lang/String;I)I"))) {
            Object *needle=nonnull(v,a[1]);if(!needle)return none;int back=!strcmp(n,"lastIndexOf");
            return iv(string_search(s,needle->text,na==3?integer(a[2]):back?INT_MAX:0,back));
        }
        if(isstatic&&!strcmp(n,"format")&&!strcmp(d,"(Ljava/lang/String;[Ljava/lang/Object;)Ljava/lang/String;"))return rv(format_string(v,obj(a[0]),obj(a[1])));
        if(!strcmp(n,"<init>")&&!isstatic) {
            if(!strcmp(d,"()V")){self->kind='s';set_text(v,self,"");return none;}
            if(!strcmp(d,"(Ljava/lang/String;)V")) {
                Object *source=nonnull(v,a[1]);if(!source)return none;
                self->kind='s';set_text(v,self,source->text);return none;
            }
            if(!strcmp(d,"([C)V")||!strcmp(d,"([CII)V")) {
                Object *chars=nonnull(v,a[1]);if(!chars)return none;
                if(chars->kind!='a'||strcmp(chars->array_desc,"[C"))fail(v,"String constructor requires char[]");
                int32_t off=na==4?integer(a[2]):0,len=na==4?integer(a[3]):(int32_t)chars->count;
                if(off<0||len<0||(size_t)off>chars->count||(size_t)len>chars->count-(size_t)off){throwing(v,"java/lang/StringIndexOutOfBoundsException");return none;}
                if((size_t)len>(META_LIMIT-1)/3)fail(v,"String exceeds metadata limit");
                char *text=(char *)alloc(v,(size_t)len*3+1),*end=text;
                for(int32_t i=0;i<len;i++)end+=write_unit(end,(uint16_t)integer(chars->data[off+i]));
                *end=0;self->kind='s';set_text(v,self,text);release(v,text);return none;
            }
        }
        if(!strcmp(n,"length")&&!strcmp(d,"()I")) { int count=0; for(size_t i=0;s[i];i++) if(((unsigned char)s[i]&0xc0)!=0x80) count++; return iv(count); }
        if(!strcmp(n,"isEmpty")&&!strcmp(d,"()Z")) return iv(!*s);
        if(!strcmp(n,"equals")&&!strcmp(d,"(Ljava/lang/Object;)Z")) return iv(obj(a[1])&&obj(a[1])->kind=='s'&&!strcmp(s,obj(a[1])->text));
        if(!strcmp(n,"equalsIgnoreCase")&&!strcmp(d,"(Ljava/lang/String;)Z"))return iv(obj(a[1])&&!case_compare((const unsigned char *)s,(const unsigned char *)obj(a[1])->text));
        if(!strcmp(n,"compareToIgnoreCase")&&!strcmp(d,"(Ljava/lang/String;)I")) {
            Object *other=nonnull(v,a[1]);if(!other)return none;
            return iv(case_compare((const unsigned char *)s,(const unsigned char *)other->text));
        }
        if(!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;")) return a[0];
        if(!strcmp(n,"endsWith")&&!strcmp(d,"(Ljava/lang/String;)Z")) {
            Object *suffix=nonnull(v,a[1]);if(!suffix)return none;
            size_t len=strlen(s),part=strlen(suffix->text);return iv(part<=len&&!memcmp(s+len-part,suffix->text,part));
        }
        if(!strcmp(n,"startsWith")&&(!strcmp(d,"(Ljava/lang/String;)Z")||!strcmp(d,"(Ljava/lang/String;I)Z"))) {
            Object *prefix=nonnull(v,a[1]);if(!prefix)return none;
            int32_t offset=na==3?integer(a[2]):0;if(offset<0)return iv(0);
            const unsigned char *p=(const unsigned char *)s;while(offset>0&&*p){utf_unit(&p);offset--;}
            if(offset)return iv(0);size_t len=strlen((const char *)p),part=strlen(prefix->text);return iv(part<=len&&!memcmp(p,prefix->text,part));
        }
        if(!strcmp(n,"trim")&&!strcmp(d,"()Ljava/lang/String;")) {
            const unsigned char *p=(const unsigned char *)s,*start=p,*end=p;
            int leading=1;while(*p){const unsigned char *before=p;unsigned ch=utf_unit(&p);if(ch>32){if(leading)start=before;leading=0;end=p;}}
            if(leading)start=end;size_t len=(size_t)(end-start);char *buf=(char *)alloc(v,len+1);memcpy(buf,start,len);
            Object *r=string(v,buf);release(v,buf);return rv(r);
        }
        if((!strcmp(n,"substring")&&(!strcmp(d,"(I)Ljava/lang/String;")||!strcmp(d,"(II)Ljava/lang/String;")))||(!strcmp(n,"subSequence")&&!strcmp(d,"(II)Ljava/lang/CharSequence;"))) {
            int start=integer(a[1]),end=na==3?integer(a[2]):INT_MAX,pos=0;
            const unsigned char *p=(const unsigned char *)s,*begin=NULL,*finish=NULL;
            do {if(pos==start)begin=p;if(pos==end)finish=p;if(!*p)break;utf_unit(&p);pos++;}while(1);
            if(na==2){end=pos;finish=p;}
            if(start<0||end<start||end>pos){throwing(v,"java/lang/StringIndexOutOfBoundsException");return none;}
            size_t len=(size_t)(finish-begin);char *buf=(char *)alloc(v,len+1);memcpy(buf,begin,len);Object *r=string(v,buf);release(v,buf);return rv(r);
        }
        if(!strcmp(n,"indexOf")&&(!strcmp(d,"(I)I")||!strcmp(d,"(II)I"))) {
            int32_t want=integer(a[1]),from=na==3?integer(a[2]):0,pos=0;const unsigned char *p=(const unsigned char *)s;
            while(*p){unsigned ch=utf_unit(&p);if(pos>=from){unsigned point=ch;
                if(ch>=0xd800&&ch<=0xdbff&&*p){const unsigned char *q=p;unsigned low=utf_unit(&q);if(low>=0xdc00&&low<=0xdfff)point=0x10000+((ch-0xd800)<<10)+(low-0xdc00);}
                if((want>=0&&want<=0xffff&&ch==(unsigned)want)||(want>0xffff&&point==(unsigned)want))return iv(pos);}
                pos++;}return iv(-1);
        }
        if(!strcmp(n,"codePointAt")&&!strcmp(d,"(I)I")) {
            int target=integer(a[1]),pos=0;const unsigned char *p=(const unsigned char *)s;
            while(*p){unsigned ch=utf_unit(&p);if(pos++==target){
                if(ch>=0xd800&&ch<=0xdbff&&*p){unsigned low=utf_unit(&p);if(low>=0xdc00&&low<=0xdfff)ch=0x10000+((ch-0xd800)<<10)+(low-0xdc00);}return iv((int32_t)ch);}}
            throwing(v,"java/lang/StringIndexOutOfBoundsException");return none;
        }
        if(!strcmp(n,"intern")&&!strcmp(d,"()Ljava/lang/String;")) {
            for(Object *o=v->objects;o;o=o->next)if(o->interned&&!strcmp(o->text,s))return rv(o);
            self->interned=1;return a[0];
        }
        if(!strcmp(n,"hashCode")&&!strcmp(d,"()I")) {
            uint32_t hash=0;const unsigned char *p=(const unsigned char *)s;
            while(*p)hash=31*hash+utf_unit(&p);return iv((int32_t)hash);
        }
        if(!strcmp(n,"compareTo")&&(!strcmp(d,"(Ljava/lang/String;)I")||!strcmp(d,"(Ljava/lang/Object;)I"))) {
            Object *other=nonnull(v,a[1]);if(!other)return none;
            if(other->kind!='s'){throwing(v,"java/lang/ClassCastException");return none;}
            const unsigned char *p=(const unsigned char *)s,*q=(const unsigned char *)other->text;
            while(*p&&*q){int x=(int)utf_unit(&p)-(int)utf_unit(&q);if(x)return iv(x);}
            int remaining=0;while(*p){utf_unit(&p);remaining++;}while(*q){utf_unit(&q);remaining--;}return iv(remaining);
        }
        if(!strcmp(n,"replace")&&!strcmp(d,"(CC)Ljava/lang/String;")) {
            char *buf=(char *)alloc(v,strlen(s)*3+1),*out=buf;const unsigned char *p=(const unsigned char *)s;
            while(*p){unsigned ch=utf_unit(&p);out+=write_unit(out,ch==(uint16_t)integer(a[1])?(uint16_t)integer(a[2]):ch);}
            *out=0;Object *r=string(v,buf);release(v,buf);return rv(r);
        }
        if(!strcmp(n,"charAt")&&!strcmp(d,"(I)C")) {
            int target=integer(a[1]), pos=0; const unsigned char *p=(const unsigned char *)s;
            while(*p) { unsigned ch=*p++; if(ch>=0xe0&&p[0]&&p[1]) { ch=((ch&15)<<12)|((p[0]&63)<<6)|(p[1]&63); p+=2; } else if(ch>=0xc0&&*p) ch=((ch&31)<<6)|(*p++&63);
                if(pos++==target) return iv((int32_t)ch);
            }
            throwing(v,"java/lang/StringIndexOutOfBoundsException"); return none;
        }
        if(isstatic&&!strcmp(n,"valueOf")&&!strcmp(d,"(Ljava/lang/Object;)Ljava/lang/String;"))return object_string(v,a[0]);
        if(isstatic&&!strcmp(n,"valueOf")&&(!strcmp(d,"(I)Ljava/lang/String;")||!strcmp(d,"(J)Ljava/lang/String;"))) { char b[128]; return rv(string(v,as_text(v,a[0],b))); }
    }
    if(!strcmp(cl,"java/lang/StringBuilder")) {
        if(!strcmp(n,"append")&&(!strcmp(d,"(Ljava/lang/CharSequence;)Ljava/lang/StringBuilder;")||!strcmp(d,"(Ljava/lang/CharSequence;II)Ljava/lang/StringBuilder;"))) {
            builder_sequence(v,self,obj(a[1]),na==4?integer(a[2]):0,na==4?integer(a[3]):0,na==2);return a[0];
        }
        if(!strcmp(n,"setLength")&&!strcmp(d,"(I)V")){builder_length(v,self,integer(a[1]));return none;}
        if(!strcmp(n,"charAt")&&!strcmp(d,"(I)C")) {
            int32_t index=integer(a[1]),position=0;const unsigned char *p=(const unsigned char *)(self->text?self->text:"");
            while(*p){unsigned ch=utf_unit(&p);if(position++==index)return iv((int32_t)ch);}
            throwing(v,"java/lang/StringIndexOutOfBoundsException");return none;
        }
        if(!strcmp(n,"length")&&!strcmp(d,"()I")) {
            const unsigned char *s=(const unsigned char *)(self->text?self->text:"");int32_t length=0;
            while(*s){utf_unit(&s);length++;}return iv(length);
        }
        if(!strcmp(n,"append")&&!strcmp(d,"(C)Ljava/lang/StringBuilder;")) {
            size_t x=self->text?strlen(self->text):0;char *buf=(char *)alloc(v,x+4);
            if(x)memcpy(buf,self->text,x);x+=write_unit(buf+x,(uint16_t)integer(a[1]));buf[x]=0;
            set_text(v,self,buf);release(v,buf);return a[0];
        }
        if(!strcmp(n,"append")&&na==2&&(!strcmp(d,"(I)Ljava/lang/StringBuilder;")||!strcmp(d,"(J)Ljava/lang/StringBuilder;")||!strcmp(d,"(Z)Ljava/lang/StringBuilder;")||!strcmp(d,"(Ljava/lang/String;)Ljava/lang/StringBuilder;")||!strcmp(d,"(Ljava/lang/Object;)Ljava/lang/StringBuilder;"))) {
            Value text=a[1];int converted=!strcmp(d,"(Ljava/lang/Object;)Ljava/lang/StringBuilder;");
            if(converted){text=object_string(v,text);if(v->exception)return none;root(v,obj(text));}
            char b[128]; const char *s=d[1]=='Z'?(integer(a[1])?"true":"false"):as_text(v,text,b); size_t x=self->text?strlen(self->text):0, y=strlen(s);
            char *t=(char *)alloc(v,x+y+1); if(x) memcpy(t,self->text,x); memcpy(t+x,s,y+1); set_text(v,self,t); release(v,t);
            if(converted)v->nr--;return a[0];
        }
        if(!strcmp(n,"toString")&&!strcmp(d,"()Ljava/lang/String;")) return rv(string(v,self->text?self->text:""));
    }
    if(!strcmp(cl,"java/lang/System")&&isstatic) {
        if(!strcmp(n,"getSecurityManager")&&!strcmp(d,"()Ljava/lang/SecurityManager;"))return rv(NULL);
        int get=!strcmp(n,"getProperty"),set=!strcmp(n,"setProperty"),clear=!strcmp(n,"clearProperty");
        if((get||set||clear)&&
           ((!strcmp(d,"(Ljava/lang/String;)Ljava/lang/String;")&&(get||clear))||
            (!strcmp(d,"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;")&&(get||set)))) {
            Object *key=nonnull(v,a[0]);if(!key)return none;
            if(!*key->text){throwing(v,"java/lang/IllegalArgumentException");return none;}
            Object *value=set?nonnull(v,a[1]):NULL;if(set&&!value)return none;
            Property *p=property(v,key->text,set);
            Object *previous=p&&p->value?string(v,p->value):NULL;
            if(set){release(v,p->value);p->value=copy(v,value->text);}
            if(clear&&p){release(v,p->value);p->value=NULL;}
            return previous?rv(previous):get&&na==2?a[1]:rv(NULL);
        }
        if(!strcmp(n,"gc")&&!strcmp(d,"()V")) { collect(v); return none; }
        if(!strcmp(n,"currentTimeMillis")&&!strcmp(d,"()J")) return val((uint64_t)time(NULL)*1000,LONG);
        if(!strcmp(n,"nanoTime")&&!strcmp(d,"()J"))return val(vm_millis()*1000000,LONG);
        if(!strcmp(n,"identityHashCode")&&!strcmp(d,"(Ljava/lang/Object;)I"))return iv((int32_t)(uintptr_t)obj(a[0]));
        /* arraycopy is implemented below after type checking helpers. */
    }
    if(!strcmp(cl,"java/lang/Math")&&isstatic) {
        if(!strcmp(n,"abs")&&!strcmp(d,"(I)I")) return iv(integer(a[0])<0?(int32_t)(0U-(uint32_t)a[0].bits):integer(a[0]));
        if(!strcmp(n,"abs")&&!strcmp(d,"(J)J")) return val((int64_t)a[0].bits<0?0-a[0].bits:a[0].bits,LONG);
        if(!strcmp(n,"sqrt")&&!strcmp(d,"(D)D")) return dv(sqrt(dbl(a[0])));
        if((!strcmp(n,"min")||!strcmp(n,"max"))&&!strcmp(d,"(II)I")) { int less=integer(a[0])<integer(a[1]); return (!strcmp(n,"min")?less:!less)?a[0]:a[1]; }
        if((!strcmp(n,"min")||!strcmp(n,"max"))&&!strcmp(d,"(JJ)J")) {int less=(int64_t)a[0].bits<(int64_t)a[1].bits;return (!strcmp(n,"min")?less:!less)?a[0]:a[1];}
        if((!strcmp(n,"min")||!strcmp(n,"max"))&&(!strcmp(d,"(FF)F")||!strcmp(d,"(DD)D"))) {
            double x=d[1]=='F'?(double)flt(a[0]):dbl(a[0]),y=d[1]=='F'?(double)flt(a[1]):dbl(a[1]);
            int minimum=!strcmp(n,"min");
            if(isnan(x))return a[0];if(isnan(y))return a[1];
            if(x==0&&y==0)return (minimum?signbit(x):!signbit(x))?a[0]:a[1];
            return (minimum?x<=y:x>=y)?a[0]:a[1];
        }
    }
    if((!strcmp(cl,"java/lang/Integer")||!strcmp(cl,"java/lang/Long"))&&isstatic&&!strcmp(n,"toString")&&
       (!strcmp(d,"(I)Ljava/lang/String;")||!strcmp(d,"(J)Ljava/lang/String;"))) { char b[128]; return rv(string(v,as_text(v,a[0],b))); }
    if(isstatic&&(!strcmp(cl,"java/lang/Integer")||!strcmp(cl,"java/lang/Long"))) {
        int large=!strcmp(cl,"java/lang/Long");
        if(!strcmp(n,"compare")&&!strcmp(d,large?"(JJ)I":"(II)I")) {
            int64_t x=large?(int64_t)a[0].bits:integer(a[0]),y=large?(int64_t)a[1].bits:integer(a[1]);return iv(x<y?-1:x>y?1:0);
        }
        if(!strcmp(d,large?"(JJ)J":"(II)I")) {
            if(!strcmp(n,"sum"))return large?val(a[0].bits+a[1].bits,LONG):iv((int32_t)(uint32_t)(a[0].bits+a[1].bits));
            if(!strcmp(n,"min")||!strcmp(n,"max")) {
                int64_t x=large?(int64_t)a[0].bits:integer(a[0]),y=large?(int64_t)a[1].bits:integer(a[1]);return (!strcmp(n,"min")?x<y:x>y)?a[0]:a[1];
            }
        }
    }
    if(!strcmp(cl,"java/lang/Integer")&&isstatic&&!strcmp(n,"numberOfLeadingZeros")&&!strcmp(d,"(I)I")) {
        uint32_t x=(uint32_t)a[0].bits;int count=0;if(!x)return iv(32);
        while(!(x&0x80000000U)){count++;x<<=1;}return iv(count);
    }
    if(isstatic&&(!strcmp(cl,"java/lang/Integer")||!strcmp(cl,"java/lang/Long"))&&(!strcmp(d,"(I)I")||!strcmp(d,"(J)I"))) {
        unsigned width=d[1]=='J'?64:32;uint64_t x=width==64?a[0].bits:(uint32_t)a[0].bits;int count=0;
        if(!strcmp(n,"bitCount")){while(x){x&=x-1;count++;}return iv(count);}
        if(!strcmp(n,"numberOfTrailingZeros")){if(!x)return iv((int32_t)width);while(!(x&1)){count++;x>>=1;}return iv(count);}
        if(!strcmp(n,"numberOfLeadingZeros")){if(!x)return iv((int32_t)width);uint64_t mask=UINT64_C(1)<<(width-1);while(!(x&mask)){count++;x<<=1;}return iv(count);}
    }
    if(isstatic&&!strcmp(n,"isNaN")) {
        if(!strcmp(cl,"java/lang/Float")&&!strcmp(d,"(F)Z"))return iv(isnan(flt(a[0]))!=0);
        if(!strcmp(cl,"java/lang/Double")&&!strcmp(d,"(D)Z"))return iv(isnan(dbl(a[0]))!=0);
    }
    if(isstatic&&!strcmp(cl,"java/lang/Float")) {
        if((!strcmp(n,"floatToRawIntBits")||!strcmp(n,"floatToIntBits"))&&!strcmp(d,"(F)I"))return iv(!strcmp(n,"floatToIntBits")&&isnan(flt(a[0]))?0x7fc00000:(int32_t)a[0].bits);
        if(!strcmp(n,"intBitsToFloat")&&!strcmp(d,"(I)F"))return val((uint32_t)integer(a[0]),FLOAT);
    }
    if(isstatic&&!strcmp(cl,"java/lang/Double")) {
        if(!strcmp(n,"isNaN")&&!strcmp(d,"(D)Z"))return iv(isnan(dbl(a[0]))!=0);
        if(!strcmp(n,"isInfinite")&&!strcmp(d,"(D)Z"))return iv(isinf(dbl(a[0]))!=0);
        if((!strcmp(n,"doubleToRawLongBits")||!strcmp(n,"doubleToLongBits"))&&!strcmp(d,"(D)J"))return val(!strcmp(n,"doubleToLongBits")&&isnan(dbl(a[0]))?UINT64_C(0x7ff8000000000000):a[0].bits,LONG);
        if(!strcmp(n,"longBitsToDouble")&&!strcmp(d,"(J)D"))return val(a[0].bits,DOUBLE);
    }
    if(!strcmp(cl,"java/lang/Character")&&isstatic) {
        if((!strcmp(n,"toLowerCase")||!strcmp(n,"toUpperCase")||!strcmp(n,"toTitleCase"))&&(!strcmp(d,"(I)I")||!strcmp(d,"(C)C")))
            return iv((int32_t)case_simple((unsigned)integer(a[0]),!strcmp(n,"toLowerCase")?0:!strcmp(n,"toUpperCase")?1:2));
        if((!strcmp(n,"isLowerCase")||!strcmp(n,"isUpperCase")||!strcmp(n,"isTitleCase"))&&(!strcmp(d,"(I)Z")||!strcmp(d,"(C)Z")))
            return iv((case_flags((unsigned)integer(a[0]))&(!strcmp(n,"isLowerCase")?16:!strcmp(n,"isUpperCase")?32:64))!=0);
        if(!strcmp(n,"charCount")&&!strcmp(d,"(I)I"))return iv(integer(a[0])>=0x10000?2:1);
        if((!strcmp(n,"isJavaIdentifierStart")||!strcmp(n,"isJavaIdentifierPart"))&&(!strcmp(d,"(I)Z")||!strcmp(d,"(C)Z"))) {
            uint32_t ch=(uint32_t)integer(a[0]);size_t lo=0,hi=sizeof identifier_ranges/sizeof *identifier_ranges;
            while(lo<hi){size_t mid=lo+(hi-lo)/2;
                if(ch<identifier_ranges[mid].lo)hi=mid;
                else if(ch>identifier_ranges[mid].hi)lo=mid+1;
                else return iv((identifier_ranges[mid].flags&(!strcmp(n,"isJavaIdentifierStart")?1:2))!=0);}
            return iv(0);
        }
    }
missing:
    fail(v,"runtime method not implemented: %s.%s%s",cl,n,d); return none;
}
static void thread_entry(void *opaque) {
    VmThread *t=(VmThread *)opaque;VM *v=t->vm;Object *o=t->object;
    Method *m=method(o->cls,"run","()V");Value arg=rv(o);
    if(m)execute(v,m,&arg,1);
    else native_call(v,load(v,"java/lang/Thread"),"run","()V",&arg,1,0);
    if(v->exception){fprintf(stderr,"Uncaught Java exception in thread %s: %s\n",o->text?o->text:"?",v->exception->cls->name);v->exception=NULL;}
    clear_locals(v,t);t->finished=1;t->state=T_DONE;v->live_threads--;schedule(v);fail(v,"terminated thread resumed");
}
static int assignable(VM *v,Object *o,const char *name) {
    if(!o) return 1;
    if(name[0]!='[') {
        if(o->kind=='a') return !strcmp(name,"java/lang/Object")||!strcmp(name,"java/lang/Cloneable")||!strcmp(name,"java/io/Serializable");
        return subtype(o->cls,load(v,name));
    }
    if(o->kind!='a') return 0;
    if(!strcmp(name,o->array_desc)) return 1;
    /* Reference-array covariance, including nested arrays. */
    const char *source=o->array_desc+1, *target=name+1;
    if(*source=='L'&&*target=='L') {
        char s[512],t[512]; size_t sn=strlen(source),tn=strlen(target);
        if(sn<3||tn<3||sn>=sizeof s||tn>=sizeof t) fail(v,"bad array descriptor");
        memcpy(s,source+1,sn-2); s[sn-2]=0; memcpy(t,target+1,tn-2); t[tn-2]=0;
        return subtype(load(v,s),load(v,t));
    }
    if(*source=='['&&*target=='L') return !strcmp(target,"Ljava/lang/Object;")||!strcmp(target,"Ljava/lang/Cloneable;")||!strcmp(target,"Ljava/io/Serializable;");
    if(*source=='['&&*target=='[') { Object fake=*o; fake.array_desc=(char *)source; return assignable(v,&fake,target); }
    return 0;
}
static int array_accepts(VM *v,Object *array,Value x) {
    const char *d=array->array_desc+1;
    if(x.tag!=REF) return 0;
    if(*d=='[') return assignable(v,obj(x),d);
    if(*d=='L') {
        char b[512]; size_t n=strlen(d); if(n<3||n>=sizeof b) fail(v,"bad array component");
        memcpy(b,d+1,n-2); b[n-2]=0; return assignable(v,obj(x),b);
    }
    return 0;
}
static Object *array_new(VM *v,const char *desc,int32_t n) {
    if(n<0) { throwing(v,"java/lang/NegativeArraySizeException"); return NULL; }
    Object *o=new_object(v,load(v,desc),'a',(size_t)n);
    root(v,o); size_t bytes=strlen(desc)+1; heap_room(v,bytes);
    o->array_desc=(char *)malloc(bytes); if(!o->array_desc) fail(v,"out of native memory");
    memcpy(o->array_desc,desc,bytes); o->bytes+=bytes; v->heap+=bytes; v->nr--;
    for(int32_t i=0;i<n;i++) o->data[i]=zero(desc+1);
    return o;
}
static Object *multi_array(VM *v,const char *desc,int32_t *sizes,unsigned dims) {
    Object *o=array_new(v,desc,sizes[0]); if(!o) return NULL;
    if(dims>1) {
        root(v,o);
        for(size_t i=0;i<o->count;i++) { o->data[i]=rv(multi_array(v,desc+1,sizes+1,dims-1)); if(v->exception) break; }
        v->nr--;
    }
    return o;
}
static Value arraycopy(VM *v,Value *a) {
    Object *s=nonnull(v,a[0]), *t=nonnull(v,a[2]);
    if(v->exception) return iv(0);
    if(s->kind!='a'||t->kind!='a') { throwing(v,"java/lang/ArrayStoreException"); return iv(0); }
    const char *sd=s->array_desc+1,*td=t->array_desc+1;
    int sr=*sd=='L'||*sd=='[', tr=*td=='L'||*td=='[';
    if(sr!=tr||(!sr&&strcmp(sd,td))) { throwing(v,"java/lang/ArrayStoreException"); return iv(0); }
    int32_t x=integer(a[1]),y=integer(a[3]),n=integer(a[4]);
    if(x<0||y<0||n<0||(size_t)x>s->count||(size_t)y>t->count||(size_t)n>s->count-(size_t)x||(size_t)n>t->count-(size_t)y) {
        throwing(v,"java/lang/ArrayIndexOutOfBoundsException"); return iv(0);
    }
    if(s==t||!sr) memmove(t->data+y,s->data+x,(size_t)n*sizeof(Value));
    else for(int32_t i=0;i<n;i++) { if(!array_accepts(v,t,s->data[x+i])) { throwing(v,"java/lang/ArrayStoreException"); break; } t->data[y+i]=s->data[x+i]; }
    return iv(0);
}
static int64_t float_integer(double x,int bits) {
    if(isnan(x)) return 0;
    if(bits==32) { if(x>=2147483647.0) return INT32_MAX; if(x<=-2147483648.0) return INT32_MIN; }
    else { if(x>=9223372036854775808.0) return INT64_MAX; if(x<=-9223372036854775808.0) return INT64_MIN; }
    return (int64_t)x;
}
static Value arithmetic(VM *v,unsigned op,Value a,Value b) {
    unsigned group=(op-0x60)/4, type=(op-0x60)%4;
    if(type==0) {
        uint32_t x=(uint32_t)a.bits,y=(uint32_t)b.bits,z=0; int32_t sx=(int32_t)x,sy=(int32_t)y;
        if(group>=3&&!y) { throwing(v,"java/lang/ArithmeticException"); return iv(0); }
        switch(group) {
        case 0:z=x+y;break; case 1:z=x-y;break;case 2:z=x*y;break;
        case 3:z=sx==INT32_MIN&&sy==-1?x:(uint32_t)(sx/sy);break;
        case 4:z=sx==INT32_MIN&&sy==-1?0:(uint32_t)(sx%sy);break;
        } return val(z,INT);
    }
    if(type==1) {
        uint64_t x=a.bits,y=b.bits,z=0; int64_t sx=(int64_t)x,sy=(int64_t)y;
        if(group>=3&&!y) { throwing(v,"java/lang/ArithmeticException"); return val(0,LONG); }
        switch(group) {
        case 0:z=x+y;break;case 1:z=x-y;break;case 2:z=x*y;break;
        case 3:z=sx==INT64_MIN&&sy==-1?x:(uint64_t)(sx/sy);break;
        case 4:z=sx==INT64_MIN&&sy==-1?0:(uint64_t)(sx%sy);break;
        } return val(z,LONG);
    }
    if(type==2) { float x=flt(a),y=flt(b),z=0; switch(group) {case 0:z=x+y;break;case 1:z=x-y;break;case 2:z=x*y;break;case 3:z=x/y;break;case 4:z=fmodf(x,y);break;} return fv(z); }
    double x=dbl(a),y=dbl(b),z=0; switch(group) {case 0:z=x+y;break;case 1:z=x-y;break;case 2:z=x*y;break;case 3:z=x/y;break;case 4:z=fmod(x,y);break;} return dv(z);
}
static void stackop(VM *v,Frame *f,unsigned op) {
    Value a=pop(v,f),b,c,d;
    switch(op) {
    case 0x57: if(wide(a)) fail(v,"pop on category 2"); break;
    case 0x58: if(!wide(a)) { b=pop(v,f); if(wide(b)) fail(v,"invalid pop2"); } break;
    case 0x59: if(wide(a)) fail(v,"invalid dup"); push(v,f,a);push(v,f,a);break;
    case 0x5a: b=pop(v,f); if(wide(a)||wide(b)) fail(v,"invalid dup_x1"); push(v,f,a);push(v,f,b);push(v,f,a);break;
    case 0x5b: b=pop(v,f); if(wide(a)) fail(v,"invalid dup_x2");
        if(wide(b)) { push(v,f,a);push(v,f,b);push(v,f,a); }
        else { c=pop(v,f);if(wide(c)) fail(v,"invalid dup_x2");push(v,f,a);push(v,f,c);push(v,f,b);push(v,f,a); }break;
    case 0x5c: if(wide(a)) {push(v,f,a);push(v,f,a);} else {b=pop(v,f);if(wide(b))fail(v,"invalid dup2");push(v,f,b);push(v,f,a);push(v,f,b);push(v,f,a);}break;
    case 0x5d:
        b=pop(v,f);
        if(wide(a)) { if(wide(b)) fail(v,"invalid dup2_x1");push(v,f,a);push(v,f,b);push(v,f,a); }
        else {c=pop(v,f);if(wide(b)||wide(c))fail(v,"invalid dup2_x1");push(v,f,b);push(v,f,a);push(v,f,c);push(v,f,b);push(v,f,a);}break;
    case 0x5e:
        b=pop(v,f);
        if(wide(a)) {
            if(wide(b)) {push(v,f,a);push(v,f,b);push(v,f,a);}
            else {c=pop(v,f);if(wide(c))fail(v,"invalid dup2_x2");push(v,f,a);push(v,f,c);push(v,f,b);push(v,f,a);}
        } else {
            if(wide(b))fail(v,"invalid dup2_x2");c=pop(v,f);
            if(wide(c)) {push(v,f,b);push(v,f,a);push(v,f,c);push(v,f,b);push(v,f,a);}
            else {d=pop(v,f);if(wide(d))fail(v,"invalid dup2_x2");push(v,f,b);push(v,f,a);push(v,f,d);push(v,f,c);push(v,f,b);push(v,f,a);}
        }break;
    case 0x5f: b=pop(v,f);if(wide(a)||wide(b))fail(v,"invalid swap");push(v,f,a);push(v,f,b);break;
    }
}
static Value execute(VM *v,Method *m,Value *args,unsigned count) {
    Value result=iv(0);
    if(!m->code) fail(v,"method has no executable Code: %s.%s%s",m->owner->name,m->name,m->desc);
    if(++v->depth>MAX_DEPTH) fail(v,"maximum call depth %d exceeded",MAX_DEPTH);
    Frame *f=(Frame *)alloc(v,sizeof(Frame)); f->method=m;
    f->locals=(Value *)alloc(v,m->locals*sizeof(Value)); f->stack=(Value *)alloc(v,((size_t)m->stack+1)*sizeof(Value));
    f->prev=v->frame; v->frame=f;
    unsigned l=0; for(unsigned i=0;i<count;i++) { local_set(v,f,l,args[i]); l+=wide(args[i])?2:1; }
    if(m->flags&0x0020) {
        f->method_lock=(m->flags&STATIC)?class_mirror(v,m->owner):obj(args[0]);
        if(!monitor_enter(v,f->method_lock))goto done;
    }
    while(f->pc<m->length) {
        if(v->opt.instruction_limit && v->steps>=v->opt.instruction_limit) fail(v,"instruction budget exceeded");
        if((v->steps++&4095)==0&&v->opt.cancelled&&v->opt.cancelled()) fail(v,"cancelled by user");
        if((v->steps&255)==0)schedule(v);
        f->ip=f->pc; unsigned op=code(v,f,1); Value a,b; unsigned idx;
        if(op>=0x02&&op<=0x08) {push(v,f,iv((int32_t)op-3));continue;}
        if(op>=0x1a&&op<=0x2d) {push(v,f,local_get(v,f,(op-0x1a)%4));continue;}
        if(op>=0x3b&&op<=0x4e) {local_set(v,f,(op-0x3b)%4,pop(v,f));continue;}
        if(op>=0x60&&op<=0x73) {b=pop(v,f);a=pop(v,f);Value r=arithmetic(v,op,a,b);if(!v->exception)push(v,f,r);goto exceptions;}
        if(op>=0x57&&op<=0x5f) {stackop(v,f,op);continue;}
        switch(op) {
        case 0x00:break;
        case 0x01:push(v,f,rv(NULL));break;
        case 0x09:case 0x0a:push(v,f,val(op-0x09,LONG));break;
        case 0x0b:case 0x0c:case 0x0d:push(v,f,fv((float)(op-0x0b)));break;
        case 0x0e:case 0x0f:push(v,f,dv((double)(op-0x0e)));break;
        case 0x10:push(v,f,iv((int8_t)code(v,f,1)));break;
        case 0x11:push(v,f,iv((int16_t)code(v,f,2)));break;
        case 0x12:case 0x13:case 0x14:idx=code(v,f,op==0x12?1:2);push(v,f,constant(v,m->owner,idx));break;
        case 0x15:case 0x16:case 0x17:case 0x18:case 0x19:idx=code(v,f,1);push(v,f,local_get(v,f,idx));break;
        case 0x36:case 0x37:case 0x38:case 0x39:case 0x3a:idx=code(v,f,1);local_set(v,f,idx,pop(v,f));break;
        case 0x2e:case 0x2f:case 0x30:case 0x31:case 0x32:case 0x33:case 0x34:case 0x35: {
            int32_t i=integer(pop(v,f)); Object *o=nonnull(v,pop(v,f)); if(!o)break;
            if(o->kind!='a')fail(v,"array expected");
            if(i<0||(size_t)i>=o->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");break;}
            push(v,f,o->data[i]);break;
        }
        case 0x4f:case 0x50:case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56: {
            a=pop(v,f);int32_t i=integer(pop(v,f));Object *o=nonnull(v,pop(v,f));if(!o)break;
            if(o->kind!='a')fail(v,"array expected");
            if(i<0||(size_t)i>=o->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");break;}
            if(op==0x53&&!array_accepts(v,o,a)){throwing(v,"java/lang/ArrayStoreException");break;}
            if(op==0x54)a=iv(o->array_desc[1]=='Z'?(integer(a)&1):(int8_t)integer(a));
            if(op==0x55)a=iv((uint16_t)integer(a));if(op==0x56)a=iv((int16_t)integer(a));o->data[i]=a;break;
        }
        case 0x74:a=pop(v,f);push(v,f,val(0U-(uint32_t)a.bits,INT));break;
        case 0x75:a=pop(v,f);push(v,f,val(0-a.bits,LONG));break;
        case 0x76:a=pop(v,f);push(v,f,fv(-flt(a)));break;
        case 0x77:a=pop(v,f);push(v,f,dv(-dbl(a)));break;
        case 0x78:case 0x79:case 0x7a:case 0x7b:case 0x7c:case 0x7d: {
            b=pop(v,f);a=pop(v,f);unsigned bits=(op&1)?64:32,shift=(unsigned)b.bits&(bits-1);uint64_t x=a.bits;
            if(bits==32)x=(uint32_t)x;
            uint64_t z;
            if(op<=0x79)z=x<<shift;
            else {z=x>>shift;if(op<=0x7b&&shift&&(x&((uint64_t)1<<(bits-1))))z|=UINT64_MAX<<(bits-shift);}
            push(v,f,val(bits==32?(uint32_t)z:z,bits==32?INT:LONG));break;
        }
        case 0x7e:case 0x7f:case 0x80:case 0x81:case 0x82:case 0x83:
            b=pop(v,f);a=pop(v,f);push(v,f,val(op<=0x7f?a.bits&b.bits:op<=0x81?a.bits|b.bits:a.bits^b.bits,(op&1)?LONG:INT));break;
        case 0x84: {idx=code(v,f,1);int32_t n=(int8_t)code(v,f,1);a=local_get(v,f,idx);local_set(v,f,idx,val((uint32_t)a.bits+(uint32_t)n,INT));break;}
        case 0x85:a=pop(v,f);push(v,f,val((uint64_t)(int64_t)integer(a),LONG));break;
        case 0x86:a=pop(v,f);push(v,f,fv((float)integer(a)));break;
        case 0x87:a=pop(v,f);push(v,f,dv((double)integer(a)));break;
        case 0x88:a=pop(v,f);push(v,f,iv((int32_t)a.bits));break;
        case 0x89:a=pop(v,f);push(v,f,fv((float)(int64_t)a.bits));break;
        case 0x8a:a=pop(v,f);push(v,f,dv((double)(int64_t)a.bits));break;
        case 0x8b:case 0x8e:a=pop(v,f);push(v,f,iv((int32_t)float_integer(op==0x8b?flt(a):dbl(a),32)));break;
        case 0x8c:case 0x8f:a=pop(v,f);push(v,f,val((uint64_t)float_integer(op==0x8c?flt(a):dbl(a),64),LONG));break;
        case 0x8d:a=pop(v,f);push(v,f,dv(flt(a)));break;
        case 0x90:a=pop(v,f);push(v,f,fv((float)dbl(a)));break;
        case 0x91:a=pop(v,f);push(v,f,iv((int8_t)integer(a)));break;
        case 0x92:a=pop(v,f);push(v,f,iv((uint16_t)integer(a)));break;
        case 0x93:a=pop(v,f);push(v,f,iv((int16_t)integer(a)));break;
        case 0x94:b=pop(v,f);a=pop(v,f);push(v,f,iv((int64_t)a.bits<(int64_t)b.bits?-1:(int64_t)a.bits>(int64_t)b.bits?1:0));break;
        case 0x95:case 0x96:case 0x97:case 0x98: {
            b=pop(v,f);a=pop(v,f);double x=op<=0x96?flt(a):dbl(a),y=op<=0x96?flt(b):dbl(b);
            push(v,f,iv(isnan(x)||isnan(y)?((op==0x95||op==0x97)?-1:1):x<y?-1:x>y?1:0));break;
        }
        case 0x99:case 0x9a:case 0x9b:case 0x9c:case 0x9d:case 0x9e:
        case 0x9f:case 0xa0:case 0xa1:case 0xa2:case 0xa3:case 0xa4:case 0xa5:case 0xa6: {
            int16_t offset=(int16_t)code(v,f,2);int take=0;
            b=pop(v,f);
            if(op>=0xa5){a=pop(v,f);take=(a.bits==b.bits)==(op==0xa5);}
            else {int32_t x,y;unsigned cmp;
                if(op>=0x9f){a=pop(v,f);x=integer(a);y=integer(b);cmp=op-0x9f;}
                else{x=integer(b);y=0;cmp=op-0x99;}
                switch(cmp){case 0:take=x==y;break;case 1:take=x!=y;break;case 2:take=x<y;break;case 3:take=x>=y;break;case 4:take=x>y;break;case 5:take=x<=y;break;}
            }
            if(take)branch(v,f,offset);break;
        }
        case 0xa7: {int16_t offset=(int16_t)code(v,f,2);branch(v,f,offset);break;}
        case 0xaa:case 0xab: {
            int32_t key=integer(pop(v,f));while(f->pc&3)(void)code(v,f,1);
            int32_t target=(int32_t)code(v,f,4);
            if(op==0xaa) {
                int32_t low=(int32_t)code(v,f,4),high=(int32_t)code(v,f,4);
                int64_t n=(int64_t)high-low+1;if(n<0||n>(m->length-f->pc)/4)fail(v,"bad tableswitch");
                for(int64_t j=0;j<n;j++){int32_t off=(int32_t)code(v,f,4);if((int64_t)key==low+j)target=off;}
            } else {
                int32_t n=(int32_t)code(v,f,4);if(n<0||(uint32_t)n>(m->length-f->pc)/8)fail(v,"bad lookupswitch");
                for(int32_t j=0;j<n;j++){int32_t k=(int32_t)code(v,f,4),off=(int32_t)code(v,f,4);if(key==k)target=off;}
            }
            branch(v,f,target);break;
        }
        case 0xac:case 0xad:case 0xae:case 0xaf:case 0xb0:result=pop(v,f);goto done;
        case 0xb1:goto done;
        case 0xb2:case 0xb3:case 0xb4:case 0xb5: {
            CP *p=cp(v,m->owner,code(v,f,2),9),*nt=cp(v,m->owner,p->b,12);
            Class *c=load(v,classname(v,m->owner,p->a)); Field *fld=field(v,&c,utf(v,m->owner,nt->a),utf(v,m->owner,nt->b));
            if((op<=0xb3)!=!!(fld->flags&STATIC))fail(v,"field static/instance mismatch");
            if(op<=0xb3) {initialize(v,c);if(v->exception)break;if(op==0xb2)push(v,f,fld->value);else fld->value=pop(v,f);}
            else {if(op==0xb5)a=pop(v,f);Object *o=nonnull(v,pop(v,f));if(!o)break;
                if(!subtype(o->cls,c)||fld->slot>=o->count)fail(v,"invalid field receiver");
                if(op==0xb4)push(v,f,o->data[fld->slot]);else o->data[fld->slot]=a;
            }break;
        }
        case 0xb6:case 0xb7:case 0xb8:case 0xb9: {
            CP *p=cp(v,m->owner,code(v,f,2),0);if(p->tag!=10&&p->tag!=11)fail(v,"invalid method reference");
            CP *nt=cp(v,m->owner,p->b,12);Class *c=load(v,classname(v,m->owner,p->a));
            const char *n=utf(v,m->owner,nt->a),*d=utf(v,m->owner,nt->b);int stat=op==0xb8;
            if(op==0xb9){(void)code(v,f,1);if(code(v,f,1))fail(v,"invalid invokeinterface");}
            unsigned na=nargs(v,d)+(stat?0:1);if(na>f->sp)fail(v,"not enough method arguments");
            Value *aa=f->stack+f->sp-na;Method *target=NULL;
            if(stat) {target=method(c,n,d);initialize(v,target?target->owner:c);if(v->exception)break;}
            else {
                Object *o=nonnull(v,aa[0]);if(!o)break;Method *resolved=method(c,n,d);
                /* Private methods are not virtual, even when newer javac uses
                 * invokevirtual or a REF_invokeVirtual method handle. */
                target=op==0xb7||(resolved&&(resolved->flags&2))?resolved:method(o->cls,n,d);
                if(!target&&op!=0xb7)target=default_method(v,o->cls,n,d);if(v->exception)break;
            }
            Value res;
            if(target) {
                if(!!(target->flags&STATIC)!=stat)fail(v,"method static/instance mismatch");
                if(target->flags&NATIVE) {
                    if(!strcmp(target->owner->name,"java/util/concurrent/atomic/AtomicLong")&&!strcmp(n,"VMSupportsCS8")&&!strcmp(d,"()Z"))res=iv(1);
                    else if(!strcmp(target->owner->name,"nspire/xml/ExpatReader")&&!strcmp(n,"parse0")&&!strcmp(d,"(Lorg/xml/sax/InputSource;ZZ)V")&&!stat)res=native_call(v,target->owner,n,d,aa,na,stat);
                    else fail(v,"unbound native method: %s.%s%s",c->name,n,d);
                } else res=execute(v,target,aa,na);
            } else if(!strcmp(c->name,"java/lang/System")&&!strcmp(n,"arraycopy")&&!strcmp(d,"(Ljava/lang/Object;ILjava/lang/Object;II)V")&&stat) res=arraycopy(v,aa);
            else {
                Class *base=c;while(base&&!base->builtin)base=base->super;
                if(op!=0xb7&&!stat&&obj(aa[0])->kind=='s'&&
                    (!strcmp(n,"equals")||!strcmp(n,"hashCode")||!strcmp(n,"toString")||!strcmp(n,"compareTo")||!strcmp(n,"length")||!strcmp(n,"charAt")||!strcmp(n,"subSequence")))base=obj(aa[0])->cls;
                if(!stat&&obj(aa[0])->kind=='e'&&(!strcmp(n,"hasMoreElements")||!strcmp(n,"nextElement")))base=obj(aa[0])->cls;
                if(!stat&&obj(aa[0])->kind=='a'&&!strcmp(n,"clone"))base=load(v,"java/lang/Object");
                if(op!=0xb7&&!stat&&(obj(aa[0])->kind=='w'||obj(aa[0])->kind=='B')&&(!strcmp(n,"equals")||!strcmp(n,"hashCode")||!strcmp(n,"toString")||!strcmp(n,"compareTo")||strstr(n,"Value")))base=obj(aa[0])->cls;
                if(!base)fail(v,"method not found: %s.%s%s",c->name,n,d);
                res=native_call(v,base,n,d,aa,na,stat);
            }
            f->sp-=na;if(!v->exception&&return_type(v,d)!='V')push(v,f,res);break;
        }
        case 0xba: {
            unsigned index=code(v,f,2);if(code(v,f,2))fail(v,"invalid invokedynamic operands");
            CP *dynamic=cp(v,m->owner,index,18),*nt=cp(v,m->owner,dynamic->b,12);
            const char *desc=utf(v,m->owner,nt->b);unsigned count=nargs(v,desc);
            if(count>f->sp)fail(v,"not enough invokedynamic arguments");
            Value value=invoke_dynamic(v,m->owner,dynamic,desc,f->stack+f->sp-count,count);
            f->sp-=count;if(!v->exception)push(v,f,value);break;
        }
        case 0xbb: {
            Class *c=load(v,classname(v,m->owner,code(v,f,2)));initialize(v,c);if(v->exception)break;
            Object *o=new_object(v,c,'o',c->slots);
            for(Class *k=c;k;k=k->super)for(unsigned i=0;i<k->nf;i++)if(!(k->fields[i].flags&STATIC))o->data[k->fields[i].slot]=zero(k->fields[i].desc);
            push(v,f,rv(o));break;
        }
        case 0xbc:case 0xbd: {
            char desc[512];
            if(op==0xbc){unsigned type=code(v,f,1);const char *types="ZCFDBSIJ";if(type<4||type>11)fail(v,"bad newarray type");desc[0]='[';desc[1]=types[type-4];desc[2]=0;}
            else {const char *name=classname(v,m->owner,code(v,f,2));if(*name!='[')load(v,name);if(snprintf(desc,sizeof desc,*name=='['?"[%s":"[L%s;",name)>=(int)sizeof desc)fail(v,"array descriptor too long");}
            int32_t n=integer(pop(v,f));Object *o=array_new(v,desc,n);if(o)push(v,f,rv(o));break;
        }
        case 0xbe:{Object *o=nonnull(v,pop(v,f));if(o){if(o->kind!='a')fail(v,"array expected");push(v,f,iv((int32_t)o->count));}break;}
        case 0xbf:{Object *o=nonnull(v,pop(v,f));if(o)v->exception=o;break;}
        case 0xc0:case 0xc1: {
            const char *name=classname(v,m->owner,code(v,f,2));a=pop(v,f);if(a.tag!=REF)fail(v,"reference expected");
            int yes=assignable(v,obj(a),name);
            if(op==0xc1)push(v,f,iv(obj(a)&&yes));else if(yes)push(v,f,a);else throwing(v,"java/lang/ClassCastException");break;
        }
        case 0xc2:case 0xc3:{Object *o=nonnull(v,pop(v,f));if(o){if(op==0xc2)monitor_enter(v,o);else monitor_exit(v,o);}break;}
        case 0xc4: {
            unsigned sub=code(v,f,1);idx=code(v,f,2);
            if(sub>=0x15&&sub<=0x19)push(v,f,local_get(v,f,idx));
            else if(sub>=0x36&&sub<=0x3a)local_set(v,f,idx,pop(v,f));
            else if(sub==0x84){int32_t n=(int16_t)code(v,f,2);a=local_get(v,f,idx);local_set(v,f,idx,val((uint32_t)a.bits+(uint32_t)n,INT));}
            else fail(v,"unsupported wide opcode 0x%02x",sub);break;
        }
        case 0xc5: {
            const char *d=classname(v,m->owner,code(v,f,2));unsigned dims=code(v,f,1),rank=0;while(d[rank]=='[')rank++;
            if(!dims||dims>rank||dims>32||f->sp<dims)fail(v,"invalid multianewarray dimensions");
            int32_t sizes[32];for(unsigned i=dims;i;i--)sizes[i-1]=integer(pop(v,f));
            for(unsigned i=0;i<dims;i++)if(sizes[i]<0){throwing(v,"java/lang/NegativeArraySizeException");break;}
            if(!v->exception){Object *o=multi_array(v,d,sizes,dims);if(!v->exception)push(v,f,rv(o));}break;
        }
        case 0xc6:case 0xc7:{int16_t off=(int16_t)code(v,f,2);a=pop(v,f);if((!a.bits)==(op==0xc6))branch(v,f,off);break;}
        case 0xc8:{int32_t off=(int32_t)code(v,f,4);branch(v,f,off);break;}
        default:fail(v,"unsupported opcode 0x%02x",op);
        }
exceptions:
        if(v->exception) {
            int found=0;
            for(unsigned i=0;i<m->nh;i++) {
                Handler *h=&m->handlers[i];
                if(f->ip>=h->start&&f->ip<h->end&&(!h->type||subtype(v->exception->cls,load(v,classname(v,m->owner,h->type))))) {
                    f->sp=0;push(v,f,rv(v->exception));v->exception=NULL;f->pc=h->handler;found=1;break;
                }
            }
            if(!found)goto done;
        }
    }
    fail(v,"method fell off end of bytecode");
done:
    if(f->method_lock&&f->method_lock->monitor_owner==v->current)monitor_exit(v,f->method_lock);
    v->frame=f->prev;v->depth--;release(v,f->stack);release(v,f->locals);release(v,f);return result;
}
int vm_run(const VmOptions *opt,int argc,const char **argv) {
    VM *v=(VM *)calloc(1,sizeof(VM));if(!v){fputs("Cannot allocate VM\n",stderr);return 1;}
    v->opt=*opt;v->threshold=65536;int status=1;
    if(!setjmp(v->abort)) {
        if(opt->heap_limit<4096||opt->heap_limit>128U*1024U*1024U)fail(v,"heap must be 4096..134217728 bytes");
        init_properties(v);
        open_paths(v,opt->bootclasspath);v->nbootpaths=v->npaths;open_paths(v,opt->classpath);
        Class *thread_class=load(v,"java/lang/Thread");
        Object *main_object=new_object(v,thread_class,'o',thread_class->slots);
        for(unsigned i=0;i<thread_class->nf;i++)main_object->data[i]=zero(thread_class->fields[i].desc);
        VmThread *main_thread=thread_record(v,main_object);
        v->current=v->main_thread=main_thread;main_thread->state=T_RUN;v->live_threads=1;
        set_text(v,main_object,"main");
        char name[512];if(strlen(opt->main_class)>=sizeof name)fail(v,"main class name too long");strcpy(name,opt->main_class);
        for(char *p=name;*p;p++)if(*p=='.')*p='/';
        Class *c=load(v,name);initialize(v,c);
        if(!v->exception) {
            Method *m=method(c,"main","([Ljava/lang/String;)V");
            if(!m||!(m->flags&STATIC)||!(m->flags&1))fail(v,"public static main(String[]) not found in %s",name);
            Object *args=array_new(v,"[Ljava/lang/String;",argc);root(v,args);
            for(int i=0;i<argc;i++)args->data[i]=rv(string(v,argv[i]));
            Value a=rv(args);execute(v,m,&a,1);v->nr--;
        }
        if(v->exception)fprintf(stderr,"Uncaught Java exception: %s\n",v->exception->cls->name);
        else status=0;
        clear_locals(v,v->main_thread);v->main_thread->finished=1;v->live_threads--;
        if(workers_alive(v)){v->main_thread->state=T_DRAIN;schedule(v);}
    }
    while(v->xml_parsers)xml_destroy(v,v->xml_parsers);
    /* A fatal longjmp can leave Expat's callback-depth guard set. Its public
     * destructor then refuses to free it. No parser can run after this point;
     * release every remaining allocation through our tracked memory suite. */
    while(v->xml_mem)xml_free(v->xml_mem+1);
    for(unsigned i=0;i<v->npaths;i++)if(v->paths[i].is_zip)mz_zip_reader_end(&v->paths[i].zip);
    while(v->objects){Object *o=v->objects;v->objects=o->next;free(o->data);free(o->text);free(o->array_desc);free(o->buffer);free(o->resource_name);free(o);}
    while(v->mem){Mem *m=v->mem;v->mem=m->next;free(m);}free(v);return status;
}
