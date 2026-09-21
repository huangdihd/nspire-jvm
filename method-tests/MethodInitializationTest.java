import java.lang.reflect.*;
public class MethodInitializationTest {
    static Object[] arguments={1};static int initialized;
    public static class Target {
        static {initialized++;arguments[0]="changed by initializer";}
        public static int apply(int x){return x;}
    }
    public static void main(String[] args)throws Exception {
        Method m=Target.class.getMethod("apply",int.class);System.out.println(initialized);
        try{m.invoke(null,arguments);}catch(Exception e){System.out.println(e.getClass().getName());}
        System.out.println(initialized);arguments[0]=9;System.out.println(m.invoke(null,arguments));
    }
}
