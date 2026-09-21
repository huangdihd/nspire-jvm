import java.io.*;
import java.util.Arrays;
import ch.qos.logback.core.joran.spi.ConsoleTarget;
public class LogbackConsoleTest {
    public static void main(String[] args)throws Exception{
        PrintStream savedOut=System.out,savedErr=System.err;
        ByteArrayOutputStream first=new ByteArrayOutputStream(),second=new ByteArrayOutputStream();
        PrintStream a=new PrintStream(first,false,"UTF-8"),b=new PrintStream(second,false,"UTF-8");
        OutputStream out=ConsoleTarget.SystemOut.getStream(),err=ConsoleTarget.SystemErr.getStream();
        System.setOut(a);System.setErr(b);
        out.write(65);out.write(new byte[]{0, -1});out.write(new byte[]{9,10,11},1,1);out.flush();
        err.write(69);err.write(new byte[]{0, -2});err.flush();
        out.close();out.write(66);
        System.setOut(b);out.write(67);
        System.setOut(savedOut);System.setErr(savedErr);
        System.out.println(Arrays.toString(first.toByteArray()));System.out.println(Arrays.toString(second.toByteArray()));
        System.out.println(a.checkError());System.out.println(b.checkError());
    }
}
