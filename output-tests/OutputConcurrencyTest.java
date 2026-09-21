import java.io.*;
public class OutputConcurrencyTest {
    static class Yielding extends OutputStream {
        final ByteArrayOutputStream bytes=new ByteArrayOutputStream();
        public void write(int b) {bytes.write(b);Thread.yield();System.gc();}
    }
    public static void main(String[] args)throws Exception {
        Yielding sink=new Yielding();PrintStream p=new PrintStream(sink,false,"UTF-8");
        Thread a=new Thread(()->{for(int i=0;i<40;i++)p.println("AAAA");});
        Thread b=new Thread(()->{for(int i=0;i<40;i++)p.println("BBBB");});
        a.start();b.start();a.join();b.join();p.close();
        byte[] bytes=sink.bytes.toByteArray();int aa=0,bb=0;boolean valid=bytes.length==400;
        for(int i=0;i<bytes.length;i+=5){
            if(i+4>=bytes.length){valid=false;break;}
            byte ch=bytes[i];valid&=(ch=='A'||ch=='B')&&bytes[i+1]==ch&&bytes[i+2]==ch&&bytes[i+3]==ch&&bytes[i+4]=='\n';
            if(ch=='A')aa++;if(ch=='B')bb++;
        }
        System.out.println(valid);System.out.println(aa);System.out.println(bb);
        ByteArrayOutputStream large=new ByteArrayOutputStream();PrintStream text=new PrintStream(large,false,"UTF-8");
        char[] chars=new char[5000];for(int i=0;i<chars.length;i++)chars[i]='x';
        text.print(chars);System.out.println(large.size());System.out.println(text.checkError());
    }
}
