import java.io.*;
import java.util.*;
import javax.xml.parsers.*;
import org.xml.sax.*;
import org.xml.sax.helpers.*;

public class XmlTest {
    static String units(String s) {
        if (s == null) return "null";
        StringBuilder b = new StringBuilder();
        for (int i=0;i<s.length();i++) b.append((int)s.charAt(i)).append(',');
        return b.toString();
    }
    static class Track extends InputStream {
        InputStream delegate; int closes, reads; boolean fail;
        Track(String file) { delegate=XmlTest.class.getResourceAsStream('/'+file); }
        public int read() throws IOException { if(fail && ++reads==64) throw new IOException("read failure"); return delegate.read(); }
        public int read(byte[] b, int off, int len) throws IOException {
            if(len==0)return 0;int c=read();if(c<0)return -1;b[off]=(byte)c;return 1;
        }
        public void close() throws IOException { closes++; delegate.close(); }
    }
    static class Text extends Reader {
        String value; int pos,closes;
        Text(String value) { this.value=value; }
        public int read(){ return pos<value.length()?value.charAt(pos++):-1; }
        public int read(char[] b,int off,int len){ if(len==0)return 0;int c=read();if(c<0)return -1;b[off]=(char)c;return 1; }
        public void close(){closes++;}
    }
    static class Handler extends DefaultHandler {
        Locator locator; StringBuilder body=new StringBuilder(); int starts; boolean stop;
        void flush(){if(body.length()!=0){System.out.println("text "+units(body.toString()));body=new StringBuilder();}}
        public void setDocumentLocator(Locator l){locator=l;}
        public void startDocument(){System.out.println("begin");}
        public void endDocument(){flush();System.out.println("end");}
        public void startPrefixMapping(String prefix,String uri){flush();System.out.println("prefix "+prefix+'='+uri);}
        public void endPrefixMapping(String prefix){flush();System.out.println("unprefix "+prefix);}
        public void startElement(String uri,String local,String q,Attributes attrs)throws SAXException{
            flush();starts++;System.gc();
            System.out.println("start "+uri+'|'+local+'|'+q+" line="+locator.getLineNumber());
            String[] values=new String[attrs.getLength()];
            for(int i=0;i<values.length;i++) values[i]=attrs.getURI(i)+'|'+attrs.getLocalName(i)+'|'+attrs.getQName(i)+'|'+attrs.getType(i)+'|'+units(attrs.getValue(i));
            Arrays.sort(values);for(String value:values)System.out.println("attr "+value);
            if(stop)throw new SAXException("callback failure");
        }
        public void endElement(String uri,String local,String q){flush();System.out.println("stop "+uri+'|'+local+'|'+q);}
        public void characters(char[] b,int off,int n){body.append(new String(b,off,n));}
        public void processingInstruction(String target,String data){flush();System.out.println("pi "+target+'|'+data);}
        public void skippedEntity(String name){flush();System.out.println("skip "+name);}
        public void fatalError(SAXParseException e){System.out.println("fatal "+e.getLineNumber()+" "+(e.getColumnNumber()>0)+" "+e.getSystemId());}
    }
    static SAXParser parser(boolean ns) throws Exception {
        SAXParserFactory f=SAXParserFactory.newInstance();f.setNamespaceAware(ns);f.setValidating(false);
        f.setFeature("http://xml.org/sax/features/external-general-entities",false);
        f.setFeature("http://xml.org/sax/features/external-parameter-entities",false);
        return f.newSAXParser();
    }
    static void parse(SAXParser parser,String file,Handler handler) throws Exception {
        Track stream=new Track(file);InputSource input=new InputSource(stream);input.setSystemId("file:///fixture.xml");
        try { parser.parse(input,handler); }
        catch(SAXException e){System.out.println("caught SAX "+(e instanceof SAXParseException));}
        finally{System.out.println("closed="+(stream.closes>0));}
    }
    public static void main(String[] args)throws Exception{
        SAXParser p=parser(true);System.out.println(p.isNamespaceAware());System.out.println(p.isValidating());
        parse(p,"valid.xml",new Handler());
        parse(parser(false),"plain.xml",new Handler());
        parse(p,"utf16.xml",new Handler());
        parse(p,"malformed.xml",new Handler());
        parse(p,"duplicate.xml",new Handler());
        parse(p,"entities.xml",new Handler());
        Handler stop=new Handler();stop.stop=true;parse(p,"valid.xml",stop);
        parse(p,"plain.xml",new Handler()); // reuse after a Java callback failure
        Text text=new Text("<r>中文\ud83d\ude03 &amp; x</r>");
        InputSource input=new InputSource(text);input.setEncoding("ISO-8859-1");p.parse(input,new Handler());System.out.println("reader closed="+(text.closes>0));
        Track broken=new Track("valid.xml");broken.fail=true;
        try{p.parse(new InputSource(broken),new DefaultHandler());}catch(IOException e){System.out.println(e.getMessage());}
        System.out.println("broken closed="+(broken.closes>0));
        XMLReader reader=p.getXMLReader();reader.setFeature("http://apache.org/xml/features/disallow-doctype-decl",true);
        parse(p,"entities.xml",new Handler());
        try{reader.getFeature("urn:unknown");}catch(SAXNotRecognizedException e){System.out.println("unknown feature");}
        try{reader.setProperty("urn:unknown",null);}catch(SAXNotRecognizedException e){System.out.println("unknown property");}
    }
}
