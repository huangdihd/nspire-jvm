/* String regex entry points execute the original OpenJDK Pattern/Matcher. */
static Value regex_string(VM *v,Object *source,Object *pattern,Object *replacement,int limit,int operation) {
    Class *type=load(v,"java/util/regex/Pattern");initialize(v,type);if(v->exception)return rv(NULL);
    Method *compile=method(type,"compile","(Ljava/lang/String;)Ljava/util/regex/Pattern;");if(!compile)fail(v,"Pattern.compile is missing");
    Value arg=rv(pattern),compiled=execute(v,compile,&arg,1);if(v->exception)return rv(NULL);root(v,obj(compiled));
    Value result=rv(NULL);
    if(!operation) {
        Value args[]={compiled,rv(source),iv(limit)};Method *m=method(type,"split","(Ljava/lang/CharSequence;I)[Ljava/lang/String;");
        if(!m)fail(v,"Pattern.split is missing");result=execute(v,m,args,3);
    }else {
        Value args[]={compiled,rv(source)};Method *m=method(type,"matcher","(Ljava/lang/CharSequence;)Ljava/util/regex/Matcher;");
        if(!m)fail(v,"Pattern.matcher is missing");Value matcher=execute(v,m,args,2);
        if(!v->exception){root(v,obj(matcher));Value call[]={matcher,rv(replacement)};
            m=method(obj(matcher)->cls,operation==1?"matches":operation==2?"replaceAll":"replaceFirst",operation==1?"()Z":"(Ljava/lang/String;)Ljava/lang/String;");
            if(!m)fail(v,"Matcher operation is missing");result=execute(v,m,call,operation==1?1:2);v->nr--;}
    }
    v->nr--;return result;
}
