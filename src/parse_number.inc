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
static Value integer_text_helper(VM *v,const char *name,const char *desc,Value *args,int large,int *handled) {
    *handled=0;if(strcmp(name,"stringSize")&&strcmp(name,"getChars"))return iv(0);
    if(strcmp(desc,large?"(J)I":"(I)I")&&strcmp(desc,large?"(JI[C)V":"(II[C)V"))return iv(0);
    *handled=1;char text[32];if(large)snprintf(text,sizeof text,"%lld",(long long)(int64_t)args[0].bits);else snprintf(text,sizeof text,"%ld",(long)integer(args[0]));
    size_t length=strlen(text);if(!strcmp(name,"stringSize"))return iv((int32_t)length);
    Object *out=nonnull(v,args[2]);if(!out)return iv(0);if(out->kind!='a'||strcmp(out->array_desc,"[C"))fail(v,"integer getChars requires char[]");
    int end=integer(args[1]);if(end<0||(size_t)end<length||(size_t)end>out->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return iv(0);}
    for(size_t i=0;i<length;i++)out->data[(size_t)end-length+i]=iv(text[i]);return iv(0);
}
