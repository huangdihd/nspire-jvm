package reflect;
import java.io.IOException;
public class Fixture {
    public static int initializations;
    public static Object[] callerArguments;
    public static class Target {
        static {initializations++;callerArguments[0]=null;System.gc();}
        public final Object payload;public final long number;public final boolean flag;public final double real;
        public Target(Object payload,long number,boolean flag,double real){this.payload=payload;this.number=number;this.flag=flag;this.real=real;System.gc();}
    }
    public static class Hidden {private Hidden(){} }
    public static class Broken {public Broken()throws IOException{throw new IOException("ctor-failure");} }
    public static abstract class Abstract {public Abstract(){} }
    public static class Small {public Small(byte value){} }
    public static class BadArity {static{initializations+=10;}public BadArity(String value){} }
    public static class Snapshot {
        public final Object payload;
        public Snapshot(Object payload){callerArguments[0]=null;System.gc();this.payload=payload;}
    }
    public static class Varargs {public final int size;public Varargs(String... values){size=values.length;} }
}
