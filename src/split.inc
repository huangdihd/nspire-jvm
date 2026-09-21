/* String.split's literal-character fast path. Other regular expressions are
 * rejected until a Java-compatible regex implementation is available. */
static Object *split_slice(VM *v,Object *source,const unsigned char *start,const unsigned char *end) {
    if(start==(const unsigned char *)source->text&&!*end)return source;
    size_t n=(size_t)(end-start);char *copy=(char *)alloc(v,n+1);memcpy(copy,start,n);
    Object *s=string(v,copy);release(v,copy);return s;
}
static Object *split_string(VM *v,Object *source,Object *pattern,int32_t limit) {
    if(!pattern){throwing(v,"java/lang/NullPointerException");return NULL;}
    const unsigned char *regex=(const unsigned char *)pattern->text;unsigned delimiter=0;
    if(*regex) {
        delimiter=utf_unit(&regex);
        if(delimiter=='\\'&&*regex) {
            delimiter=utf_unit(&regex);
            if((delimiter>='a'&&delimiter<='z')||(delimiter>='A'&&delimiter<='Z')||(delimiter>='0'&&delimiter<='9'))delimiter=0x110000;
        } else if(delimiter&&delimiter<128&&strchr(".$|()[{^?*+\\",(int)delimiter))delimiter=0x110000;
    } else delimiter=0x110000;
    if(*regex||delimiter>=0x110000||(delimiter>=0xd800&&delimiter<=0xdfff))fail(v,"String.split regular expression is not implemented");
    const unsigned char *text=(const unsigned char *)source->text,*start=text,*cursor=text;
    size_t pieces=0,kept=0;int matched=0;
    while(*cursor&&(limit<=0||pieces<(size_t)limit-1)) {
        const unsigned char *before=cursor;
        if(utf_unit(&cursor)==delimiter){matched=1;pieces++;if(before!=start)kept=pieces;start=cursor;}
    }
    const unsigned char *end=cursor+strlen((const char *)cursor);pieces++;if(start!=end)kept=pieces;
    if(!matched)pieces=1;else if(!limit)pieces=kept;
    if(pieces>INT_MAX)fail(v,"too many String.split results");
    Object *result=array_new(v,"[Ljava/lang/String;",(int32_t)pieces);root(v,result);
    start=text;
    for(size_t i=0;i<pieces;i++) {
        cursor=start;const unsigned char *stop=end,*next=end;
        if(limit<=0||i<(size_t)limit-1)while(*cursor){const unsigned char *before=cursor;if(utf_unit(&cursor)==delimiter){stop=before;next=cursor;break;}}
        result->data[i]=rv(split_slice(v,source,start,stop));start=next;
    }
    v->nr--;return result;
}
