import java.io.*;

public class InputDefaultsTest {
    static void check(boolean ok) { if(!ok) throw new AssertionError(); }
    static class Data extends InputStream {
        int pos;
        final int length, failAt;
        Data(int length,int failAt) { this.length=length; this.failAt=failAt; }
        public int read() throws IOException {
            if(pos==failAt) throw new IOException("read failure");
            if(pos==length) return -1;
            if(pos%511==0) System.gc();
            return (pos++)&255;
        }
    }
    public static void main(String[] args) throws Exception {
        Data data=new Data(5000,-1);
        check(!data.markSupported() && data.available()==0);
        data.mark(10);
        try { data.reset(); throw new AssertionError(); }
        catch(IOException e) { System.out.println(e.getMessage()); }
        check(data.skip(-1)==0 && data.skip(4097)==4097 && data.read()==1);
        check(data.skip(Long.MAX_VALUE)==902 && data.read()==-1);
        check(data.read(new byte[0])==0);
        Data partial=new Data(8,3);
        byte[] buffer=new byte[8];
        check(partial.read(buffer,1,6)==3);
        check(buffer[1]==0 && buffer[2]==1 && buffer[3]==2);
        try { partial.read(buffer); throw new AssertionError(); }
        catch(IOException e) { System.out.println("first read propagates: "+e.getMessage()); }
        Data open=new Data(4,-1); open.close(); check(open.read()==0);
        try { open.read(null); throw new AssertionError(); }
        catch(NullPointerException e) { System.out.println("null buffer"); }
        try { open.read(buffer,Integer.MAX_VALUE,2); throw new AssertionError(); }
        catch(IndexOutOfBoundsException e) { System.out.println("bounds"); }
        System.out.println("default reads, skip, close and partial error semantics");
    }
}
