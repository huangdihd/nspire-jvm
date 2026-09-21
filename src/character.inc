/* UTF-16/code-point APIs and Unicode 13 character properties. Generated tables
 * retain their upstream license; the adapter itself is original MIT code. */
#include "../vendor/openjdk17-casing/character.inc"
static int character_length(VM *v,Object *source) {
    if(source->kind=='a'){if(strcmp(source->array_desc,"[C"))fail(v,"Character requires char[]");return (int)source->count;}
    Value args[]={rv(source)};return integer(sequence_call(v,source,"length","()I",args,1));
}
static unsigned character_unit(VM *v,Object *source,int index) {
    if(source->kind=='a') {
        if(index<0||(size_t)index>=source->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return 0;}
        return (uint16_t)integer(source->data[index]);
    }
    Value args[]={rv(source),iv(index)};return (uint16_t)integer(sequence_call(v,source,"charAt","(I)C",args,2));
}
static int character_high(unsigned ch){return ch>=0xd800&&ch<=0xdbff;}
static int character_low(unsigned ch){return ch>=0xdc00&&ch<=0xdfff;}
static unsigned character_pair(unsigned high,unsigned low){return ((high-0xd800)<<10)+(low-0xdc00)+0x10000;}
static unsigned character_point(VM *v,Object *source,int index,int bound,int before) {
    int at=before?(int32_t)((uint32_t)index-1):index;unsigned first=character_unit(v,source,at);if(v->exception)return 0;
    if(before) {
        if(character_low(first)&&at>bound){unsigned prev=character_unit(v,source,at-1);if(character_high(prev))return character_pair(prev,first);}
    }else if(character_high(first)) {
        if(bound<0)bound=character_length(v,source);
        if(!v->exception&&at<bound-1){unsigned next=character_unit(v,source,at+1);if(character_low(next))return character_pair(first,next);}
    }
    return first;
}
static void character_put(VM *v,Object *array,int index,unsigned ch) {
    if(index<0||(size_t)index>=array->count){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return;}array->data[index]=iv((uint16_t)ch);
}
static Value character_native(VM *v,const char *name,const char *desc,Value *a,int *handled) {
    *handled=1;int32_t c=integer(a[0]);
    if(!strcmp(desc,"(C)Z")) {
        unsigned ch=(uint16_t)c;
        if(!strcmp(name,"isSurrogate"))return iv(ch>=0xd800&&ch<=0xdfff);
        if(!strcmp(name,"isHighSurrogate"))return iv(character_high(ch));
        if(!strcmp(name,"isLowSurrogate"))return iv(character_low(ch));
    }
    if(!strcmp(desc,"(I)Z")) {
        if(!strcmp(name,"isValidCodePoint"))return iv((uint32_t)c<0x110000);
        if(!strcmp(name,"isBmpCodePoint"))return iv((uint32_t)c<=0xffff);
        if(!strcmp(name,"isSupplementaryCodePoint"))return iv(c>=0x10000&&c<=0x10ffff);
    }
    if(!strcmp(name,"isSurrogatePair")&&!strcmp(desc,"(CC)Z"))return iv(character_high((uint16_t)c)&&character_low((uint16_t)integer(a[1])));
    if(!strcmp(name,"toCodePoint")&&!strcmp(desc,"(CC)I"))return iv((int32_t)((((uint32_t)(uint16_t)c-0xd800)<<10)+((uint32_t)(uint16_t)integer(a[1])-0xdc00)+0x10000));
    if(!strcmp(name,"highSurrogate")&&!strcmp(desc,"(I)C"))return iv((uint16_t)(((int64_t)c>>10)+(0xd800-(0x10000>>10))));
    if(!strcmp(name,"lowSurrogate")&&!strcmp(desc,"(I)C"))return iv((uint16_t)((c&1023)+0xdc00));
    if((!strcmp(name,"getType")&&(!strcmp(desc,"(I)I")||!strcmp(desc,"(C)I")))||
       (!strncmp(name,"is",2)&&(!strcmp(desc,"(I)Z")||!strcmp(desc,"(C)Z")))) {
        unsigned flags=(unsigned)case_range((unsigned)c,character_properties,sizeof character_properties/sizeof *character_properties);
        if(!strcmp(name,"getType"))return iv((int)(flags&31));
        const char *names[]={"isAlphabetic","isIdeographic","isLetter","isDigit","isLetterOrDigit","isWhitespace","isSpaceChar","isMirrored","isDefined","isUnicodeIdentifierStart","isUnicodeIdentifierPart","isIdentifierIgnorable","isISOControl"};
        for(unsigned i=0;i<sizeof names/sizeof *names;i++)if(!strcmp(name,names[i]))return iv(!!(flags&(1u<<(i+5))));
    }
    if(!strcmp(name,"digit")&&(!strcmp(desc,"(II)I")||!strcmp(desc,"(CI)I"))) {
        int digit=case_range((unsigned)c,character_digits,sizeof character_digits/sizeof *character_digits)-1,radix=integer(a[1]);return iv(radix>=2&&radix<=36&&digit<radix?digit:-1);
    }
    if((!strcmp(name,"toChars")&&(!strcmp(desc,"(I)[C")||!strcmp(desc,"(I[CI)I")))||(!strcmp(name,"toSurrogates")&&!strcmp(desc,"(I[CI)V"))) {
        if((uint32_t)c>=0x110000){throwing(v,"java/lang/IllegalArgumentException");return iv(0);}
        unsigned units=c>=0x10000?2:1;int fresh=!strcmp(desc,"(I)[C"),index=fresh?0:integer(a[2]);
        Object *out=fresh?array_new(v,"[C",(int32_t)units):nonnull(v,a[1]);if(!out)return iv(0);
        if(out->kind!='a'||strcmp(out->array_desc,"[C"))fail(v,"Character.toChars requires char[]");
        if(units==1)character_put(v,out,index,(unsigned)c);
        else {character_put(v,out,(int32_t)((uint32_t)index+1),0xdc00+((unsigned)c&1023));if(!v->exception)character_put(v,out,index,0xd800+(((unsigned)c-0x10000)>>10));}
        return fresh?rv(out):iv((int32_t)units);
    }
    int at=!strcmp(name,"codePointAt")||!strcmp(name,"codePointAtImpl"),before=!strcmp(name,"codePointBefore")||!strcmp(name,"codePointBeforeImpl");
    int count=!strcmp(name,"codePointCount")||!strcmp(name,"codePointCountImpl"),offset=!strcmp(name,"offsetByCodePoints")||!strcmp(name,"offsetByCodePointsImpl");
    if(at||before||count||offset) {
        Object *source=nonnull(v,a[0]);if(!source)return iv(0);root(v,source);
        int array=desc[1]=='[',len=-1,index=integer(a[1]),bound=before?0:-1;Value result=iv(0);
        if(array||count||offset)len=character_length(v,source);if(v->exception)goto done;
        if(at||before) {
            if(array) {
                bound=before?0:len;
                if(strchr(desc+4,'I')&&nargs(v,desc)==3) {
                    bound=integer(a[2]);
                    if(before?(index<=bound||bound<0||index>len):(index<0||index>=bound||bound<0||bound>len)){throwing(v,"java/lang/IndexOutOfBoundsException");goto done;}
                }
            }
            result=iv((int32_t)character_point(v,source,index,bound,before));goto done;
        }
        int start=array?integer(a[1]):0,end=array?integer(a[2]):len;
        if(array){if(start<0||end<0||start>len-end){throwing(v,"java/lang/IndexOutOfBoundsException");goto done;}end+=start;}
        if(count) {
            if(!array){start=integer(a[1]);end=integer(a[2]);if(start<0||end<start||end>len){throwing(v,"java/lang/IndexOutOfBoundsException");goto done;}}
            int found=0;for(int i=start;i<end&&!v->exception;i++,found++){unsigned ch=character_unit(v,source,i);if(v->exception)break;if(character_high(ch)&&i+1<end){unsigned next=character_unit(v,source,i+1);if(character_low(next))i++;}}
            result=iv(found);goto done;
        }
        index=integer(a[array?3:1]);int steps=integer(a[array?4:2]);
        if(index<start||index>end){throwing(v,"java/lang/IndexOutOfBoundsException");goto done;}
        while(steps&&!v->exception) {
            if(steps>0){if(index==end){throwing(v,"java/lang/IndexOutOfBoundsException");break;}unsigned ch=character_unit(v,source,index++);if(v->exception)break;if(character_high(ch)&&index<end){unsigned next=character_unit(v,source,index);if(character_low(next))index++;}steps--;}
            else {if(index==start){throwing(v,"java/lang/IndexOutOfBoundsException");break;}unsigned ch=character_unit(v,source,--index);if(v->exception)break;if(character_low(ch)&&index>start){unsigned prev=character_unit(v,source,index-1);if(character_high(prev))index--;}steps++;}
        }
        result=iv(index);
done:   v->nr--;return result;
    }
    *handled=0;return iv(0);
}
static void string_get_chars(VM *v,Object *self,int begin,int end,Object *target,int destination) {
    const unsigned char *p=(const unsigned char *)self->text;int length=0;while(*p){utf_unit(&p);length++;}
    if(begin<0||end<begin||end>length){throwing(v,"java/lang/StringIndexOutOfBoundsException");return;}
    if(!target){throwing(v,"java/lang/NullPointerException");return;}
    if(target->kind!='a'||strcmp(target->array_desc,"[C"))fail(v,"String.getChars requires char[]");
    if(destination<0||(size_t)destination>target->count||(size_t)(end-begin)>target->count-(size_t)destination){throwing(v,"java/lang/ArrayIndexOutOfBoundsException");return;}
    p=(const unsigned char *)self->text;for(int i=0;i<end;i++){unsigned ch=utf_unit(&p);if(i>=begin)target->data[destination+i-begin]=iv((int32_t)ch);}
}
