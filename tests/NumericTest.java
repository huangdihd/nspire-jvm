public class NumericTest {
    static int check(int x,int y) {
        int r = x + y; r ^= x-y; r ^= x*y;
        if(y!=0) { r ^= x/y; r ^= x%y; }
        return r ^ (x<<y) ^ (x>>y) ^ (x>>>y) ^ (x&y) ^ (x|y) ^ -x;
    }
    static long check(long x,long y) {
        long r=x+y; r^=x-y; r^=x*y;
        if(y!=0) {r^=x/y; r^=x%y;}
        return r^(x<<y)^(x>>y)^(x>>>y)^(x&y)^(x|y)^-x;
    }
    public static void main(String[] args) {
        int[] ints={0,1,-1,Integer.MIN_VALUE,Integer.MAX_VALUE,1234567,-1234567,32,64};
        long[] longs={0,1,-1,Long.MIN_VALUE,Long.MAX_VALUE,123456789012345L,-123456789012345L,32,64};
        for(int x:ints)for(int y:ints)System.out.println(check(x,y));
        for(long x:longs)for(long y:longs)System.out.println(check(x,y));
        double[] doubles={0,-0.0,1.75,-1.75,1e100,-1e100,Double.NaN,Double.POSITIVE_INFINITY,Double.NEGATIVE_INFINITY};
        for(double x:doubles){System.out.println((int)x);System.out.println((long)x);System.out.println(x<0);System.out.println(x>=0);}
        float f=1.5f;double d=2.25;
        System.out.println((int)(f*8));System.out.println((long)(d*16));
        System.out.println((int)(f+1.5f));System.out.println((int)(d-0.25));
        System.out.println((int)(10.5f%3));System.out.println((int)(10.5%3));
        System.out.println((int)Math.sqrt(144));
        System.out.println(Math.abs(Integer.MIN_VALUE)); System.out.println(Math.abs(Long.MIN_VALUE));
        System.out.println(Math.min(4,9));System.out.println(Math.max(4,9));
        for(double x:doubles)for(double y:doubles) {
            double lo=Math.min(x,y),hi=Math.max(x,y);
            System.out.println(Double.isNaN(lo));System.out.println(Double.isNaN(hi));
            System.out.println((long)lo);System.out.println((long)hi);
            System.out.println(1.0/lo==Double.NEGATIVE_INFINITY);System.out.println(1.0/hi==Double.NEGATIVE_INFINITY);
            float flo=Math.min((float)x,(float)y),fhi=Math.max((float)x,(float)y);
            System.out.println(Float.isNaN(flo));System.out.println(Float.isNaN(fhi));
            System.out.println((long)flo);System.out.println((long)fhi);
            System.out.println(1.0f/flo==Float.NEGATIVE_INFINITY);System.out.println(1.0f/fhi==Float.NEGATIVE_INFINITY);
        }
        int[] fbits={0,0x80000000,0x7f800000,0xff800000,0x7fc01234,0xffc05678,1,0x3fc00000};
        for(int bits:fbits){float value=Float.intBitsToFloat(bits);System.out.println(Float.floatToRawIntBits(value));System.out.println(Float.floatToIntBits(value));}
        long[] dbits={0L,0x8000000000000000L,0x7ff0000000000000L,0xfff0000000000000L,0x7ff8000000001234L,0xfff8000000005678L,1L,0x3ff8000000000000L};
        for(long bits:dbits){double value=Double.longBitsToDouble(bits);System.out.println(Double.doubleToRawLongBits(value));System.out.println(Double.doubleToLongBits(value));}
    }
}
