/* ObjectMethods call sites emitted by javac for records. MIT.
 * Field handles are resolved once; Java component callbacks run normally.
 * This is a bootstrap intrinsic, not a general MethodHandle implementation. */
struct RecordShape { Class *type;unsigned count;Field **fields;const char *names; };
static int record_class(Class *c) {
    return c->record.bytes&&(c->access&0x10)&&c->super&&!strcmp(c->super->name,"java/lang/Record");
}
static RecordShape *record_shape(VM *v,Class *owner,Bootstrap *b) {
    if(b->record_shape)return b->record_shape;
    if(b->nargs<2)fail(v,"ObjectMethods requires a class and component names");
    Class *type=load(v,classname(v,owner,b->args[0]));
    if(!record_class(type))fail(v,"ObjectMethods currently requires a final record class");
    RecordShape *shape=(RecordShape *)alloc(v,sizeof *shape);shape->type=type;shape->count=b->nargs-2;
    shape->names=utf(v,owner,cp(v,owner,b->args[1],8)->a);
    shape->fields=(Field **)alloc(v,shape->count*sizeof(Field *));
    for(unsigned i=0;i<shape->count;i++) {
        CP *handle=cp(v,owner,b->args[i+2],15);
        if(handle->a!=1)fail(v,"ObjectMethods accessor handle kind is not implemented: %u",handle->a);
        CP *ref=cp(v,owner,handle->b,9),*nt=cp(v,owner,ref->b,12);
        Class *declared=load(v,classname(v,owner,ref->a));
        if(declared!=type)fail(v,"ObjectMethods field handle must reference the record class");
        Field *f=field(v,&declared,utf(v,owner,nt->a),utf(v,owner,nt->b));
        if(declared!=type||(f->flags&STATIC)||f->slot>=type->slots)fail(v,"invalid ObjectMethods instance field handle");
        shape->fields[i]=f;
    }
    b->record_shape=shape;return shape;
}
static int record_equal(VM *v,Value left,Value right) {
    if(left.tag!=right.tag)fail(v,"record field value tag mismatch");
    if(left.tag!=REF)return annotation_number_bits(left)==annotation_number_bits(right);
    Object *a=obj(left),*b=obj(right);if(a==b)return 1;if(!a||!b)return 0;
    Value args[]={left,right};return integer(annotation_virtual(v,a,"equals","(Ljava/lang/Object;)Z",args,2))!=0;
}
static uint32_t record_hash(VM *v,Value value,const char *desc) {
    if(value.tag!=REF) {
        uint64_t bits=annotation_number_bits(value);
        return *desc=='Z'?(integer(value)?1231:1237):(uint32_t)(bits^(wide(value)?bits>>32:0));
    }
    Object *o=obj(value);if(!o)return 0;
    return (uint32_t)integer(annotation_virtual(v,o,"hashCode","()I",&value,1));
}
static void record_text(VM *v,ConcatBuffer *buffer,Value value,char type) {
    if(value.tag==REF) {
        Value result=object_string(v,value);if(v->exception)return;
        const char *s=obj(result)?obj(result)->text:"null";
        concat_append(v,buffer,s,strlen(s));return;
    }
    if(type=='F'||type=='D') {
        Class *decimal=load(v,"sun/misc/FloatingDecimal");initialize(v,decimal);if(v->exception)return;
        Method *m=method(decimal,"toJavaFormatString",type=='F'?"(F)Ljava/lang/String;":"(D)Ljava/lang/String;");
        if(!m)fail(v,"FloatingDecimal conversion method is missing");
        Value result=execute(v,m,&value,1);if(v->exception)return;
        if(result.tag!=REF||!obj(result)||obj(result)->kind!='s')fail(v,"invalid FloatingDecimal result");
        concat_append(v,buffer,obj(result)->text,strlen(obj(result)->text));return;
    }
    concat_value(v,buffer,value,type);
}
static Value record_dynamic(VM *v,Class *owner,CP *site,const char *desc,Bootstrap *b,Value *args,unsigned count) {
    RecordShape *shape=record_shape(v,owner,b);const char *name=utf(v,owner,cp(v,owner,site->b,12)->a);
    int equals=!strcmp(name,"equals"),hash=!strcmp(name,"hashCode"),text=!strcmp(name,"toString");
    if(!equals&&!hash&&!text)fail(v,"invalid ObjectMethods operation: %s",name);
    char *expected=(char *)alloc(v,strlen(shape->type->name)+64);
    snprintf(expected,strlen(shape->type->name)+64,"(L%s;%s)%s",shape->type->name,equals?"Ljava/lang/Object;":"",equals?"Z":hash?"I":"Ljava/lang/String;");
    if(strcmp(desc,expected)||count!=(equals?2U:1U))fail(v,"invalid ObjectMethods call site descriptor");release(v,expected);
    /* Call-site arguments are still on the interpreter operand stack, including
     * while callbacks allocate, throw or switch cooperative threads. */
    Object *self=obj(args[0]),*other=equals?obj(args[1]):NULL;
    if(equals&&self==other)return iv(1);
    if(self&&self->cls!=shape->type)fail(v,"invalid ObjectMethods receiver class");
    if(equals&&(!other||other->cls!=shape->type))return iv(0);
    if(!self&&shape->count){throwing(v,"java/lang/NullPointerException");return iv(0);}
    if(equals) {
        /* OpenJDK 17 builds nested guards, comparing the last component first. */
        for(unsigned i=shape->count;i>0&&!v->exception;i--) {
            Field *f=shape->fields[i-1];if(!record_equal(v,self->data[f->slot],other->data[f->slot]))return iv(0);
        }
        return iv(!v->exception);
    }
    if(hash) {
        uint32_t result=0;
        for(unsigned i=0;i<shape->count&&!v->exception;i++) {
            Field *f=shape->fields[i];result=31*result+record_hash(v,self->data[f->slot],f->desc);
        }
        return iv((int32_t)result);
    }
    const char *names=shape->names;unsigned fields=!*names?0:1;
    for(const char *p=names;*p;p++)if(*p==';')fields++;
    if(fields!=shape->count)fail(v,"ObjectMethods component name count mismatch");
    ConcatBuffer out={0};Object *simple=class_name_string(v,shape->type,1);
    concat_append(v,&out,simple->text,strlen(simple->text));concat_append(v,&out,"[",1);
    for(unsigned i=0;i<shape->count&&!v->exception;i++) {
        const char *end=strchr(names,';');size_t length=end?(size_t)(end-names):strlen(names);
        if(i)concat_append(v,&out,", ",2);concat_append(v,&out,names,length);concat_append(v,&out,"=",1);
        Field *f=shape->fields[i];record_text(v,&out,self->data[f->slot],*f->desc);names=end?end+1:names+length;
    }
    concat_append(v,&out,"]",1);Object *result=v->exception?NULL:string(v,out.data);release(v,out.data);return rv(result);
}
