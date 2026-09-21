import java.io.Serializable;
import java.lang.invoke.SerializedLambda;
import java.lang.reflect.*;
import java.util.function.*;
public class SerializableLambdaTest {
    interface Flag {}
    interface Fn extends Serializable { long apply(int x); }
    static class Box { long n;Box(long n){this.n=n;}long add(int x){return n+x;} }
    static long twice(int x){return x*2L;}
    static Fn capture(long wide,double fraction,int[] state,String text) {
        return (Fn & Flag) x -> wide+(long)fraction+state[0]+text.length()+x;
    }
    static SerializedLambda replacement(Object fn)throws Exception {
        Method method=fn.getClass().getDeclaredMethod("writeReplace");method.setAccessible(true);
        return (SerializedLambda)method.invoke(fn);
    }
    static Object restore(SerializedLambda value)throws Exception {
        Method method=SerializedLambda.class.getDeclaredMethod("readResolve");method.setAccessible(true);
        return method.invoke(value);
    }
    static void inspect(Object fn)throws Exception {
        SerializedLambda value=replacement(fn);
        System.out.println(fn instanceof Serializable);
        System.out.println(value.getCapturingClass()+"|"+value.getFunctionalInterfaceClass()+"|"+value.getFunctionalInterfaceMethodName());
        System.out.println(value.getFunctionalInterfaceMethodSignature()+"|"+value.getImplClass()+"|"+value.getImplMethodName());
        System.out.println(value.getImplMethodKind()+"|"+value.getImplMethodSignature()+"|"+value.getInstantiatedMethodType());
        System.out.println(value.getCapturedArgCount());
        for(int i=0;i<value.getCapturedArgCount();i++)System.out.println(value.getCapturedArg(i)==null?"null":value.getCapturedArg(i).getClass().getName());
    }
    public static void main(String[] args)throws Exception {
        int[] state={3};Fn fn=capture(9000000000L,2.25,state,"hello");inspect(fn);
        System.out.println(fn instanceof Flag);System.out.println(fn.apply(5));
        SerializedLambda saved=replacement(fn);Fn restored=(Fn)restore(saved);state[0]=19;System.gc();
        System.out.println(restored.apply(7));System.out.println(restored instanceof Flag);
        for(int i=0;i<600;i++){Fn other=capture(i,0.5,new int[]{i},"x");Fn copy=(Fn)restore(replacement(other));if(copy.apply(1)!=2*i+2)throw new AssertionError();}
        Fn method=SerializableLambdaTest::twice;inspect(method);System.out.println(((Fn)restore(replacement(method))).apply(21));
        Box box=new Box(30);Fn bound=box::add;inspect(bound);Fn copy=(Fn)restore(replacement(bound));box.n=40;System.out.println(copy.apply(2));
        Function<Box,Long> unbound=(Function<Box,Long>&Serializable) b->b.n;inspect(unbound);
        Function<Box,Long> decoded=(Function<Box,Long>)restore(replacement(unbound));System.out.println(decoded.apply(box));
        IntFunction<Box> constructor=(IntFunction<Box>&Serializable)Box::new;inspect(constructor);
        System.out.println(((IntFunction<Box>)restore(replacement(constructor))).apply(17).n);
        Fn empty=(Fn) x->x;SerializedLambda data=replacement(empty);
        SerializedLambda corrupt=new SerializedLambda(SerializableLambdaTest.class,data.getFunctionalInterfaceClass(),"wrong",data.getFunctionalInterfaceMethodSignature(),data.getImplMethodKind(),data.getImplClass(),data.getImplMethodName(),data.getImplMethodSignature(),data.getInstantiatedMethodType(),new Object[0]);
        try{restore(corrupt);throw new AssertionError();}catch(InvocationTargetException e){
            // JDK 17 adds an InvalidObjectException wrapper to readResolve;
            // compare the actual javac validation failure at the root.
            Throwable cause=e;while(cause.getCause()!=null)cause=cause.getCause();
            if(!(cause instanceof IllegalArgumentException))throw new AssertionError();
            System.out.println("invalid lambda rejected: "+cause.getClass().getName());
        }
        try{((Runnable)()->{}).getClass().getDeclaredMethod("writeReplace");throw new AssertionError();}catch(NoSuchMethodException e){System.out.println("ordinary lambda has no replacement");}
    }
}
