import java.lang.reflect.*;
public class MalformedParameterTest {
    public static void main(String[] args) throws Exception {
        Method method=ParameterTest.Fixture.class.getMethod("mixed",int.class,long.class,double.class,String[].class);
        try {
            if(args.length!=0) {
                System.out.println(method.getParameterAnnotations().length);
            } else {
                for(Parameter p:method.getParameters())
                    System.out.println(p.getName()+":"+p.isNamePresent()+":"+p.getModifiers()+":"+p.isSynthetic()+":"+p.isImplicit());
            }
        } catch(Throwable error) { System.out.println(error.getClass().getName()); }
    }
}
