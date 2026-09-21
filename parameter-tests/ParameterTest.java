import java.lang.annotation.*;
import java.lang.reflect.*;
import java.util.*;
public class ParameterTest {
    @Retention(RetentionPolicy.RUNTIME) @Target(ElementType.PARAMETER)
    public @interface Mark { int value(); }
    public static class Fixture {
        public Fixture(@Mark(1) final int value, String name) {}
        public static void mixed(@Mark(7) final int value, long wide, double other, String... rest) {}
        public void empty() {}
    }
    public class Inner { public Inner(final String name) {} }
    enum Choice { A(2); Choice(final int number) {} }
    static void check(boolean value) { if(!value)throw new AssertionError(); }
    static void print(Executable method, boolean annotations, boolean text) {
        Parameter[] parameters = method.getParameters();
        Parameter[] again = method.getParameters();
        check(parameters != again);
        System.out.println(method.getName()+":"+parameters.length);
        for(int i=0;i<parameters.length;i++) {
            Parameter p=parameters[i];
            check(p==again[i]);
            check(p.getDeclaringExecutable()==method);
            check(p.getType()==method.getParameterTypes()[i]);
            check(p.equals(p) && !p.equals(null) && !p.equals("x"));
            check(p.hashCode()==(method.hashCode()^i));
            System.out.println(p.getName()+":"+p.isNamePresent()+":"+p.getModifiers()+":"+p.isImplicit()+":"+p.isSynthetic()+":"+p.isVarArgs()+":"+p.getType().getName());
            if(text)System.out.println(p.toString());
            if(annotations) {
                Annotation[] first=p.getAnnotations(),second=p.getDeclaredAnnotations();
                check(first.length==second.length);System.out.println(first.length);
                Mark mark=p.getAnnotation(Mark.class);
                check(p.isAnnotationPresent(Mark.class)==(mark!=null));
                check(Objects.equals(mark,p.getDeclaredAnnotation(Mark.class)));
                if(mark!=null)System.out.println(mark.value());
                if(first.length!=0){first[0]=null;check(p.getDeclaredAnnotations()[0]!=null);}
                try{p.getAnnotation(null);throw new AssertionError();}catch(NullPointerException expected){}
            }
        }
        if(parameters.length!=0){parameters[0]=null;check(method.getParameters()[0]!=null);}
        System.gc();
        check(method.getParameters().length==again.length);
    }
    public static void main(String[] args) throws Exception {
        Method mixed=Fixture.class.getMethod("mixed",int.class,long.class,double.class,String[].class);
        print(mixed,true,true);
        Parameter[] left=mixed.getParameters();
        Parameter[] right=Fixture.class.getMethod("mixed",int.class,long.class,double.class,String[].class).getParameters();
        for(int i=0;i<left.length;i++){check(left[i].equals(right[i]));check(left[i].hashCode()==right[i].hashCode());}
        check(!left[0].equals(left[1]));
        print(Fixture.class.getConstructor(int.class,String.class),true,true);
        print(Fixture.class.getMethod("empty"),true,true);
        print(Inner.class.getConstructor(ParameterTest.class,String.class),false,false);
        print(Choice.class.getDeclaredConstructor(String.class,int.class,int.class),false,false);
        print(Object.class.getConstructor(),false,false);
        print(String.class.getMethod("substring",int.class,int.class),false,false);
        for(int i=0;i<80;i++) {
            Method m=Fixture.class.getMethod("mixed",int.class,long.class,double.class,String[].class);
            Parameter p=m.getParameters()[3];m=null;
            System.gc();check(p.getType()==String[].class && p.isVarArgs());
            check(p.getDeclaringExecutable().getParameters()[3]==p);
        }
        System.out.println("cache roots retained");
    }
}
