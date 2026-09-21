public class RecordFloatingTest {
    record Numbers(float small, double large) {}
    public static void main(String[] args) {
        long[] edges={0L,Long.MIN_VALUE,1L,0x000fffffffffffffL,0x0010000000000000L,0x7fefffffffffffffL,0x7ff0000000000000L,0xfff0000000000000L,0x7ff8000000000001L,0x3ff0000000000000L,0x3f1a36e2eb1c432dL,0x416312d000000000L};
        int[] floats={0,Integer.MIN_VALUE,1,0x007fffff,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc00001,0x3f800000,0x38d1b717,0x4b189680};
        for(int i=0;i<edges.length;i++)System.out.println(new Numbers(Float.intBitsToFloat(floats[i]),Double.longBitsToDouble(edges[i])));
        long bits=0x123456789abcdefL;
        for(int i=0;i<300;i++) {
            bits^=bits<<13;bits^=bits>>>7;bits^=bits<<17;
            Numbers value=new Numbers(Float.intBitsToFloat((int)(bits>>>32)),Double.longBitsToDouble(bits));
            System.out.println(value); if(i%13==0)System.gc();
        }
    }
}
