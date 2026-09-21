import java.lang.reflect.*;
public class MethodConversionTest {
    public static byte b(byte x){return x;}public static short s(short x){return x;}
    public static char c(char x){return x;}public static int i(int x){return x;}
    public static long j(long x){return x;}public static float f(float x){return x;}
    public static double d(double x){return x;}public static boolean z(boolean x){return x;}
    static void show(Object x){
        System.out.println(x.getClass().getName());
        if(x instanceof Character)System.out.println((int)((Character)x).charValue());
        else if(x instanceof Boolean)System.out.println(x);
        else System.out.println(Double.doubleToLongBits(((Number)x).doubleValue()));
    }
    public static void main(String[] args)throws Exception {
        Object[] values={Byte.valueOf((byte)-2),Short.valueOf((short)300),Character.valueOf('中'),Integer.valueOf(-234567),Long.valueOf(9876543210L),Float.valueOf(1.25f),Double.valueOf(-9.5),Boolean.TRUE};
        Class<?>[] types={byte.class,short.class,char.class,int.class,long.class,float.class,double.class,boolean.class};
        String[] names={"b","s","c","i","j","f","d","z"};
        for(int to=0;to<types.length;to++)for(Object value:values){
            try{show(MethodConversionTest.class.getMethod(names[to],types[to]).invoke(null,value));}
            catch(IllegalArgumentException e){System.out.println("conversion rejected");}
        }
        for(int bits:new int[]{0,0x80000000,0x7f800000,0xff800000,0x7fffffff,0xffffffff}){
            Float value=Float.valueOf(Float.intBitsToFloat(bits));
            System.out.println(value.hashCode());System.out.println(value.equals(Float.valueOf(Float.intBitsToFloat(bits))));
            System.out.println(value.intValue());System.out.println(value.longValue());
            System.out.println(Float.floatToRawIntBits((Float)MethodConversionTest.class.getMethod("f",float.class).invoke(null,value)));
        }
        for(int n=-128;n<128;n++)if(Byte.valueOf((byte)n)!=Byte.valueOf((byte)n)||Short.valueOf((short)n)!=Short.valueOf((short)n))throw new AssertionError();
        for(int n=0;n<128;n++)if(Character.valueOf((char)n)!=Character.valueOf((char)n))throw new AssertionError();
        System.out.println("wrapper caches passed");
    }
}
