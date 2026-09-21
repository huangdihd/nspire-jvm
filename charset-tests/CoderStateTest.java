import java.nio.*;
import java.nio.charset.*;
public class CoderStateTest {
    static void result(CoderResult r,Buffer in,Buffer out){System.out.println(r+":"+in.position()+":"+out.position());}
    public static void main(String[] args)throws Exception {
        CharsetDecoder d=StandardCharsets.UTF_8.newDecoder();ByteBuffer in=ByteBuffer.allocate(8);CharBuffer out=CharBuffer.allocate(1);
        in.put((byte)0xf0).put((byte)0x9f).flip();result(d.decode(in,out,false),in,out);
        in.compact().put((byte)0x98).put((byte)0x80).flip();result(d.decode(in,out,false),in,out);
        out=CharBuffer.allocate(2);result(d.decode(in,out,true),in,out);System.out.println(d.flush(out));out.flip();EncodingTest.chars(out.toString());
        CharsetTest.error(()->d.decode(ByteBuffer.allocate(0),CharBuffer.allocate(0),false));
        d.reset();System.out.println(d.onMalformedInput(CodingErrorAction.REPLACE).replaceWith("X").decode(ByteBuffer.wrap(new byte[]{(byte)0xff,65})));
        d.reset();d.onMalformedInput(CodingErrorAction.IGNORE);System.out.println(d.decode(ByteBuffer.wrap(new byte[]{(byte)0xff,65})));
        CharsetEncoder e=StandardCharsets.UTF_8.newEncoder();CharBuffer chars=CharBuffer.allocate(3);ByteBuffer bytes=ByteBuffer.allocate(3);
        chars.put('\ud83d').flip();result(e.encode(chars,bytes,false),chars,bytes);
        chars.compact().put('\ude00').flip();result(e.encode(chars,bytes,false),chars,bytes);
        bytes=ByteBuffer.allocate(4);result(e.encode(chars,bytes,true),chars,bytes);System.out.println(e.flush(bytes));EncodingTest.bytes(bytes.array());
        CharsetTest.error(()->e.encode(CharBuffer.allocate(0),ByteBuffer.allocate(0),false));
        e.reset();System.out.println(e.canEncode("\ud83d\ude00"));System.out.println(e.canEncode("\ud800"));
        CharsetEncoder ascii=StandardCharsets.US_ASCII.newEncoder().onUnmappableCharacter(CodingErrorAction.REPLACE).replaceWith(new byte[]{33});
        ByteBuffer replacement=ascii.encode(CharBuffer.wrap("\u4e2d\ud83d\ude00A"));byte[] result=new byte[replacement.remaining()];replacement.get(result);EncodingTest.bytes(result);
        CharsetTest.error(()->StandardCharsets.UTF_8.newEncoder().replaceWith(new byte[]{(byte)0xff}));
        for(int i=1;i<5;i++){CoderResult r=CoderResult.malformedForLength(i);System.out.println(r+":"+r.length()+":"+r.isError());CharsetTest.error(()->r.throwException());}
        CharsetTest.error(()->CoderResult.UNDERFLOW.length());CharsetTest.error(()->CoderResult.OVERFLOW.throwException());
        CharsetTest.error(()->CoderResult.unmappableForLength(0));
    }
}
