/* Primitive wrappers required for reflective argument and result adaptation.
 * Parsing/formatting and the remaining wrapper APIs retain explicit gaps. */
static Value small_box_native(VM *v,Class *c,const char *name,const char *desc,Value *args,int stat,int *handled) {
    *handled=0;const char *cl=c->name;unsigned code;
    if(!strcmp(cl,"java/lang/Byte"))code='B';else if(!strcmp(cl,"java/lang/Short"))code='S';
    else if(!strcmp(cl,"java/lang/Character"))code='C';else if(!strcmp(cl,"java/lang/Float"))code='F';else return iv(0);
    char factory[96],constructor[5]={'(',(char)code,')','V',0};snprintf(factory,sizeof factory,"(%c)L%s;",code,cl);
    if((stat&&!strcmp(name,"valueOf")&&!strcmp(desc,factory))||(!stat&&!strcmp(name,"<init>")&&!strcmp(desc,constructor))) {
        Value value=args[stat?0:1];if(code=='B')value=iv((int8_t)integer(value));else if(code=='S')value=iv((int16_t)integer(value));else if(code=='C')value=iv((uint16_t)integer(value));
        int cache=code=='B'?0:code=='S'?1:2,index=code=='F'?0:integer(value)+(code=='C'?0:128),cached=code!='F'&&index>=0&&index<(code=='C'?128:256);
        *handled=1;if(stat&&cached&&v->small_cache[cache][index])return rv(v->small_cache[cache][index]);
        Object *o=stat?new_object(v,c,'w',1):obj(args[0]);o->kind='w';o->data[0]=value;if(stat&&cached)v->small_cache[cache][index]=o;return stat?rv(o):iv(0);
    }
    if(stat)return iv(0);Object *self=obj(args[0]);Value value=self->data[0];
    if(!strcmp(name,"charValue")&&code=='C'&&!strcmp(desc,"()C")){*handled=1;return value;}
    if(code!='C') {
        const char *names[]={"byteValue","shortValue","intValue","longValue","floatValue","doubleValue"},*descs[]={"()B","()S","()I","()J","()F","()D"};
        for(unsigned i=0;i<6;i++)if(!strcmp(name,names[i])&&!strcmp(desc,descs[i])) {
            *handled=1;double number=code=='F'?(double)flt(value):(double)integer(value);
            if(i==5)return dv(number);if(i==4)return fv((float)number);
            if(i==3){if(isnan(number))return val(0,LONG);if(number>=9223372036854775808.0)return val(INT64_MAX,LONG);if(number<=-9223372036854775808.0)return val((uint64_t)INT64_MIN,LONG);return val((uint64_t)(int64_t)number,LONG);}
            int32_t n=isnan(number)?0:number>=2147483648.0?INT_MAX:number<=-2147483648.0?INT_MIN:(int32_t)number;
            return iv(i==0?(int8_t)n:i==1?(int16_t)n:n);
        }
    }
    if(!strcmp(name,"equals")&&!strcmp(desc,"(Ljava/lang/Object;)Z")) {
        *handled=1;Object *other=obj(args[1]);if(!other||other->cls!=c)return iv(0);
        return iv(code=='F'?annotation_number_bits(value)==annotation_number_bits(other->data[0]):integer(value)==integer(other->data[0]));
    }
    if(!strcmp(name,"hashCode")&&!strcmp(desc,"()I")){*handled=1;return code=='F'?iv((int32_t)annotation_number_bits(value)):value;}
    if(!strcmp(name,"toString")&&!strcmp(desc,"()Ljava/lang/String;")) {
        *handled=1;if(code=='F')fail(v,"Float object text formatting is not implemented");
        char text[128];if(code=='C'){size_t n=write_unit(text,(uint16_t)integer(value));text[n]=0;return rv(string(v,text));}
        return rv(string(v,as_text(v,value,text)));
    }
    return iv(0);
}
