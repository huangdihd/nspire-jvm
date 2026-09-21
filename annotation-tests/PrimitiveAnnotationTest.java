import java.lang.annotation.*;
public class PrimitiveAnnotationTest {
    @Retention(RetentionPolicy.RUNTIME) @interface Values {
        byte b() default -128; short s() default -32768; char c() default '\uffff'; boolean z() default true;
        long l() default Long.MIN_VALUE; float f() default -0.0f; double d() default 0.0/0.0;
        byte[] bs() default {-128,127}; short[] ss() default {-32768,32767}; char[] cs() default {'\u0000','\uffff'};
        boolean[] zs() default {true,false}; long[] ls() default {Long.MIN_VALUE,Long.MAX_VALUE};
        float[] fs() default {0.0f/0.0f,1.0f/0.0f,-0.0f}; double[] ds() default {-1.0/0.0,0.0,-0.0};
    }
    @Values static class A {}
    @Values static class B {}
    @Values(f=0.0f) static class Different {}
    static void check(boolean x){if(!x)throw new AssertionError();}
    public static void main(String[] args) {
        Values a=A.class.getAnnotation(Values.class),b=B.class.getAnnotation(Values.class);
        check(a.b()==-128&&a.s()==-32768&&a.c()==65535&&a.z()&&a.l()==Long.MIN_VALUE&&Double.isNaN(a.d()));
        check(Float.floatToIntBits(a.f())==0x80000000&&a.bs()[1]==127&&a.ss()[1]==32767&&a.cs()[0]==0);
        check(!a.zs()[1]&&a.ls()[1]==Long.MAX_VALUE&&Float.floatToIntBits(a.fs()[1])==0x7f800000&&Double.isInfinite(a.ds()[0]));
        a.bs()[0]=0;a.ss()[0]=0;a.cs()[0]='x';a.zs()[0]=false;a.ls()[0]=0;a.fs()[0]=0;a.ds()[0]=0;System.gc();
        check(a.bs()[0]==-128&&a.ss()[0]==-32768&&a.cs()[0]==0&&a.zs()[0]&&a.ls()[0]==Long.MIN_VALUE&&Float.floatToIntBits(a.fs()[0])==0x7fc00000);
        check(a.equals(b)&&a.hashCode()==b.hashCode()&&!a.equals(Different.class.getAnnotation(Values.class)));
        System.out.println(a.hashCode());
        System.out.println("all primitive annotation types and arrays passed");
    }
}
