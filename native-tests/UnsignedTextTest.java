public class UnsignedTextTest {
    static void one(long value) {
        System.out.println(Long.toHexString(value)+"|"+Long.toOctalString(value)+"|"+Long.toBinaryString(value));
        int small=(int)value;
        System.out.println(Integer.toHexString(small)+"|"+Integer.toOctalString(small)+"|"+Integer.toBinaryString(small));
    }
    public static void main(String[] args) {
        one(0); one(-1); one(Long.MIN_VALUE); one(Long.MAX_VALUE);
        for(int bit=0;bit<64;bit++) { long value=1L<<bit;one(value);one(value-1); }
        long value=0x7adca95ea12789L;
        for(int i=0;i<128;i++) { value=value*6364136223846793005L+1442695040888963407L;one(value); }
    }
}
