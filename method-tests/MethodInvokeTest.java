import java.lang.reflect.*;
public class MethodInvokeTest {
    static int initialized;
    public static class Target {
        static {initialized++;}
        int value;
        public void setValue(int value){this.value=value;System.gc();}
        public int getValue(){return value;}
        public static long wide(long a,int b){return a+b;}
        public static double decimal(double a){return a;}
        public byte by(byte x){return x;}public short sh(short x){return x;}public char ch(char x){return x;}
        public float fl(float x){return x;}public boolean bool(boolean x){return x;}
        public String reference(String[] a){return a[0];}
        public String varargs(String...a){return a[0];}
        public void fail(){throw new IllegalStateException("target");}
        public void fatal(){throw new AssertionError();}
        public synchronized boolean locked(){return Thread.holdsLock(this);}
        public String dispatch(){return "base";}
        private String privateCall(){return "private-base";}
    }
    public static class Derived extends Target {public String dispatch(){return "derived";} public String privateCall(){return "derived-public";}}
    interface Action {void run()throws Exception;}
    static void check(Action a){try{a.run();System.out.println("ok");}catch(Exception e){System.out.println(e.getClass().getName());}}
    public static void main(String[] args)throws Exception {
        Class<?> type=Target.class;Method setter=type.getMethod("setValue",int.class);System.out.println(initialized);
        Target object=new Target();System.out.println(initialized);System.out.println(setter.invoke(object,Short.valueOf((short)12))==null);
        System.out.println(type.getMethod("getValue").invoke(object));
        System.out.println(type.getMethod("wide",long.class,int.class).invoke("ignored",Integer.valueOf(100),Byte.valueOf((byte)4)));
        System.out.println(Double.doubleToLongBits((Double)type.getMethod("decimal",double.class).invoke(null,Float.valueOf(1.25f))));
        System.out.println(type.getMethod("by",byte.class).invoke(object,Byte.valueOf((byte)-8)));
        System.out.println(type.getMethod("sh",short.class).invoke(object,Byte.valueOf((byte)7)));
        System.out.println((int)((Character)type.getMethod("ch",char.class).invoke(object,Character.valueOf('中'))).charValue());
        System.out.println(Float.floatToIntBits((Float)type.getMethod("fl",float.class).invoke(object,Long.valueOf(19))));
        System.out.println(type.getMethod("bool",boolean.class).invoke(object,true));
        System.out.println(type.getMethod("reference",String[].class).invoke(object,(Object)new String[]{"reference"}));
        System.out.println(type.getMethod("varargs",String[].class).invoke(object,(Object)new String[]{"varargs"}));
        check(()->type.getMethod("varargs",String[].class).invoke(object,"a","b"));
        System.out.println(type.getMethod("locked").invoke(object));
        System.out.println(type.getMethod("dispatch").invoke(new Derived()));
        Method hidden=type.getDeclaredMethod("privateCall");hidden.setAccessible(true);System.out.println(hidden.invoke(new Derived()));
        check(()->setter.invoke(null,1));check(()->setter.invoke("wrong",1));check(()->setter.invoke(object));
        check(()->setter.invoke(object,(Object)null));check(()->setter.invoke(object,Long.valueOf(1)));check(()->setter.invoke(object,true));
        check(()->type.getMethod("bool",boolean.class).invoke(object,1));
        check(()->type.getMethod("ch",char.class).invoke(object,Byte.valueOf((byte)1)));
        for(String n:new String[]{"fail","fatal"})try{type.getMethod(n).invoke(object);}catch(InvocationTargetException e){System.out.println(e.getCause().getClass().getName());System.out.println(e.getTargetException()==e.getCause());
            try{e.initCause(null);throw new AssertionError();}catch(IllegalStateException expected){System.out.println("wrapper cause locked");}}
        System.out.println(Boolean.class.getMethod("valueOf",String.class).invoke(null,"TRUE"));
        System.out.println(Object.class.getMethod("equals",Object.class).invoke("same","same"));
        System.out.println(Object.class.getMethod("hashCode").invoke("text").equals("text".hashCode()));
        System.out.println(Object.class.getMethod("getClass").invoke(object)==Target.class);
        for(int i=0;i<100;i++){Method m=type.getMethod("getValue");System.gc();if(((Integer)m.invoke(object))!=12)throw new AssertionError();}
        System.out.println("reflection GC passed");
    }
}
