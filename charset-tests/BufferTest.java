import java.nio.*;
public class BufferTest {
    public static void main(String[] args)throws Exception {
        ByteBuffer b=ByteBuffer.allocate(32);b.position(2);b.put((byte)11).put((byte)12).mark();b.put((byte)13);b.reset();System.out.println(b.position());
        b.flip();b.position(2);ByteBuffer slice=b.slice();slice.put(0,(byte)33);System.out.println(b.get(2));
        ByteBuffer read=b.asReadOnlyBuffer();CharsetTest.error(()->read.put((byte)4));CharsetTest.error(()->read.array());
        b.compact();System.out.println(b.position()+":"+b.limit());b.clear();CharsetTest.error(()->b.reset());
        CharsetTest.error(()->b.position(40));CharsetTest.error(()->b.limit(-1));
        for(ByteOrder order:new ByteOrder[]{ByteOrder.BIG_ENDIAN,ByteOrder.LITTLE_ENDIAN}) {
            b.clear().limit(32);b.order(order);b.putChar('中').putShort((short)-123).putInt(0x89abcdef).putLong(0x1234567887654321L).putFloat(-0.0f).putDouble(Double.NaN);b.flip();
            System.out.println((int)b.getChar()+":"+b.getShort()+":"+b.getInt()+":"+b.getLong()+":"+Float.floatToRawIntBits(b.getFloat())+":"+Double.doubleToLongBits(b.getDouble()));
            b.clear();b.asIntBuffer().put(0,0x12345678);System.out.println(b.getInt(0));
            b.asCharBuffer().put(2,'字');System.out.println((int)b.getChar(4));
            b.asLongBuffer().put(1,-45);System.out.println(b.getLong(8));
            b.asShortBuffer().put(1,(short)99);System.out.println(b.getShort(2));
            b.asFloatBuffer().put(4,1.5f);System.out.println(Float.floatToIntBits(b.getFloat(16)));
            b.asDoubleBuffer().put(3,1.5);System.out.println(Double.doubleToLongBits(b.getDouble(24)));
        }
        CharBuffer chars=CharBuffer.wrap("012345");chars.position(1).limit(5);System.out.println(chars.subSequence(1,3));System.out.println(chars.chars().sum());
        CharBuffer copy=CharBuffer.allocate(7);copy.put(chars).flip();System.out.println(copy.toString());
        CharsetTest.error(()->copy.get(6));copy.position(copy.limit());CharsetTest.error(()->copy.get());
        ByteBuffer full=ByteBuffer.allocate(0);CharsetTest.error(()->full.put((byte)1));
        System.out.println(ByteOrder.nativeOrder()==ByteOrder.LITTLE_ENDIAN);
    }
}
