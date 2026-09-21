import java.nio.*;
import java.nio.charset.*;
import java.nio.charset.spi.CharsetProvider;
import java.util.*;
public class TestCharsetProvider extends CharsetProvider {
    private static final Charset CS=new Shift();
    public Charset charsetForName(String name){return name.equalsIgnoreCase("x-nspire-shift")||name.equalsIgnoreCase("shift-test")?CS:null;}
    public Iterator<Charset> charsets(){return Collections.singleton(CS).iterator();}
    public static class Shift extends Charset {
        public Shift(){super("x-nspire-shift",new String[]{"shift-test"});}
        public boolean contains(Charset c){return c instanceof Shift;}
        public CharsetDecoder newDecoder(){return new CharsetDecoder(this,1,1){
            protected CoderResult decodeLoop(ByteBuffer in,CharBuffer out){
                System.gc();while(in.hasRemaining()){
                    if(!out.hasRemaining())return CoderResult.OVERFLOW;
                    int b=in.get(in.position())&255;if(b==0||b>128)return CoderResult.unmappableForLength(1);
                    in.get();out.put((char)(b-1));
                }return CoderResult.UNDERFLOW;
            }
        };}
        public CharsetEncoder newEncoder(){return new CharsetEncoder(this,1,1){
            protected CoderResult encodeLoop(CharBuffer in,ByteBuffer out){
                System.gc();while(in.hasRemaining()){
                    if(!out.hasRemaining())return CoderResult.OVERFLOW;
                    char c=in.get(in.position());if(c>127)return CoderResult.unmappableForLength(1);
                    if(c=='!')throw new IllegalStateException("codec callback");
                    in.get();out.put((byte)(c+1));
                }return CoderResult.UNDERFLOW;
            }
        };}
    }
}
