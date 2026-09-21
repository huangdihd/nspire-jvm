/* Java linkage errors and platform library naming. This does not supply a JNI
 * ABI or load desktop shared objects into the calculator interpreter. */
static Object *library_name(VM *v,Object *name) {
    if(!name){throwing(v,"java/lang/NullPointerException");return NULL;}
    const unsigned char *p=(const unsigned char *)name->text;unsigned units=0;
    while(*p){utf_unit(&p);if(++units>240){file_error(v,"java/lang/IllegalArgumentException","name too long");return NULL;}}
    /* Adopt the Unix lib<name>.so convention on both Linux and Ndless. The
     * latter has no dynamic JNI loader; mapping is not a claim of loadability. */
    size_t length=strlen(name->text);char *mapped=(char *)alloc(v,length+7);
    memcpy(mapped,"lib",3);memcpy(mapped+3,name->text,length);memcpy(mapped+length+3,".so",4);
    Object *result=string(v,mapped);release(v,mapped);return result;
}
static void library_load(VM *v,Object *name,int by_name) {
    if(!name){throwing(v,"java/lang/NullPointerException");return;}
    const char *message="Dynamic JNI library loading is not available in Nspire JVM";
    if(by_name&&strchr(name->text,'/'))message="Directory separator in library name";
    else if(!by_name&&name->text[0]!='/')message="Expecting an absolute path of the library";
    file_error(v,"java/lang/UnsatisfiedLinkError",message);
}
static void native_unlinked(VM *v,Method *method) {
    size_t size=strlen(method->owner->name)+strlen(method->name)+strlen(method->desc)+40;
    char *message=(char *)alloc(v,size);
    snprintf(message,size,"Native method is not linked: %s.%s%s",method->owner->name,method->name,method->desc);
    file_error(v,"java/lang/UnsatisfiedLinkError",message);release(v,message);
}
