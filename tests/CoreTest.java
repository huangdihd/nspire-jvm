interface Adder { int apply(int n); }
class Base { int x; Base(int x) { this.x = x; } int value() { return x; } }
class Derived extends Base implements Adder {
    long extra;
    Derived(int x) { super(x); extra = 1234567890123L; }
    int value() { return x * 3; }
    public int apply(int n) { return value() + n; }
}
class Boom extends RuntimeException { Boom() { super(); } }
public class CoreTest {
    static int init = initialize();
    static int initialize() { return 73; }
    static int dense(int x) { switch(x) { case 1: return 12; case 2: return 24; case 3: return 36; default: return -1; } }
    static int sparse(int x) { switch(x) { case -123: return 3; case 700: return 8; case 99999: return 11; default: return 0; } }
    static void boom() { throw new Boom(); }
    static void output(long x) { System.out.println(x); }
    public static void main(String[] args) {
        System.out.println(init);
        Base b = new Derived(7);
        System.out.println(b.value());
        System.out.println(((Adder)b).apply(4));
        Derived d = (Derived)b;
        System.out.println(d.extra++);
        System.out.println(++d.extra);
        d.x += 2; System.out.println(d.value());
        long[] la = {Long.MIN_VALUE, 7, 13};
        output(la[1]++); output(++la[1]);
        int[] a = {2,4,6,8,10}; System.arraycopy(a,0,a,1,4);
        for(int i=0;i<a.length;i++)System.out.println(a[i]);
        int[][] multi = new int[3][4]; multi[2][3] = 89; System.out.println(multi[2][3]);
        System.out.println(multi instanceof Object[]);
        Object[] oa = new Derived[2]; oa[0] = d;
        try { oa[1] = new Base(1); } catch(ArrayStoreException e) { System.out.println("array store caught"); }
        try { int i = -1; System.out.println(a[i]); } catch(IndexOutOfBoundsException e) { System.out.println("bounds caught"); }
        try { Object z = null; z.hashCode(); } catch(NullPointerException e) { System.out.println("null caught"); }
        try { boom(); } catch(Exception e) { System.out.println("cross-frame exception caught"); }
        try { int[] z = new int[-3]; } catch(NegativeArraySizeException e) { System.out.println("size caught"); }
        Object wrong = new Base(1);
        try { Derived z = (Derived)wrong; } catch(ClassCastException e) { System.out.println("cast caught"); }
        for(int i=0;i<5;i++)System.out.println(dense(i));
        System.out.println(sparse(-123));System.out.println(sparse(700));System.out.println(sparse(99999));
        System.out.println("hello".length()); System.out.println("hello".charAt(1));
        System.out.println("same" == "same");
        System.out.println("same".equals("same")); System.out.println("same".equals(null));
        CharSequence text="hello";System.out.println(text.length());System.out.println(text.charAt(2));System.out.println(text.subSequence(1,4));
        System.out.println(((Comparable<String>)"abc").compareTo("abd"));
        System.out.println("jar:file:/logback.xml".endsWith(".xml"));System.out.println("abc".endsWith("abcd"));
        System.out.println("abc".startsWith("bc",1));System.out.println("abc".startsWith("",3));System.out.println("abc".startsWith("",4));
        System.out.println("a\ud83d\ude00".startsWith("\ude00",2));System.out.println("a\ud83d\ude00".endsWith("\ude00"));
        try{"x".endsWith(null);}catch(NullPointerException e){System.out.println("null suffix rejected");}
        System.out.println("answer=" + d.value() + ",long=" + d.extra);
        byte[] bytes = new byte[1]; bytes[0] = (byte)255; System.out.println(bytes[0]);
        short[] shorts = {(short)65535}; System.out.println(shorts[0]);
        char[] chars = {(char)65535}; System.out.println((int)chars[0]);
        boolean[] bool = {true,false};System.out.println(bool[0]);
        System.out.println(args.length);
        for(String arg:args)System.out.println(arg);
    }
}
