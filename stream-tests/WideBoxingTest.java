import java.util.*;

public class WideBoxingTest {
    public static class Values {
        public Values(long x,double y){System.out.println(x);System.out.println(Double.doubleToLongBits(y));}
    }
    public static void main(String[] args) throws Exception {
        long[] longs={Long.MIN_VALUE,-129,-128,0,127,128,Long.MAX_VALUE};
        for(long x:longs) {
            Long a=Long.valueOf(x),b=Long.valueOf(x);
            if(x>=-128&&x<=127)System.out.println(a==b);
            System.out.println(a.longValue());System.out.println(a.intValue());System.out.println(a.shortValue());System.out.println(a.byteValue());
            System.out.println(a.equals(b));System.out.println(a.hashCode());System.out.println(a.compareTo(0L));
            System.gc();System.out.println(a.longValue()==x);
        }
        double[] doubles={Double.NaN,Double.longBitsToDouble(0x7ff0000000000001L),Double.POSITIVE_INFINITY,
            Double.NEGATIVE_INFINITY,0.0,-0.0,1.5,2147483648.0,9223372036854775808.0,-9223372036854775808.0};
        for(double x:doubles) {
            Double boxed=Double.valueOf(x);
            System.out.println(boxed.hashCode());System.out.println(boxed.equals(Double.valueOf(x)));
            System.out.println(boxed.intValue());System.out.println(boxed.longValue());
            System.out.println(boxed.shortValue());System.out.println(boxed.byteValue());
            System.out.println(boxed.compareTo(Double.valueOf(0.0)));System.out.println(boxed.compareTo(Double.valueOf(Double.NaN)));
        }
        System.out.println(Double.valueOf(0.0).equals(Double.valueOf(-0.0)));
        System.out.println(Double.valueOf(1).equals(Long.valueOf(1)));
        HashSet<Long> set=new HashSet<>();set.add(1L);set.add(1L);set.add(Long.MAX_VALUE);System.out.println(set.size());
        Values.class.getConstructor(long.class,double.class).newInstance(Long.valueOf(Long.MAX_VALUE),Long.valueOf(17));
        Values.class.getConstructor(long.class,double.class).newInstance(Integer.valueOf(5),Double.valueOf(1.5));
        try { Values.class.getConstructor(long.class,double.class).newInstance(Double.valueOf(1),Double.valueOf(2)); }
        catch(IllegalArgumentException expected){System.out.println("no narrowing");}
    }
}
