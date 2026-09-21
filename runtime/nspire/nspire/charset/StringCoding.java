/* Native String's bridge to actual Java charset implementations. MIT license. */
package nspire.charset;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.charset.IllegalCharsetNameException;
import java.nio.charset.UnsupportedCharsetException;

public final class StringCoding {
    private StringCoding() {}
    private static Charset charset(String name)throws UnsupportedEncodingException {
        if(name==null)throw new NullPointerException();
        try {return Charset.forName(name);}
        catch(IllegalCharsetNameException e){throw new UnsupportedEncodingException(name);}
        catch(UnsupportedCharsetException e){throw new UnsupportedEncodingException(name);}
    }
    public static byte[] encode(String value,Charset cs) {
        if(cs==null)throw new NullPointerException();
        ByteBuffer buffer=cs.encode(value);
        byte[] result=new byte[buffer.remaining()];buffer.get(result);return result;
    }
    public static byte[] encode(String value,String name)throws UnsupportedEncodingException {return encode(value,charset(name));}
    public static byte[] encode(String value){return encode(value,Charset.defaultCharset());}
    public static String decode(byte[] bytes,int off,int len,Charset cs) {
        if(cs==null)throw new NullPointerException();
        if(bytes==null)throw new NullPointerException();
        if(off<0||len<0||off>bytes.length-len)throw new StringIndexOutOfBoundsException();
        return cs.decode(ByteBuffer.wrap(bytes,off,len)).toString();
    }
    public static String decode(byte[] bytes,int off,int len,String name)throws UnsupportedEncodingException {
        if(name==null)throw new NullPointerException();
        if(bytes==null)throw new NullPointerException();
        if(off<0||len<0||off>bytes.length-len)throw new StringIndexOutOfBoundsException();
        return decode(bytes,off,len,charset(name));
    }
    public static String decode(byte[] bytes,int off,int len){return decode(bytes,off,len,Charset.defaultCharset());}
}
