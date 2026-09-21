import java.lang.annotation.*;
public class UnsupportedAnnotationTextTest {
    @Retention(RetentionPolicy.RUNTIME) @interface Number { double value() default 1.25; }
    @Number static class Example {}
    public static void main(String[] args) {System.out.println(Example.class.getAnnotation(Number.class));}
}
