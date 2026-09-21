import java.nio.*;
import java.nio.charset.*;
import java.util.*;
public class EncodingTest {
    static void chars(String s){System.out.print(s.length()+":");for(int i=0;i<s.length();i++)System.out.print((int)s.charAt(i)+",");System.out.println();}
    static void bytes(byte[] b){System.out.println(Arrays.toString(b));}
    public static void main(String[] args)throws Exception {
        for(String name:CharsetTest.names) {
            Charset cs=Charset.forName(name);System.out.println(name);
            for(String text:new String[]{"","plain\u0000text","\u00a3\u00ff\u4e2d\u6587\ud83d\ude00","\ud800","\udc00","a\ud800z\udc00\ud800\ud800"}) {
                byte[] encoded=text.getBytes(cs);bytes(encoded);bytes(text.getBytes(name));
                chars(new String(encoded,cs));chars(new String(encoded,0,encoded.length,name));
                ByteBuffer bb=cs.encode(CharBuffer.wrap(text.toCharArray()));byte[] b=new byte[bb.remaining()];bb.get(b);bytes(b);
                chars(cs.decode(ByteBuffer.wrap(encoded).asReadOnlyBuffer()).toString());
            }
            for(byte[] data:new byte[][]{{0},{(byte)0xc0,(byte)0x80},{(byte)0xed,(byte)0xa0,(byte)0x80},{(byte)0xf0,(byte)0x9f},{(byte)0xff,(byte)0xfe,65,0},{(byte)0xfe,(byte)0xff,0,65},{(byte)0xe2,40,(byte)0xa1},{(byte)0xf4,(byte)0x90,(byte)0x80,(byte)0x80}}) {
                chars(new String(data,cs));
                CharsetTest.error(()->cs.newDecoder().decode(ByteBuffer.wrap(data)));
            }
            byte[] data={9,65,66,67,8};chars(new String(data,1,3,cs));
            CharsetTest.error(()->new String(data,-1,3,cs));
            CharsetTest.error(()->new String(data,1,Integer.MAX_VALUE,cs));
            CharsetTest.error(()->cs.newEncoder().encode(CharBuffer.wrap("\ud800")));
            CharsetTest.error(()->cs.newEncoder().replaceWith(new byte[0]));
            CharsetTest.error(()->cs.newDecoder().onMalformedInput(null));
        }
        String sample="default\u0000\u4e2d\ud83d\ude00";bytes(sample.getBytes());chars(new String(sample.getBytes()));
        CharsetTest.error(()->sample.getBytes("x-missing-encoding"));
        CharsetTest.error(()->sample.getBytes("bad name"));
        CharsetTest.error(()->sample.getBytes((Charset)null));
        CharsetTest.error(()->new String(new byte[0],(String)null));
        CharsetTest.error(()->new String(new byte[0],-1,0,"x-missing-encoding"));
        CharsetTest.error(()->new String(new byte[0],-1,0,(String)null));
        CharsetTest.error(()->new String(new byte[0],-1,0,(Charset)null));
        for(int i=0;i<30;i++){String value=new String(sample.getBytes(StandardCharsets.UTF_8),StandardCharsets.UTF_8);if(!value.equals(sample))throw new AssertionError();System.gc();}
        System.out.println("GC conversion pass");
    }
}
