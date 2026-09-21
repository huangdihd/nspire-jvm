import java.lang.annotation.*;
public class RepeatableTest {
    @Inherited @Retention(RetentionPolicy.RUNTIME) @Repeatable(Tags.class)
    @interface Tag { String value(); }
    @Inherited @Retention(RetentionPolicy.RUNTIME) @interface Tags { Tag[] value(); }
    @Tag("a") @Tag("b") static class Parent {}
    static class Child extends Parent {}
    @Tag("c") static class Override extends Parent {}
    @Tags({}) static class Empty extends Parent {}
    @Tag("first") @Tags({@Tag("second"),@Tag("third")}) static class Mixed {}
    @Tags({@Tag("first"),@Tag("second")}) @Tag("third") static class Reversed {}
    @Tag("iface") interface Tagged {}
    static class Implements implements Tagged {}
    static String values(Tag[] tags){StringBuilder b=new StringBuilder();for(Tag t:tags)b.append(t.value()).append(',');return b.toString();}
    static void check(boolean x){if(!x)throw new AssertionError();}
    public static void main(String[] args) {
        check(Parent.class.getAnnotation(Tag.class)==null);
        check(values(Parent.class.getAnnotationsByType(Tag.class)).equals("a,b,"));
        check(values(Child.class.getAnnotationsByType(Tag.class)).equals("a,b,"));
        check(Child.class.getDeclaredAnnotationsByType(Tag.class).length==0);
        check(values(Override.class.getAnnotationsByType(Tag.class)).equals("c,"));
        check(values(Empty.class.getAnnotationsByType(Tag.class)).equals("a,b,"));
        check(values(Mixed.class.getDeclaredAnnotationsByType(Tag.class)).equals("first,second,third,"));
        check(values(Reversed.class.getDeclaredAnnotationsByType(Tag.class)).equals("first,second,third,"));
        check(Implements.class.getAnnotationsByType(Tag.class).length==0);
        Tag[] tags=Parent.class.getAnnotationsByType(Tag.class);check(tags.getClass()==Tag[].class);tags[0]=null;
        System.gc();check(values(Parent.class.getAnnotationsByType(Tag.class)).equals("a,b,"));
        System.out.println("repeatable annotation ordering, inheritance and array isolation passed");
    }
}
