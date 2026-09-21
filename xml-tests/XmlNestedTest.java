import java.io.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class XmlNestedTest {
    static int events;
    static void inner()throws Exception{
        XmlTest.parser(true).parse(new InputSource(new XmlTest.Text("<inner>中文\ud83d\ude03</inner>")),new DefaultHandler(){
            public void startElement(String u,String l,String q,Attributes a){events++;System.gc();}
            public void characters(char[] c,int off,int len){events+=len;}
        });
    }
    public static void main(String[] args)throws Exception{
        final SAXParser parser=XmlTest.parser(true);
        parser.parse(new InputSource(new XmlTest.Text("<outer><child/></outer>")),new DefaultHandler(){
            public void startElement(String u,String l,String q,Attributes a)throws SAXException{
                try{
                    try { parser.parse(new InputSource(new XmlTest.Text("<illegal/>")),this); }
                    catch(SAXException e){events++;}
                    inner();
                    Thread t=new Thread(){public void run(){try{inner();}catch(Exception e){throw new RuntimeException("nested parse failed");}}};
                    t.start();t.join();System.gc();
                }catch(Exception e){throw new SAXException(e);}
            }
        });
        System.out.println(events);
        parser.parse(new InputSource(new XmlTest.Text("<reused/>")),new DefaultHandler());
        System.out.println("nested parsers and thread callbacks passed");
    }
}
