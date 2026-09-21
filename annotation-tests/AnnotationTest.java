import java.lang.annotation.*;
import java.lang.reflect.*;
import java.util.*;

public class AnnotationTest {
    static int initialized;
    enum Phase { FIRST, SECOND }
    @Retention(RetentionPolicy.RUNTIME) @interface Nested { String value() default "nested"; }
    @Retention(RetentionPolicy.RUNTIME) @Inherited
    @interface Config {
        int count() default 7;
        String name() default "x\u0000\u4e2d\ud83d\ude00";
        Phase phase() default Phase.FIRST;
        Class<?> type() default int[][].class;
        Nested nested() default @Nested;
        int[] numbers() default {2,3,5};
        Class<?>[] types() default {void.class, int.class, String.class, String[][].class};
        Nested[] children() default {@Nested("a"),@Nested("b")};
    }
    @Config @Deprecated static class Base { static { initialized++; } }
    static class Child extends Base { static { initialized++; } }
    @Config(count=11, phase=Phase.SECOND, numbers={9}) static class Override extends Base {}
    @Config interface TaggedInterface {}
    static class Implements implements TaggedInterface {}
    @Retention(RetentionPolicy.CLASS) @interface Hidden { }
    @Hidden static class Invisible { }
    @Config static class Same { }
    @Retention(RetentionPolicy.RUNTIME) @interface Tiny { int value(); }
    @Tiny(1) static class TinyOne {}
    static class TinyManual implements Tiny {
        public int value(){System.gc();return 1;}
        public Class<? extends Annotation> annotationType(){return Tiny.class;}
        public int hashCode(){return (127*"value".hashCode())^1;}
        public boolean equals(Object o){return o instanceof Tiny && ((Tiny)o).value()==1;}
    }
    static class Members {
        @Config(count=3) public int field;
        @Config(count=4) public Members() { }
    }
    static void check(boolean yes){if(!yes)throw new AssertionError();}
    public static void main(String[] args) throws Exception {
        Config a=Child.class.getAnnotation(Config.class);
        check(initialized==0);
        check(a!=null&&a.count()==7&&a.phase()==Phase.FIRST&&a.type()==int[][].class);
        check(a.name().equals("x\u0000\u4e2d\ud83d\ude00")&&a.nested().value().equals("nested"));
        check(a.types()[0]==void.class&&a.types()[1]==int.class&&a.types()[3]==String[][].class);
        check(a.annotationType()==Config.class&&a instanceof Annotation&&a instanceof java.io.Serializable);
        check(Config.class.isAnnotation()&&Config.class.isInterface()&&!Annotation.class.isAnnotation());
        check(Config.class.getAnnotation(Retention.class).value()==RetentionPolicy.RUNTIME);
        check(Child.class.getDeclaredAnnotation(Config.class)==null);
        check(Child.class.isAnnotationPresent(Config.class)&&!Child.class.isAnnotationPresent(Deprecated.class));
        check(!Implements.class.isAnnotationPresent(Config.class)&&Invisible.class.getAnnotations().length==0);
        check(Base.class.getDeclaredAnnotations().length==2&&Child.class.getAnnotations().length==1);
        check(Child.class.getDeclaredAnnotations().length==0&&String[].class.getAnnotations().length==0&&int.class.getAnnotations().length==0);
        Config b=Override.class.getAnnotation(Config.class);check(b.count()==11&&b.phase()==Phase.SECOND&&b.numbers()[0]==9);
        int[] ints=a.numbers();ints[0]=999;
        Class<?>[] types=a.types();types[0]=Object.class;
        Nested[] nested=a.children();nested[0]=null;
        Annotation[] annotations=Child.class.getAnnotations();annotations[0]=null;
        System.gc();
        check(a.numbers()[0]==2&&a.types()[0]==void.class&&a.children()[0].value().equals("a"));
        check(Child.class.getAnnotations()[0]!=null&&a.equals(Same.class.getAnnotation(Config.class)));
        check(a.hashCode()==Same.class.getAnnotation(Config.class).hashCode()&&!a.equals(b)&&!a.equals(null)&&!a.equals("x"));
        check(a.toString().contains("Config")&&a.toString().contains("phase=")&&a.toString().contains("count=7"));
        Tiny tiny=TinyOne.class.getAnnotation(Tiny.class);check(tiny.equals(new TinyManual())&&new TinyManual().equals(tiny)&&tiny.hashCode()==new TinyManual().hashCode());
        AnnotatedElement element=Child.class;check(element.getAnnotation(Config.class).count()==7);
        Field field=Members.class.getDeclaredField("field");check(field.getAnnotation(Config.class).count()==3&&field.getDeclaredAnnotations().length==1);
        Constructor<Members> ctor=Members.class.getDeclaredConstructor();check(ctor.getAnnotation(Config.class).count()==4);
        try {Base.class.getAnnotation(null);throw new AssertionError();}catch(NullPointerException expected) { }
        try {Base.class.getDeclaredAnnotationsByType(null);throw new AssertionError();}catch(NullPointerException expected) { }
        for(int i=0;i<60;i++){System.gc();check(a.nested().value().equals("nested")&&a.equals(Same.class.getAnnotation(Config.class)));}
        System.out.println("annotation values, retention, inheritance, members, equality and GC passed");
    }
}
