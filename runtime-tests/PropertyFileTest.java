import java.io.*;
import java.util.*;

public class PropertyFileTest {
    static class Chunks extends ByteArrayInputStream {
        boolean closed;
        Chunks(byte[] bytes){super(bytes);}
        public int read(byte[] out,int off,int len){System.gc();return super.read(out,off,Math.min(len,3));}
        public int read(byte[] out){return read(out,0,out.length);}
        public void close(){closed=true;}
    }
    static byte[] latin(String s){byte[] out=new byte[s.length()];for(int i=0;i<out.length;i++)out[i]=(byte)s.charAt(i);return out;}
    static void units(String s){System.out.println(s.length());for(int i=0;i<s.length();i++)System.out.println((int)s.charAt(i));}
    public static void main(String[] args)throws Exception {
        Properties defaults=new Properties();defaults.setProperty("default","inherited");defaults.setProperty("override","old");
        Properties p=new Properties(defaults);
        String config=" # comment\r\n! ignored\rkey = first\nkey: second\n"
            +"escaped\\ key\\:\\==a\\tb\\nc\\rd\\fe\\q\n"
            +"continued=hello\\\r\n  world\\\n\t!data\nempty\n=empty-key\n"
            +"latin=é\nunicode=\\u4e2d\\u0000\\ud83d\\ude00\n"
            +"override=new\ntrail=value  \n";
        StringBuilder longLine=new StringBuilder("long=");for(int i=0;i<9000;i++)longLine.append('x');
        Chunks in=new Chunks(latin(config+longLine.toString()));p.load(in);
        System.out.println(in.closed);System.out.println(p.getProperty("key"));
        units(p.getProperty("escaped key:="));System.out.println(p.getProperty("continued"));
        System.out.println(p.getProperty("empty").length());System.out.println(p.getProperty(""));
        units(p.getProperty("latin"));units(p.getProperty("unicode"));
        System.out.println(p.getProperty("default"));System.out.println(p.getProperty("override"));
        System.out.println(p.getProperty("missing","fallback"));System.out.println(p.getProperty("trail").length());
        System.out.println(p.getProperty("long").length());
        byte[] utf8={107,61,(byte)0xe4,(byte)0xb8,(byte)0xad,10};
        Properties chars=new Properties();Reader reader=new InputStreamReader(new ByteArrayInputStream(utf8),"UTF-8");
        chars.load(reader);units(chars.getProperty("k"));System.out.println(reader.read());
        try{p.load(new ByteArrayInputStream(latin("bad=\\u12xz")));}catch(IllegalArgumentException e){System.out.println("bad escape rejected");}
        try{p.load((InputStream)null);}catch(NullPointerException e){System.out.println("null stream rejected");}
        Properties cloned=(Properties)p.clone();cloned.setProperty("key","changed");System.gc();
        System.out.println(p.getProperty("key"));System.out.println(cloned.getProperty("key"));
        p.put("nonString",Integer.valueOf(7));p.put(Integer.valueOf(8),"notAStringKey");
        Set<String> names=p.stringPropertyNames();System.out.println(names.contains("default"));System.out.println(names.contains("nonString"));
        System.out.println(names.size());System.out.println(p.getProperty("nonString","fallback"));
        // Constructor copies UTF-16 code units and does not retain the array.
        char[] input={'a',0,'中','\ud83d','\ude00','z'};String made=new String(input,1,4);input[2]='x';System.gc();units(made);
        System.out.println(new String().length());System.out.println(new String(made).equals(made));
        try{new String(input,Integer.MAX_VALUE,1);}catch(IndexOutOfBoundsException e){System.out.println("String bounds rejected");}
    }
}
