/* String.format subset: %s, %%, %n; argument indices, '<', '-', width and
 * string precision. Numeric/date/locale conversions and Formattable callbacks
 * remain unsupported and produce explicit VM diagnostics. */
static int named_type(Class *c,const char *name) {
    if(!c)return 0;if(!strcmp(c->name,name))return 1;
    if(named_type(c->super,name))return 1;
    for(unsigned i=0;i<c->ni;i++)if(named_type(c->interfaces[i],name))return 1;
    return 0;
}
static unsigned format_number(VM *v,const char **cursor) {
    unsigned n=0;const char *p=*cursor;
    while(*p>='0'&&*p<='9') {
        unsigned digit=(unsigned)(*p++-'0');
        if(n>((unsigned)INT_MAX-digit)/10)fail(v,"String.format numeric parameter exceeds supported range");
        n=n*10+digit;
    }
    *cursor=p;return n;
}
static void format_padding(VM *v,ConcatBuffer *buffer,size_t count) {
    static const char spaces[]="                                ";
    if(count>META_LIMIT)fail(v,"String.format width exceeds metadata limit");
    while(count){size_t n=count<sizeof spaces-1?count:sizeof spaces-1;concat_append(v,buffer,spaces,n);count-=n;}
}
static Object *format_string(VM *v,Object *format,Object *args) {
    if(!format){throwing(v,"java/lang/NullPointerException");return NULL;}
    if(format->kind!='s'||(args&&(args->kind!='a'||(args->array_desc[1]!='L'&&args->array_desc[1]!='['))))fail(v,"invalid String.format arguments");
    ConcatBuffer buffer={0};size_t ordinary=0,previous=SIZE_MAX;
    const char *p=format->text;
    while(*p&&!v->exception) {
        if(*p!='%'){const char *start=p;while(*p&&*p!='%')p++;concat_append(v,&buffer,start,(size_t)(p-start));continue;}
        p++;const char *digits=p;unsigned index=format_number(v,&p);int explicit_index=*p=='$';
        if(explicit_index){if(!index)fail(v,"invalid String.format argument index");p++;}else p=digits;
        int left=0,relative=0;
        while(*p=='-'||*p=='<') {
            int *flag=*p++=='-'?&left:&relative;if(*flag)fail(v,"duplicate String.format flag");*flag=1;
        }
        if(*p&&strchr("#+ 0,(",*p))fail(v,"String.format flag is not implemented: %c",*p);
        unsigned width=format_number(v,&p),precision=0;int precise=*p=='.';
        if(precise){p++;if(*p<'0'||*p>'9')fail(v,"missing String.format precision");precision=format_number(v,&p);}
        char conversion=*p;if(!conversion)fail(v,"incomplete String.format specifier");p++;
        if(left&&!width)fail(v,"String.format '-' flag requires a width");
        if(conversion=='n') {
            if(left||relative||width||precise)fail(v,"invalid String.format newline specifier");
            Property *line=property(v,"line.separator",0);const char *text=line&&line->initial?line->initial:"\n";
            concat_append(v,&buffer,text,strlen(text));continue;
        }
        if(conversion=='%') {
            if(relative||precise)fail(v,"invalid String.format percent specifier");
            if(!left&&width>1)format_padding(v,&buffer,width-1);
            concat_append(v,&buffer,"%",1);if(left&&width>1)format_padding(v,&buffer,width-1);continue;
        }
        if(conversion!='s')fail(v,"String.format conversion is not implemented: %c",conversion);
        size_t selected=relative?previous:explicit_index?(size_t)index-1:ordinary++;
        if(selected==SIZE_MAX||(args&&selected>=args->count))fail(v,"missing String.format argument");
        previous=selected;Value value=args?args->data[selected]:rv(NULL);
        if(obj(value)&&named_type(obj(value)->cls,"java/util/Formattable"))fail(v,"String.format Formattable callbacks are not implemented");
        Value converted=object_string(v,value);if(v->exception)break;root(v,obj(converted));
        const char *text=obj(converted)?obj(converted)->text:"null";
        const unsigned char *end=(const unsigned char *)text;size_t units=0;
        while(*end&&(!precise||units<precision)){utf_unit(&end);units++;}
        size_t padding=width>units?width-units:0;
        if(!left)format_padding(v,&buffer,padding);
        concat_append(v,&buffer,text,(size_t)(end-(const unsigned char *)text));
        if(left)format_padding(v,&buffer,padding);v->nr--;
    }
    Object *result=v->exception?NULL:string(v,buffer.data?buffer.data:"");release(v,buffer.data);return result;
}
