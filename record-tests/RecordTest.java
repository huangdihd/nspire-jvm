import java.util.HashSet;

public class RecordTest {
    record Empty() {}
    record OtherEmpty() {}
    record Pair(int number, String label) {}
    record Changed(int number) { public int number() { return number + 1; } }
    record Normalized(int value) { Normalized { value = Math.abs(value); } }
    record Primitives(boolean z, byte b, short s, char c, int i, long l, float f, double d) {}
    record Floating(float f, double d) {}
    record References(Object first, Object second) {}
    record Callbacks(Callback first, Callback second, Callback third) {}
    static String trace = "";
    static class Callback {
        final String name; final int value;
        Callback(String name, int value) { this.name = name; this.value = value; }
        public boolean equals(Object other) { trace += "e" + name; pressure(); return other instanceof Callback && ((Callback)other).value == value; }
        public int hashCode() { trace += "h" + name; pressure(); return value; }
        public String toString() { trace += "t" + name; pressure(); return name; }
    }
    static class Throws {
        public boolean equals(Object other) { throw new IllegalStateException("equals"); }
        public int hashCode() { throw new IllegalStateException("hash"); }
        public String toString() { throw new IllegalStateException("text"); }
    }
    static class NullText { public String toString() { return null; } }
    static void pressure() { for (int i=0;i<10;i++) { byte[] data=new byte[2000]; data[1]=2; } System.gc(); }
    static void check(boolean value) { if (!value) throw new AssertionError("record check"); }
    public static void main(String[] args) throws Exception {
        check(Empty.class.isRecord() && Pair.class.isRecord());
        check(!Record.class.isRecord() && !Object.class.isRecord() && !int.class.isRecord() && !Pair[].class.isRecord());
        check(Pair.class.getSuperclass() == Record.class && new Pair(1,"x") instanceof Record);
        Empty empty=new Empty(); check(empty.equals(new Empty()) && !empty.equals(new OtherEmpty()) && !empty.equals(null));
        check(empty.hashCode()==0); System.out.println(empty);
        Pair p=new Pair(-9,"中\u0000"); check(p.equals(new Pair(-9,new String(new char[]{'中',0}))));
        check(!p.equals(new Pair(9,p.label())) && !p.equals(new Pair(-9,null)) && !p.equals(null));
        check(new Pair(4,null).equals(new Pair(4,null)));
        System.out.println(p); System.out.println(p.hashCode()); System.out.println(new Pair(4,null));
        Changed changed=new Changed(7); check(changed.number()==8 && changed.hashCode()==7); System.out.println(changed);
        Normalized normalized=new Normalized(-8); check(normalized.value()==8); System.out.println(normalized);
        record Local(String value) {} System.out.println(new Local("local")); check(Local.class.isRecord());
        Primitives all=new Primitives(true,(byte)-100,(short)-30000,'中',Integer.MIN_VALUE,Long.MIN_VALUE,Float.NaN,-0.0);
        Primitives same=new Primitives(true,(byte)-100,(short)-30000,'中',Integer.MIN_VALUE,Long.MIN_VALUE,Float.intBitsToFloat(0xffc00004),-0.0);
        check(all.equals(same) && all.hashCode()==same.hashCode()); System.out.println(all); System.out.println(all.hashCode());
        check(!new Floating(0.0f,0.0).equals(new Floating(-0.0f,0.0)));
        check(!new Floating(0.0f,0.0).equals(new Floating(0.0f,-0.0)));
        check(new Floating(Float.NaN,Double.NaN).equals(new Floating(Float.intBitsToFloat(0x7fa00001),Double.longBitsToDouble(0xfff8000000000023L))));
        Object[] array={"x"}; References ref=new References(array,null);
        check(ref.equals(new References(array,null)) && !ref.equals(new References(new Object[]{"x"},null)));
        check(ref.hashCode()==31*array.hashCode()); check(ref.toString().equals("References[first="+array.toString()+", second=null]"));
        System.out.println(new References(new NullText(),null));
        HashSet<Pair> set=new HashSet<>(); set.add(new Pair(2,"two")); set.add(new Pair(2,"two")); check(set.size()==1);
        Callbacks a=new Callbacks(new Callback("a",1),new Callback("b",2),new Callback("c",3));
        Callbacks b=new Callbacks(new Callback("a",1),new Callback("b",2),new Callback("c",3));
        trace=""; check(a.equals(b)); System.out.println("equals:"+trace);
        trace=""; System.out.println(a.hashCode()); System.out.println("hash:"+trace);
        trace=""; System.out.println(a); System.out.println("text:"+trace);
        trace=""; check(a.equals(a)); check(trace.isEmpty());
        for(int operation=0;operation<3;operation++) {
            try { References x=new References(new Throws(),null); if(operation==0)x.equals(new References(new Throws(),null));else if(operation==1)x.hashCode();else x.toString(); throw new AssertionError(); }
            catch(IllegalStateException expected){System.out.println(expected.getMessage());}
        }
        check(Pair.class.getDeclaredConstructor(int.class,String.class).newInstance(4,"four").equals(new Pair(4,"four")));
        System.out.println("record behavior passed");
    }
}
