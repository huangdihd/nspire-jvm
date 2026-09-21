import ch.qos.logback.core.encoder.LayoutWrappingEncoder;
public class LogbackHeaderPendingTest {
    public static void main(String[] args) {
        LayoutWrappingEncoder<String> encoder=new LayoutWrappingEncoder<String>();
        encoder.setLayout(new LogbackCharsetTest.Layout());encoder.headerBytes();
    }
}
