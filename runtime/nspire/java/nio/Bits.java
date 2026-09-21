/* Heap-buffer byte access for Nspire JVM. MIT license; see project LICENSE.
 * Deliberately has no absolute-address/direct-memory API. */
package java.nio;
final class Bits {
    static ByteOrder byteOrder() {
        return "big".equals(sun.misc.VM.getSavedProperty("sun.cpu.endian")) ? ByteOrder.BIG_ENDIAN : ByteOrder.LITTLE_ENDIAN;
    }
    private static long read(ByteBuffer b,int index,int size,boolean big) {
        long value=0;
        for(int i=0;i<size;i++)value=(value<<8)|(b._get(index+(big?i:size-1-i))&255L);
        return value;
    }
    private static void write(ByteBuffer b,int index,long value,int size,boolean big) {
        for(int i=0;i<size;i++)b._put(index+(big?size-1-i:i),(byte)(value>>>(i*8)));
    }
    static char getChar(ByteBuffer b,int i,boolean big){return (char)read(b,i,2,big);}
    static char getCharB(ByteBuffer b,int i){return getChar(b,i,true);}
    static char getCharL(ByteBuffer b,int i){return getChar(b,i,false);}
    static void putChar(ByteBuffer b,int i,char v,boolean big){write(b,i,v,2,big);}
    static void putCharB(ByteBuffer b,int i,char v){putChar(b,i,v,true);}
    static void putCharL(ByteBuffer b,int i,char v){putChar(b,i,v,false);}
    static short getShort(ByteBuffer b,int i,boolean big){return (short)read(b,i,2,big);}
    static short getShortB(ByteBuffer b,int i){return getShort(b,i,true);}
    static short getShortL(ByteBuffer b,int i){return getShort(b,i,false);}
    static void putShort(ByteBuffer b,int i,short v,boolean big){write(b,i,v,2,big);}
    static void putShortB(ByteBuffer b,int i,short v){putShort(b,i,v,true);}
    static void putShortL(ByteBuffer b,int i,short v){putShort(b,i,v,false);}
    static int getInt(ByteBuffer b,int i,boolean big){return (int)read(b,i,4,big);}
    static int getIntB(ByteBuffer b,int i){return getInt(b,i,true);}
    static int getIntL(ByteBuffer b,int i){return getInt(b,i,false);}
    static void putInt(ByteBuffer b,int i,int v,boolean big){write(b,i,v,4,big);}
    static void putIntB(ByteBuffer b,int i,int v){putInt(b,i,v,true);}
    static void putIntL(ByteBuffer b,int i,int v){putInt(b,i,v,false);}
    static long getLong(ByteBuffer b,int i,boolean big){return read(b,i,8,big);}
    static long getLongB(ByteBuffer b,int i){return getLong(b,i,true);}
    static long getLongL(ByteBuffer b,int i){return getLong(b,i,false);}
    static void putLong(ByteBuffer b,int i,long v,boolean big){write(b,i,v,8,big);}
    static void putLongB(ByteBuffer b,int i,long v){putLong(b,i,v,true);}
    static void putLongL(ByteBuffer b,int i,long v){putLong(b,i,v,false);}
    static float getFloat(ByteBuffer b,int i,boolean big){return Float.intBitsToFloat(getInt(b,i,big));}
    static float getFloatB(ByteBuffer b,int i){return getFloat(b,i,true);}
    static float getFloatL(ByteBuffer b,int i){return getFloat(b,i,false);}
    static void putFloat(ByteBuffer b,int i,float v,boolean big){putInt(b,i,Float.floatToRawIntBits(v),big);}
    static void putFloatB(ByteBuffer b,int i,float v){putFloat(b,i,v,true);}
    static void putFloatL(ByteBuffer b,int i,float v){putFloat(b,i,v,false);}
    static double getDouble(ByteBuffer b,int i,boolean big){return Double.longBitsToDouble(getLong(b,i,big));}
    static double getDoubleB(ByteBuffer b,int i){return getDouble(b,i,true);}
    static double getDoubleL(ByteBuffer b,int i){return getDouble(b,i,false);}
    static void putDouble(ByteBuffer b,int i,double v,boolean big){putLong(b,i,Double.doubleToRawLongBits(v),big);}
    static void putDoubleB(ByteBuffer b,int i,double v){putDouble(b,i,v,true);}
    static void putDoubleL(ByteBuffer b,int i,double v){putDouble(b,i,v,false);}
}
