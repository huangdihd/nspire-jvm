import java.io.InputStream;
import java.net.URLConnection;
import org.xml.sax.Attributes;
import ch.qos.logback.core.ContextBase;
import ch.qos.logback.core.joran.event.*;

/** Component test of the unmodified Logback bundled inside the Xinbot release. */
public class LogbackXmlTest {
    public static void main(String[] args) throws Exception {
        ContextBase context = new ContextBase();
        SaxEventRecorder recorder = new SaxEventRecorder(context);
        URLConnection connection = LogbackXmlTest.class.getResource("/logback.xml").openConnection();
        connection.setUseCaches(false);
        InputStream stream = connection.getInputStream();
        recorder.recordEvents(stream);
        int count = 0;
        for (SaxEvent event : recorder.getSaxEventList()) {
            count++;
            if (event instanceof StartEvent) {
                StartEvent start = (StartEvent)event;
                System.out.println("start " + start.getQName() + " line=" + start.getLocator().getLineNumber());
                Attributes attributes = start.getAttributes();
                for (int i=0;i<attributes.getLength();i++) System.out.println(attributes.getQName(i)+"="+attributes.getValue(i));
            } else if (event instanceof BodyEvent) {
                System.out.println("body " + ((BodyEvent)event).getText());
            } else System.out.println("end " + event.getQName());
            System.gc();
        }
        stream.close();
        System.out.println("events=" + count);
    }
}
