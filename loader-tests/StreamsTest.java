import java.io.*;
public class StreamsTest {
    public static void main(String[] args)throws Exception {
        byte[] bytes={0,1,127,(byte)128,(byte)255};
        ByteArrayInputStream in=new ByteArrayInputStream(bytes,1,4);
        bytes[1]=42;System.gc(); // the stream must retain and share the backing array
        System.out.println(in.available());System.out.println(in.read());in.mark(100);
        byte[] out=new byte[6];System.out.println(in.read(out,2,4));
        System.out.println(out[2]);System.out.println(out[3]);System.out.println(out[4]);
        System.out.println(in.read(out,0,0));System.out.println(in.read());
        in.reset();System.out.println(in.read());System.out.println(in.skip(99));
        in.close();in.reset();System.out.println(in.read());
        try{in.read(out,-1,1);}catch(IndexOutOfBoundsException e){System.out.println("bounds checked");}
        // UTF-8 -> UTF-16, including NUL, non-ASCII, a surrogate pair and CRLF.
        byte[] text={65,0,(byte)0xc3,(byte)0xa9,(byte)0xe4,(byte)0xb8,(byte)0xad,
                     (byte)0xf0,(byte)0x9f,(byte)0x98,(byte)0x80,13,10,66,13,67,10,10};
        BufferedReader r=new BufferedReader(new InputStreamReader(new ByteArrayInputStream(text),"utf-8"));
        String line=r.readLine();System.out.println(line.length());
        for(int i=0;i<line.length();i++)System.out.println((int)line.charAt(i));
        System.out.println(line.codePointAt(4));System.out.println(line.substring(2,4).length());
        System.out.println(line.indexOf(0x1f600));
        System.out.println(r.readLine());System.out.println(r.readLine());System.out.println(r.readLine().length());
        System.out.println(r.readLine()==null);r.close();r.close();
        try{r.readLine();}catch(IOException e){System.out.println("closed reader");}
        System.out.println("  abc\t\r\n".trim());System.out.println("\t ".trim().length());
        System.out.println(Character.isJavaIdentifierStart('中'));System.out.println(Character.isJavaIdentifierPart(0x0301));
        System.out.println(Character.isJavaIdentifierStart(0x1f600));
    }
}
