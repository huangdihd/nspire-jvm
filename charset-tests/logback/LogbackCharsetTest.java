import java.nio.charset.Charset;
import java.util.Arrays;
import ch.qos.logback.core.LayoutBase;
import ch.qos.logback.core.encoder.LayoutWrappingEncoder;
import ch.qos.logback.core.joran.util.StringToObjectConverter;
public class LogbackCharsetTest {
    public static class Layout extends LayoutBase<String> {public String doLayout(String s){return "log:"+s;}public String getFileHeader(){return "header";}}
    public static void main(String[] args)throws Exception {
        System.out.println(StringToObjectConverter.canBeBuiltFromSimpleString(Charset.class));
        for(String name:new String[]{"UTF-8","UTF-16LE","US-ASCII","ISO-8859-1"}) {
            Charset c=(Charset)StringToObjectConverter.convertArg(null,name,Charset.class);
            LayoutWrappingEncoder<String> encoder=new LayoutWrappingEncoder<String>();encoder.setLayout(new Layout());
            encoder.getClass().getMethod("setCharset",Charset.class).invoke(encoder,c);
            System.out.println(encoder.getCharset().name());System.out.println(Arrays.toString(encoder.encode("\u4e2d\ud83d\ude00")));
        }
    }
}
