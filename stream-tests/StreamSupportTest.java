import java.util.*;

public class StreamSupportTest {
    static class Sequence implements CharSequence {
        final String text;
        Sequence(String text){this.text=text;}
        public int length(){System.gc();return text.length();}
        public char charAt(int i){System.gc();return text.charAt(i);}
        public CharSequence subSequence(int a,int b){return text.substring(a,b);}
        public String toString(){throw new AssertionError("append must use CharSequence methods");}
    }
    static class Broken extends Sequence {
        Broken(){super("xy");}
        public char charAt(int i){if(i==1)throw new IllegalStateException();return super.charAt(i);}
    }
    static void show(StringBuilder s) {
        System.out.println(s.length());
        for(int i=0;i<s.length();i++)System.out.println((int)s.charAt(i));
    }
    public static void main(String[] args) {
        StringBuilder b=new StringBuilder().append("p");
        b.append(new Sequence("中\ud801\udc00z"),1,3);show(b);
        b.append((CharSequence)null,1,3);show(b);
        b.append((CharSequence)b,0,2);show(b);
        b.setLength(2);show(b);b.setLength(5);show(b);b.setLength(0);show(b);
        b.append(new Sequence("abc"));
        try{b.append(new Broken());}catch(IllegalStateException expected){System.out.println(b.toString());}
        try{b.append(new Sequence("ab"),-1,2);}catch(IndexOutOfBoundsException expected){System.out.println("bounds");}
        try{b.setLength(-1);}catch(StringIndexOutOfBoundsException expected){System.out.println("negative length");}
        StringJoiner join=new StringJoiner(",","[","]");join.add("a").add("b");
        System.out.println(join.toString());System.out.println(join.toString());
        join.merge(new StringJoiner(";").add("c").add("d"));System.out.println(join.toString());
        int[] ints={0,1,-1,Integer.MIN_VALUE,Integer.MAX_VALUE,0x55555555,0x123400};
        for(int x:ints)System.out.println(Integer.bitCount(x)+"/"+Integer.numberOfLeadingZeros(x)+"/"+Integer.numberOfTrailingZeros(x));
        long[] longs={0L,1L,-1L,Long.MIN_VALUE,Long.MAX_VALUE,0x5555555555555555L,0x123456789000L};
        for(long x:longs)System.out.println(Long.bitCount(x)+"/"+Long.numberOfLeadingZeros(x)+"/"+Long.numberOfTrailingZeros(x));
        System.out.println(Math.min(Long.MIN_VALUE,Long.MAX_VALUE));System.out.println(Math.max(Long.MIN_VALUE,Long.MAX_VALUE));
        System.out.println(Integer.compare(Integer.MIN_VALUE,Integer.MAX_VALUE));System.out.println(Long.compare(Long.MAX_VALUE,Long.MIN_VALUE));
        System.out.println(Integer.sum(Integer.MAX_VALUE,1));System.out.println(Long.sum(Long.MAX_VALUE,1));
        double[] doubles={0,-0.0,Double.NaN,Double.POSITIVE_INFINITY,Double.NEGATIVE_INFINITY,1.5};
        for(double d:doubles)System.out.println(Double.isNaN(d)+"/"+Double.isInfinite(d));
    }
}
