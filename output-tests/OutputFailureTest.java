import java.io.*;
public class OutputFailureTest {
    interface Attempt { void run() throws Exception; }
    static void check(Attempt a) {try{a.run();System.out.println("ok");}catch(Exception e){System.out.println(e.getClass().getName());}}
    static class Fails extends OutputStream {
        int mode, closes, flushes;
        public void write(int b)throws IOException{if(mode==1)throw new IOException();if(mode==2)throw new InterruptedIOException();if(mode==3)throw new IllegalStateException();}
        public void flush()throws IOException{flushes++;if(mode==4)throw new IOException();if(mode==5)throw new InterruptedIOException();}
        public void close()throws IOException{closes++;if(mode==6)throw new IOException();}
    }
    public static void main(String[] args)throws Exception{
        for(int mode=0;mode<=6;mode++){
            Fails sink=new Fails();sink.mode=mode;PrintStream p=new PrintStream(sink,true,"UTF-8");
            check(()->p.write(4));System.out.println(Thread.interrupted());System.out.println(p.checkError());
            check(()->p.write(new byte[]{4,5}));System.out.println(Thread.interrupted());System.out.println(p.checkError());
            check(()->p.print("x"));System.out.println(Thread.interrupted());System.out.println(p.checkError());
            p.close();System.out.println(sink.closes);System.out.println(p.checkError());System.out.println(Thread.interrupted());
        }
        PrintStream p=new PrintStream(new ByteArrayOutputStream());
        check(()->p.write(null));check(()->p.write(null,0,0));check(()->p.write(new byte[1],-1,0));
        check(()->p.write(new byte[1],0,2));check(()->p.print((char[])null));
        check(()->new PrintStream((OutputStream)null));
        check(()->new PrintStream(null,false,"not-a-charset"));
        check(()->new PrintStream(null,false,(String)null));
        check(()->new PrintStream(new ByteArrayOutputStream(),false,"not-a-charset"));
        check(()->new PrintStream(new ByteArrayOutputStream(),false,(String)null));
        check(()->new PrintStream(new ByteArrayOutputStream(),false,"uTf-8"));
        check(()->new PrintStream(new ByteArrayOutputStream(),false,"unicode-1-1-utf-8"));
        Fails sink=new Fails();PrintStream inner=new PrintStream(sink),outer=new PrintStream(inner);
        sink.mode=1;outer.write(2);System.out.println(outer.checkError());
        Fails flushes=new Fails();PrintStream auto=new PrintStream(flushes,true,"UTF-8");
        auto.write(1);System.out.println(flushes.flushes);
        auto.write('\n');System.out.println(flushes.flushes);
        auto.write(new byte[0]);System.out.println(flushes.flushes);
        auto.print("A");System.out.println(flushes.flushes);
        auto.print("\n\n");System.out.println(flushes.flushes);
        auto.print(new char[]{'\n','\n'});System.out.println(flushes.flushes);
        auto.println();System.out.println(flushes.flushes);
        auto.flush();System.out.println(flushes.flushes);
    }
}
