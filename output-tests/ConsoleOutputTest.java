import java.io.*;
public class ConsoleOutputTest {
    public static void main(String[] args)throws Exception{
        PrintStream savedOut=System.out,savedErr=System.err;
        ByteArrayOutputStream captured=new ByteArrayOutputStream();PrintStream capture=new PrintStream(captured,false,"UTF-8");
        System.setOut(capture);System.setErr(savedOut);
        savedOut.write(new byte[]{65,0,66,10});savedErr.write(new byte[]{69,0,70,10});
        System.out.print("captured中");System.err.print("redirected\n");
        System.setOut(savedOut);System.setErr(savedErr);
        System.out.println(captured.size());
        System.out.print("utf8中😀\n");System.err.print("error中😀\n");
        byte[] all=new byte[256];for(int i=0;i<all.length;i++)all[i]=(byte)i;
        savedOut.write(all);savedErr.write(all);
        savedOut.close();savedOut.write(88);savedErr.println(savedOut.checkError());
    }
}
