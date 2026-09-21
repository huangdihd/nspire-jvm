import java.lang.reflect.*;
import java.util.*;
public class MethodDiscoveryTest {
    public interface A { String shared(); default int value(){return 1;} static int own(){return 2;} }
    public interface B { String shared(); }
    public interface C extends A { default int value(){return 3;} }
    public abstract static class Pair implements A,B {}
    public static class Base<T> {public T generic(){return null;} public static int hidden(){return 1;} protected void secret(){} private void onlyBase(){} }
    public static class Child extends Base<String> implements C {
        public String generic(){return "child";} public String shared(){return "shared";} public static int hidden(){return 2;}
        public void varargs(String...args){} public synchronized final long operation(int[] values, String name)throws java.io.IOException{return 5;}
        private void privateMethod(){}
    }
    static String key(Method m) {
        String s=m.getDeclaringClass().getName()+"#"+m.getName()+":"+m.getReturnType().getName()+":"+m.getModifiers()+":"+m.isBridge()+":"+m.isSynthetic()+":"+m.isVarArgs()+":"+m.isDefault();
        for(Class<?> p:m.getParameterTypes())s+="/"+p.getName();
        for(Class<?> p:m.getExceptionTypes())s+="!"+p.getName();
        return s;
    }
    static void list(Class<?> type,boolean declared) {
        Method[] methods=declared?type.getDeclaredMethods():type.getMethods();String[] keys=new String[methods.length];
        for(int i=0;i<methods.length;i++)keys[i]=key(methods[i]);Arrays.sort(keys);for(String key:keys)System.out.println(key);
    }
    interface Action {void run()throws Exception;}
    static void check(Action a){try{a.run();System.out.println("ok");}catch(Exception e){System.out.println(e.getClass().getName());}}
    public static void main(String[] args)throws Exception {
        list(Child.class,false);list(Child.class,true);list(Pair.class,false);list(C.class,false);list(A.class,true);
        System.out.println(int.class.getMethods().length);System.out.println(void.class.getDeclaredMethods().length);
        list(String[].class,false);System.out.println(String[].class.getDeclaredMethods().length);
        Method method=Child.class.getMethod("generic");System.out.println(method.getReturnType().getName());System.out.println(method.isBridge());
        Method copy=Child.class.getMethod("generic",(Class<?>[])null);System.out.println(method!=copy);System.out.println(method.equals(copy));System.out.println(method.hashCode()==copy.hashCode());
        method.setAccessible(true);System.out.println(copy.isAccessible());
        method.getParameterTypes();Method params=Child.class.getMethod("operation",int[].class,String.class);
        Class<?>[] types=params.getParameterTypes();types[0]=null;System.out.println(params.getParameterTypes()[0].getName());
        System.out.println(params.getParameterCount());Class<?>[] exceptions=params.getExceptionTypes();exceptions[0]=null;System.out.println(params.getExceptionTypes()[0].getName());
        check(()->Child.class.getMethod("secret"));check(()->Child.class.getDeclaredMethod("shared"));
        check(()->Child.class.getDeclaredMethod("onlyBase"));check(()->Child.class.getMethod("<init>"));
        check(()->Child.class.getMethod(null));check(()->Child.class.getMethod("operation",new Class<?>[]{null,String.class}));
        check(()->C.class.getMethod("own"));
    }
}
