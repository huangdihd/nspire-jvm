/* Java floating-point lexical grammar followed by libc binary conversion.
 * C starts in the C locale; Java Locale settings never change it. Ndless/newlib
 * rounding still needs target execution verification. */
static int number_digit(unsigned c,int hex){return (c>='0'&&c<='9')||(hex&&((c>='a'&&c<='f')||(c>='A'&&c<='F')));}
static Value parse_floating(VM *v,Object *input,int single) {
    if(!input){throwing(v,"java/lang/NullPointerException");return iv(0);}
    char *text=(char *)alloc(v,strlen(input->text)+1);const unsigned char *q=(const unsigned char *)input->text;size_t size=0;
    while(*q){unsigned ch=utf_unit(&q);text[size++]=ch<=127?(char)ch:(char)0x80;}
    char *start=text,*end=text+size;while(start<end&&(unsigned char)*start<=32)start++;while(end>start&&(unsigned char)end[-1]<=32)end--;
    char *p=start;int negative=0;if(p<end&&(*p=='-'||*p=='+'))negative=*p++=='-';
    Value result=single?fv(0):dv(0);
    if(end-p==3&&!memcmp(p,"NaN",3)){result=single?fv(NAN):dv(NAN);goto done;}
    if(end-p==8&&!memcmp(p,"Infinity",8)){result=single?fv(negative?-INFINITY:INFINITY):dv(negative?-INFINITY:INFINITY);goto done;}
    int hex=end-p>=2&&p[0]=='0'&&(p[1]=='x'||p[1]=='X');if(hex)p+=2;
    size_t digits=0;while(p<end&&number_digit((unsigned char)*p,hex)){p++;digits++;}
    if(p<end&&*p=='.'){p++;while(p<end&&number_digit((unsigned char)*p,hex)){p++;digits++;}}
    if(!digits)goto invalid;
    if(p<end&&(hex?(*p=='p'||*p=='P'):(*p=='e'||*p=='E'))) {
        p++;if(p<end&&(*p=='-'||*p=='+'))p++;char *exponent=p;while(p<end&&number_digit((unsigned char)*p,0))p++;if(p==exponent)goto invalid;
    }else if(hex)goto invalid;
    char *finish=p;if(p<end&&*p&&strchr("fFdD",*p))p++;if(p!=end)goto invalid;
    *finish=0;result=single?fv(strtof(start,NULL)):dv(strtod(start,NULL));goto done;
invalid:
    throwing(v,"java/lang/NumberFormatException");set_text(v,v->exception,"Invalid Java floating-point string");
done:release(v,text);return result;
}
static Value parse_integral(VM *v,Object *input,int radix,int large) {
    const char *message=NULL;char detail[100];
    if(!input)message="Cannot parse null string";
    else if(radix<2){snprintf(detail,sizeof detail,"radix %d less than Character.MIN_RADIX",radix);message=detail;}
    else if(radix>36){snprintf(detail,sizeof detail,"radix %d greater than Character.MAX_RADIX",radix);message=detail;}
    if(message){throwing(v,"java/lang/NumberFormatException");set_text(v,v->exception,message);return iv(0);}
    const unsigned char *p=(const unsigned char *)input->text;int negative=0;
    if(*p=='+'||*p=='-')negative=*p++=='-';
    uint64_t limit=large?UINT64_C(0x7fffffffffffffff):UINT64_C(0x7fffffff);limit+=(unsigned)negative;
    uint64_t result=0;int valid=*p!=0;
    while(*p&&valid) {
        unsigned ch=utf_unit(&p);
        int digit=case_range(ch,character_digits,sizeof character_digits/sizeof *character_digits)-1;
        if(digit<0||digit>=radix||result>(limit-(unsigned)digit)/(unsigned)radix)valid=0;
        else result=result*(unsigned)radix+(unsigned)digit;
    }
    if(!valid) {
        size_t capacity=strlen(input->text)+80;char *text=(char *)alloc(v,capacity);
        if(radix==10)snprintf(text,capacity,"For input string: \"%s\"",input->text);
        else snprintf(text,capacity,"For input string: \"%s\" under radix %d",input->text,radix);
        throwing(v,"java/lang/NumberFormatException");set_text(v,v->exception,text);release(v,text);return iv(0);
    }
    if(negative)result=UINT64_C(0)-result;
    return large?val(result,LONG):iv((int32_t)(uint32_t)result);
}
static Value integer_text_helper(VM *v,const char *name,const char *desc,Value *args,int large,int *handled) {
    *handled=0;
    if(!strcmp(name,"toString")&&!strcmp(desc,large?"(JI)Ljava/lang/String;":"(II)Ljava/lang/String;")) {
        *handled=1;int radix=integer(args[1]);if(radix<2||radix>36)radix=10;
        int64_t signed_value=large?(int64_t)args[0].bits:(int64_t)integer(args[0]);
        uint64_t value=signed_value<0?UINT64_C(0)-(uint64_t)signed_value:(uint64_t)signed_value;
        char text[66];unsigned end=65;text[end]=0;
        do{text[--end]="0123456789abcdefghijklmnopqrstuvwxyz"[value%(unsigned)radix];value/=(unsigned)radix;}while(value);
        if(signed_value<0)text[--end]='-';return rv(string(v,text+end));
    }
    if(!strcmp(name,large?"parseLong":"parseInt")&&
       (!strcmp(desc,large?"(Ljava/lang/String;)J":"(Ljava/lang/String;)I")||
        !strcmp(desc,large?"(Ljava/lang/String;I)J":"(Ljava/lang/String;I)I"))) {
        *handled=1;return parse_integral(v,obj(args[0]),strstr(desc,";I)")?integer(args[1]):10,large);
    }
    unsigned shift=!strcmp(name,"toHexString")?4:!strcmp(name,"toOctalString")?3:!strcmp(name,"toBinaryString")?1:0;
    if(shift&&!strcmp(desc,large?"(J)Ljava/lang/String;":"(I)Ljava/lang/String;")) {
        *handled=1;uint64_t value=large?args[0].bits:(uint32_t)args[0].bits;
        char text[65];unsigned pos=64,mask=(1U<<shift)-1;text[pos]=0;
        do{text[--pos]="0123456789abcdef"[value&mask];value>>=shift;}while(value);
        return rv(string(v,text+pos));
    }
    if(strcmp(name,"stringSize")&&strcmp(name,"getChars"))return iv(0);
    if(strcmp(desc,large?"(J)I":"(I)I")&&strcmp(desc,large?"(JI[C)V":"(II[C)V"))return iv(0);
    *handled=1;char text[32];if(large)snprintf(text,sizeof text,"%lld",(long long)(int64_t)args[0].bits);else snprintf(text,sizeof text,"%ld",(long)integer(args[0]));
    size_t length=strlen(text);if(!strcmp(name,"stringSize"))return iv((int32_t)length);
    Object *out=nonnull(v,args[2]);if(!out)return iv(0);if(out->kind!='a'||strcmp(out->array_desc,"[C"))fail(v,"integer getChars requires char[]");
    int end=integer(args[1]);if(end<0||(size_t)end<length||(size_t)end>out->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return iv(0);}
    for(size_t i=0;i<length;i++)out->data[(size_t)end-length+i]=iv(text[i]);return iv(0);
}
