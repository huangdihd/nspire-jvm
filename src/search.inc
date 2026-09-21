/* Literal substring search over the VM's modified UTF-8 strings. Start/result
 * positions count UTF-16 units, including individual surrogate halves. */
static int32_t string_search(const char *text,const char *needle,int32_t from,int backwards) {
    const unsigned char *p=(const unsigned char *)text;int32_t length=0;
    while(*p){utf_unit(&p);length++;}
    if(from<0){if(backwards)return -1;from=0;}
    if(from>length)from=length;
    if(!*needle)return from;
    p=(const unsigned char *)text;int32_t at=0,result=-1;size_t remaining=strlen(text),size=strlen(needle);
    while(*p) {
        if((backwards?at<=from:at>=from)&&remaining>=size&&!memcmp(p,needle,size)) {
            result=at;if(!backwards)return result;
        }
        if(backwards&&at>=from)break;
        const unsigned char *old=p;utf_unit(&p);remaining-=(size_t)(p-old);at++;
    }
    return result;
}
