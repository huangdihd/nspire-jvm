import java.lang.annotation.*;
import java.lang.reflect.*;
public class MethodAnnotationTest {
    @Retention(RetentionPolicy.RUNTIME) public @interface Tag {String value();}
    public static class Bean {
        @Tag("method") public void action(){}
    }
    public static void main(String[] args)throws Exception {
        Method m=Bean.class.getMethod("action");
        System.out.println(m.getAnnotation(Tag.class).value());
        System.out.println(m.getDeclaredAnnotations().length);System.out.println(m.isAnnotationPresent(Tag.class));
        Tag tag=m.getAnnotation(Tag.class);System.gc();System.out.println(Tag.class.getMethod("value").invoke(tag));
        System.out.println(m.getAnnotationsByType(Tag.class).length);
    }
}
