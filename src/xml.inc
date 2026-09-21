/* SAX/Expat bridge. MIT license; see LICENSE. Expat is independently licensed.
 * Every active parser is registered for cleanup even if VM execution aborts.
 * Expat allocations have an additional, per-VM 8 MiB budget. */
typedef struct XmlDecl { struct XmlDecl *next;XML_Char *element,*name,*type; } XmlDecl;
typedef struct XmlEntity { struct XmlEntity *next;XML_Char *name;size_t length; } XmlEntity;
struct XmlParse {
    XmlParse *next;VM *vm;XML_Parser parser;Object *reader;
    XmlDecl *declarations;XmlEntity *entities;int namespaces,disallow_doctype;
};
struct XmlMem { VM *vm;size_t size;XmlMem *prev,*next;long double alignment; };
static VM *xml_allocator_vm;
#define XML_MEMORY_LIMIT (8U*1024U*1024U)
static void *xml_malloc(size_t size) {
    VM *v=xml_allocator_vm;
    if(!v||size>XML_MEMORY_LIMIT||v->xml_bytes>XML_MEMORY_LIMIT-size)return NULL;
    XmlMem *m=(XmlMem *)malloc(sizeof *m+size);if(!m)return NULL;
    m->vm=v;m->size=size;m->prev=NULL;m->next=v->xml_mem;
    if(m->next)m->next->prev=m;v->xml_mem=m;v->xml_bytes+=size;return m+1;
}
static void xml_free(void *ptr) {
    if(!ptr)return;XmlMem *m=(XmlMem *)ptr-1;
    if(m->prev)m->prev->next=m->next;else m->vm->xml_mem=m->next;
    if(m->next)m->next->prev=m->prev;m->vm->xml_bytes-=m->size;free(m);
}
static void *xml_realloc(void *ptr,size_t size) {
    if(!ptr)return xml_malloc(size);
    XmlMem *m=(XmlMem *)ptr-1;VM *v=m->vm;size_t old=m->size;
    if(size>XML_MEMORY_LIMIT||v->xml_bytes-old>XML_MEMORY_LIMIT-size)return NULL;
    XmlMem *next=(XmlMem *)realloc(m,sizeof *m+size);if(!next)return NULL;
    if(next->prev)next->prev->next=next;else v->xml_mem=next;
    if(next->next)next->next->prev=next;
    next->size=size;v->xml_bytes=v->xml_bytes-old+size;return next+1;
}
static void xml_destroy(VM *v,XmlParse *p) {
    if(p->parser)XML_ParserFree(p->parser);
    while(p->declarations) {
        XmlDecl *d=p->declarations;p->declarations=d->next;
        release(v,d->element);release(v,d->name);release(v,d->type);release(v,d);
    }
    while(p->entities){XmlEntity *e=p->entities;p->entities=e->next;release(v,e->name);release(v,e);}
    XmlParse **next=&v->xml_parsers;while(*next&&*next!=p)next=&(*next)->next;
    if(*next)*next=p->next;release(v,p);
}
static size_t xml_length(const XML_Char *text) { size_t n=0;if(text)while(text[n])n++;return n; }
static Object *xml_string(VM *v,const XML_Char *text,size_t length) {
    if(!text)return NULL;
    if(length>META_LIMIT/3-1)fail(v,"XML string too large");
    char *s=(char *)alloc(v,length*3+1);size_t used=0;
    for(size_t i=0;i<length;i++)used+=write_unit(s+used,text[i]);s[used]=0;
    Object *result=string(v,s);release(v,s);return result;
}
static Object *xml_full_string(VM *v,const XML_Char *text) { return xml_string(v,text,xml_length(text)); }
static Value xml_call(XmlParse *p,const char *name,const char *desc,Value *args,unsigned count) {
    VM *v=p->vm;if(v->exception)return iv(0);
    Method *m=method(p->reader->cls,name,desc);if(!m)fail(v,"XML bridge method missing: %s",name);
    Value result=execute(v,m,args,count);
    xml_allocator_vm=v; /* A Java callback may parse on another thread/reader. */
    if(v->exception&&p->parser)XML_StopParser(p->parser,XML_FALSE);
    return result;
}
static void xml_position(XmlParse *p) {
    Value args[]={rv(p->reader),iv((int32_t)XML_GetCurrentLineNumber(p->parser)),iv((int32_t)XML_GetCurrentColumnNumber(p->parser)+1)};
    xml_call(p,"position","(II)V",args,3);
}
static void xml_fatal(XmlParse *p,const char *message) {
    VM *v=p->vm;xml_position(p);if(v->exception)return;
    Object *msg=string(v,message);root(v,msg);Value args[]={rv(p->reader),rv(msg)};
    xml_call(p,"fatal","(Ljava/lang/String;)V",args,2);v->nr--;
}
static int xml_equal(const XML_Char *a,const XML_Char *b) {
    while(*a&&*a==*b){a++;b++;}return *a==*b;
}
static XML_Char *xml_copy(VM *v,const XML_Char *text) {
    size_t n=xml_length(text)+1;XML_Char *copy=(XML_Char *)alloc(v,n*sizeof *copy);memcpy(copy,text,n*sizeof *copy);return copy;
}
/* Expat namespace triplets are URI <0x1f> local [<0x1f> prefix]. */
static void xml_name(XmlParse *p,Object *array,size_t offset,const XML_Char *name) {
    VM *v=p->vm;size_t length=xml_length(name),first=0,second;
    while(first<length&&name[first]!=31)first++;
    if(!p->namespaces||first==length) {
        array->data[offset]=rv(string(v,""));
        array->data[offset+1]=rv(p->namespaces?xml_full_string(v,name):string(v,""));
        array->data[offset+2]=rv(xml_full_string(v,name));return;
    }
    second=first+1;while(second<length&&name[second]!=31)second++;
    array->data[offset]=rv(xml_string(v,name,first));
    array->data[offset+1]=rv(xml_string(v,name+first+1,second-first-1));
    if(second==length)array->data[offset+2]=rv(xml_string(v,name+first+1,second-first-1));
    else {
        size_t prefix=length-second-1,local=second-first-1;
        XML_Char *q=(XML_Char *)alloc(v,(prefix+local+2)*sizeof *q);
        memcpy(q,name+second+1,prefix*sizeof *q);q[prefix]=':';
        memcpy(q+prefix+1,name+first+1,local*sizeof *q);q[prefix+local+1]=0;
        array->data[offset+2]=rv(xml_full_string(v,q));release(v,q);
    }
}
static void XMLCALL xml_start(void *user,const XML_Char *name,const XML_Char **attributes) {
    XmlParse *p=(XmlParse *)user;VM *v=p->vm;if(v->exception)return;unsigned saved=v->nr;
    xml_position(p);if(v->exception)return;
    Object *names=array_new(v,"[Ljava/lang/String;",3);root(v,names);xml_name(p,names,0,name);
    size_t count=0;while(attributes[count*2])count++;
    if(count>INT_MAX/5)fail(v,"too many XML attributes");
    Object *values=array_new(v,"[Ljava/lang/String;",(int32_t)(count*5));root(v,values);
    for(size_t i=0;i<count;i++) {
        xml_name(p,values,i*5,attributes[i*2]);
        /* Xerces also exposes lexical attribute local names in SAX1 mode. */
        if(!p->namespaces)values->data[i*5+1]=values->data[i*5+2];
        const XML_Char *type=NULL;
        for(XmlDecl *d=p->declarations;d;d=d->next) {
            /* DTD names use lexical qualified names, including with namespaces. */
            Object *element=xml_full_string(v,d->element);root(v,element);
            int match=!strcmp(element->text,obj(names->data[2])->text);v->nr--;
            if(match) {
                Object *attr=xml_full_string(v,d->name);root(v,attr);
                match=!strcmp(attr->text,obj(values->data[i*5+2])->text);v->nr--;
            }
            if(match){type=d->type;break;}
        }
        const XML_Char notation_type[]={'N','O','T','A','T','I','O','N',0};
        int notation=type&&xml_length(type)>=8&&!memcmp(type,notation_type,8*sizeof *type);
        values->data[i*5+3]=rv(!type?string(v,"CDATA"):type[0]=='('?string(v,"NMTOKEN"):notation?string(v,"NOTATION"):xml_full_string(v,type));
        values->data[i*5+4]=rv(xml_full_string(v,attributes[i*2+1]));
    }
    Value args[]={rv(p->reader),rv(names),rv(values)};xml_call(p,"elementStart","([Ljava/lang/String;[Ljava/lang/String;)V",args,3);v->nr=saved;
}
static void XMLCALL xml_end(void *user,const XML_Char *name) {
    XmlParse *p=(XmlParse *)user;VM *v=p->vm;if(v->exception)return;unsigned saved=v->nr;
    xml_position(p);if(v->exception)return;
    Object *names=array_new(v,"[Ljava/lang/String;",3);root(v,names);xml_name(p,names,0,name);
    Value args[]={rv(p->reader),rv(names)};xml_call(p,"elementEnd","([Ljava/lang/String;)V",args,2);v->nr=saved;
}
static void XMLCALL xml_text(void *user,const XML_Char *text,int length) {
    XmlParse *p=(XmlParse *)user;VM *v=p->vm;if(v->exception)return;
    xml_position(p);if(v->exception)return;
    Object *chars=array_new(v,"[C",length);root(v,chars);
    for(int i=0;i<length;i++)chars->data[i]=iv(text[i]);
    Value args[]={rv(p->reader),rv(chars)};xml_call(p,"text","([C)V",args,2);v->nr--;
}
static void xml_strings(XmlParse *p,const char *method_name,const char *desc,const XML_Char **strings,unsigned count) {
    VM *v=p->vm;if(v->exception)return;unsigned saved=v->nr;Value args[5];args[0]=rv(p->reader);
    xml_position(p);if(v->exception)return;
    for(unsigned i=0;i<count;i++){Object *s=xml_full_string(v,strings[i]);root(v,s);args[i+1]=rv(s);}
    xml_call(p,method_name,desc,args,count+1);v->nr=saved;
}
static const XML_Char xml_empty[]={0};
static void XMLCALL xml_prefix_start(void *user,const XML_Char *prefix,const XML_Char *uri) {
    const XML_Char *s[]={prefix?prefix:xml_empty,uri?uri:xml_empty};xml_strings(user,"prefixStart","(Ljava/lang/String;Ljava/lang/String;)V",s,2);
}
static void XMLCALL xml_prefix_end(void *user,const XML_Char *prefix) {
    const XML_Char *s[]={prefix?prefix:xml_empty};xml_strings(user,"prefixEnd","(Ljava/lang/String;)V",s,1);
}
static void XMLCALL xml_instruction(void *user,const XML_Char *target,const XML_Char *data) {
    const XML_Char *s[]={target,data};xml_strings(user,"instruction","(Ljava/lang/String;Ljava/lang/String;)V",s,2);
}
static void XMLCALL xml_skipped(void *user,const XML_Char *name,int parameter) {
    if(parameter)return;const XML_Char *s[]={name};xml_strings(user,"skipped","(Ljava/lang/String;)V",s,1);
}
static void XMLCALL xml_doctype(void *user,const XML_Char *name,const XML_Char *system,const XML_Char *public_id,int internal) {
    (void)name;(void)system;(void)public_id;(void)internal;XmlParse *p=user;
    if(p->disallow_doctype)xml_fatal(p,"DOCTYPE is disabled");
}
static void XMLCALL xml_attribute(void *user,const XML_Char *element,const XML_Char *name,const XML_Char *type,const XML_Char *default_value,int required) {
    (void)default_value;(void)required;XmlParse *p=user;VM *v=p->vm;if(v->exception)return;
    for(XmlDecl *d=p->declarations;d;d=d->next)if(xml_equal(d->element,element)&&xml_equal(d->name,name))return;
    XmlDecl *d=(XmlDecl *)alloc(v,sizeof *d);d->next=p->declarations;p->declarations=d;
    d->element=xml_copy(v,element);d->name=xml_copy(v,name);d->type=xml_copy(v,type);
}
static void XMLCALL xml_entity(void *user,const XML_Char *name,int parameter,const XML_Char *value,int value_length,const XML_Char *base,const XML_Char *system,const XML_Char *public_id,const XML_Char *notation) {
    (void)value;(void)value_length;(void)base;(void)public_id;(void)notation;
    XmlParse *p=user;VM *v=p->vm;if(v->exception||parameter||!system)return;
    XmlEntity *e=(XmlEntity *)alloc(v,sizeof *e);e->next=p->entities;p->entities=e;
    e->name=xml_copy(v,name);e->length=xml_length(name);
}
static int XMLCALL xml_external(XML_Parser parser,const XML_Char *context,const XML_Char *base,const XML_Char *system,const XML_Char *public_id) {
    (void)base;(void)system;(void)public_id;XmlParse *p=XML_GetUserData(parser);VM *v=p->vm;
    if(v->exception)return XML_STATUS_ERROR;if(!context)return XML_STATUS_OK;
    /* Context tokens list open entities and namespace bindings. Because this
     * adapter never enters an external entity, exactly one open external
     * entity is the current reference; internal ancestors are not selected. */
    for(const XML_Char *token=context;*token;) {
        const XML_Char *end=token;while(*end&&*end!='\f')end++;
        for(XmlEntity *e=p->entities;e;e=e->next)if(e->length==(size_t)(end-token)&&!memcmp(e->name,token,e->length*sizeof *token)) {
            const XML_Char *s[]={e->name};xml_strings(p,"skipped","(Ljava/lang/String;)V",s,1);
            return v->exception?XML_STATUS_ERROR:XML_STATUS_OK;
        }
        token=*end?end+1:end;
    }
    xml_fatal(p,"Cannot identify the skipped external entity");return XML_STATUS_ERROR;
}
static void XMLCALL xml_notation(void *user,const XML_Char *name,const XML_Char *base,const XML_Char *system,const XML_Char *public_id) {
    (void)base;const XML_Char *s[]={name,public_id,system};xml_strings(user,"notation","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",s,3);
}
static void XMLCALL xml_unparsed(void *user,const XML_Char *name,const XML_Char *base,const XML_Char *system,const XML_Char *public_id,const XML_Char *notation) {
    (void)base;const XML_Char *s[]={name,public_id,system,notation};xml_strings(user,"unparsed","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",s,4);
}
static Value xml_get(VM *v,Object *source,const char *name,const char *desc) {
    Method *m=method(source->cls,name,desc);if(!m)fail(v,"XML input accessor missing: %s",name);
    Value self=rv(source);return execute(v,m,&self,1);
}
static void xml_parse(VM *v,Object *reader,Object *source,int namespaces,int disallow_doctype) {
    unsigned saved=v->nr;root(v,reader);root(v,source);
    Object *stream=obj(xml_get(v,source,"getCharacterStream","()Ljava/io/Reader;"));root(v,stream);int characters=!!stream;
    if(!v->exception&&!stream){stream=obj(xml_get(v,source,"getByteStream","()Ljava/io/InputStream;"));root(v,stream);}
    if(v->exception){v->nr=saved;return;}
    if(!stream){throwing(v,"java/io/IOException");set_text(v,v->exception,"XML parsing requires an InputSource byte or character stream; URI opening is not implemented");v->nr=saved;return;}
    Object *encoding=characters?NULL:obj(xml_get(v,source,"getEncoding","()Ljava/lang/String;"));root(v,encoding);
    if(v->exception){v->nr=saved;return;}
    XML_Char name[128];const XML_Char *selected=NULL;
    if(characters){const XML_Char utf16[]={ 'U','T','F','-','1','6','L','E',0 };memcpy(name,utf16,sizeof utf16);selected=name;}
    else if(encoding){const unsigned char *s=(const unsigned char *)encoding->text;unsigned n=0;while(*s&&n<127)name[n++]=(XML_Char)utf_unit(&s);if(*s)fail(v,"XML encoding name too long");name[n]=0;selected=name;}
    XmlParse *p=(XmlParse *)alloc(v,sizeof *p);p->vm=v;p->reader=reader;p->namespaces=namespaces;p->disallow_doctype=disallow_doctype;
    p->next=v->xml_parsers;v->xml_parsers=p;xml_allocator_vm=v;
    XML_Memory_Handling_Suite memory={xml_malloc,xml_realloc,xml_free};const XML_Char separator=31;
    p->parser=XML_ParserCreate_MM(selected,&memory,namespaces?&separator:NULL);
    if(!p->parser){xml_destroy(v,p);throwing(v,"java/lang/OutOfMemoryError");v->nr=saved;return;}
    XML_SetUserData(p->parser,p);XML_SetReturnNSTriplet(p->parser,1);
    XML_SetElementHandler(p->parser,xml_start,xml_end);XML_SetCharacterDataHandler(p->parser,xml_text);
    XML_SetNamespaceDeclHandler(p->parser,xml_prefix_start,xml_prefix_end);
    XML_SetProcessingInstructionHandler(p->parser,xml_instruction);XML_SetSkippedEntityHandler(p->parser,xml_skipped);
    XML_SetStartDoctypeDeclHandler(p->parser,xml_doctype);XML_SetAttlistDeclHandler(p->parser,xml_attribute);
    XML_SetEntityDeclHandler(p->parser,xml_entity);XML_SetExternalEntityRefHandler(p->parser,xml_external);
    XML_SetNotationDeclHandler(p->parser,xml_notation);XML_SetUnparsedEntityDeclHandler(p->parser,xml_unparsed);
    XML_SetParamEntityParsing(p->parser,XML_PARAM_ENTITY_PARSING_NEVER);
    /* External references are reported as skipped; no file or URL is opened. */
    Value self=rv(reader);xml_call(p,"documentStart","()V",&self,1);
    unsigned char bytes[4096];size_t total=0;int eof=0;
    while(!eof&&!v->exception) {
        size_t used=0;
        while(used<sizeof bytes-1) {
            int ch=characters?reader_read(v,stream):stream_read(v,stream);
            if(v->exception)break;if(ch<0){eof=1;break;}
            bytes[used++]=(unsigned char)ch;if(characters)bytes[used++]=(unsigned char)(ch>>8);
        }
        if(v->exception)break;total+=used;
        if(total>8U*1024U*1024U){xml_fatal(p,"XML input exceeds the 8 MiB limit");break;}
        xml_allocator_vm=v;
        if(XML_Parse(p->parser,(const char *)bytes,(int)used,eof)==XML_STATUS_ERROR) {
            if(!v->exception) {
                enum XML_Error error=XML_GetErrorCode(p->parser);
                if(error==XML_ERROR_NO_MEMORY)throwing(v,"java/lang/OutOfMemoryError");
                else xml_fatal(p,XML_ErrorString(error));
            }
            break;
        }
    }
    if(!v->exception)xml_call(p,"documentEnd","()V",&self,1);
    /* SAX parsers own supplied streams for the duration of parsing, including
     * failure paths. Preserve the first exception if close also fails. */
    Object *pending=v->exception;root(v,pending);v->exception=NULL;reader_close(v,stream);
    if(pending)v->exception=pending;
    xml_destroy(v,p);v->nr=saved;
}
