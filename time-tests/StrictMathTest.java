public class StrictMathTest {
    static void check(long bits) {
        double x=Double.longBitsToDouble(bits);
        System.out.println(bits+"|"+Double.doubleToLongBits(StrictMath.log(x))+"|"+Double.doubleToLongBits(StrictMath.sqrt(x)));
    }
    public static void main(String[] args) {
        for(long bits:new long[]{0,Long.MIN_VALUE,1,-1,0x000fffffffffffffL,0x0010000000000000L,
                0x3fefffffffffffffL,0x3ff0000000000000L,0x3ff0000000000001L,0x4000000000000000L,
                0x7fefffffffffffffL,0x7ff0000000000000L,0xfff0000000000000L,
                0x7ff0000000000001L,0x7ff8000000000000L,0xfff8000000000000L})check(bits);
        for(int exponent=0;exponent<2047;exponent+=13) {
            long bits=(long)exponent<<52;
            check(bits);check(bits+1);if(bits>0)check(bits-1);
        }
        long state=314159265358979L;
        for(int i=0;i<2048;i++) {
            state=state*6364136223846793005L+1442695040888963407L;
            check(state);check(state&Long.MAX_VALUE);
        }
    }
}
