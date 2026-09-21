import java.io.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class XmlAbortTest {
    static void parse()throws Exception {
        SAXParserFactory.newInstance().newSAXParser().parse(new InputSource(XmlAbortTest.class.getResourceAsStream("/valid.xml")),new DefaultHandler(){
            public void startElement(String uri,String local,String q,Attributes attrs){
                System.out.println(String.format("%d",1));
            }
        });
    }
    public static void main(String[] args)throws Exception {
        if(args.length==0)parse();
        else {
            SAXParserFactory.newInstance().newSAXParser().parse(new InputSource(XmlAbortTest.class.getResourceAsStream("/valid.xml")),new DefaultHandler(){
                public void startElement(String uri,String local,String q,Attributes attrs)throws SAXException {
                    Thread t=new Thread(){public void run(){try{parse();}catch(Exception e){throw new RuntimeException("parse failed");}}};
                    t.start();try{t.join();}catch(InterruptedException e){throw new SAXException(e);}
                }
            });
        }
    }
}
