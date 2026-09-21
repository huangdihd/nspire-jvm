import java.io.*;
import java.util.Arrays;
public class PrintStreamTest {
    static class Probe extends PrintStream {
        int writes;
        Probe(OutputStream out) throws Exception { super(out, false, "UTF-8"); }
        public void write(byte[] b, int off, int len) { writes++; super.write(b, off, len); System.gc(); }
        void error(boolean on) { if(on)setError();else clearError(); }
    }
    public static void main(String[] args) throws Exception {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        Probe p = new Probe(bytes);
        p.print("A\0中😀"); p.print('\ud83d'); p.flush(); p.print('\ude03');
        p.print('\ud800'); p.print('X'); p.print('\udc00');
        p.print(new char[]{'Y','\0','\ud83d','\ude00'});
        p.print((String)null); p.print((Object)null); p.print(true); p.print(-42); p.print(1234567890123L);
        p.print(new Object(){public String toString(){System.gc();return "OBJ";}});
        try { p.print(new Object(){public String toString(){return null;}}); }
        catch (NullPointerException e) { System.out.println("null toString rejected"); }
        Appendable append = p; append.append('!').append("ABC",1,3).append(null).append(null,1,3);
        p.append(new StringBuilder("SEQUENCE"), 1, 4);
        p.format("[%s%%]", "format");
        OutputStream out = p; out.write(new byte[]{0, -1, 17}); out.write(0x142); out.flush();
        System.out.println(Arrays.toString(bytes.toByteArray()));
        System.out.println(p.writes>0); System.out.println(p instanceof FilterOutputStream);
        System.out.println(p instanceof Closeable);System.out.println(p instanceof Flushable);
        p.error(true);System.out.println(p.checkError());p.error(false);System.out.println(p.checkError());
        p.print('\ud800'); p.close(); p.close();
        System.out.println(bytes.toByteArray()[bytes.size()-1]); System.out.println(p.checkError());
        int size=bytes.size(); p.print("ignored"); p.write(1);System.out.println(bytes.size()==size);System.out.println(p.checkError());
        try { p.write(null, -1, -1);System.out.println("closed errors swallowed"); } catch(Exception e){System.out.println(e.getClass().getName());}
    }
}
