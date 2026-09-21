import ch.qos.logback.core.encoder.LayoutWrappingEncoder;
public class LogbackHeaderTest {
    public static void main(String[] args) {
        // Compare the same configured line separator on Windows and Unix.
        System.setProperty("line.separator","\n");
        LayoutWrappingEncoder<String> encoder=new LayoutWrappingEncoder<String>();
        encoder.setLayout(new LogbackCharsetTest.Layout());
        byte[] header=encoder.headerBytes();
        for(byte value:header)System.out.println(value&255);
    }
}
