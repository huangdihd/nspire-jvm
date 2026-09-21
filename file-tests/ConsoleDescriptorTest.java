import java.io.*;

public class ConsoleDescriptorTest {
    static void check(boolean ok) { if(!ok) throw new AssertionError(); }
    public static void main(String[] args) throws Exception {
        if(args[0].equals("input")) {
            // Avoid the JDK's optional System.in read-ahead when mixing aliases.
            FileInputStream raw=new FileInputStream(FileDescriptor.in);
            System.setIn(raw);
            FileInputStream alias=new FileInputStream(FileDescriptor.in);
            check(raw.getFD()==FileDescriptor.in && raw.getFD().valid());
            System.out.println("available "+raw.available());
            check(System.in.read()==0);
            check(alias.read()==255);
            byte[] tail=new byte[8]; int total=0,n;
            while((n=raw.read(tail,0,tail.length))!=-1) {
                for(int i=0;i<n;i++) total+=tail[i]&255;
            }
            check(alias.read()==-1 && alias.available()==0);
            System.out.println("tail sum "+total);
            alias.close(); check(!FileDescriptor.in.valid());
            try { raw.read(); throw new AssertionError(); }
            catch(IOException e) { System.out.println("stdin aliases closed"); }
            System.setIn(new ByteArrayInputStream(new byte[]{42}));
            check(System.in.read()==42 && System.in.read()==-1);
        } else if(args[0].equals("output")) {
            PrintStream report=System.err;
            FileOutputStream raw=new FileOutputStream(FileDescriptor.out);
            raw.write(new byte[]{0,65,(byte)255,10});
            System.out.print("console 😀\n"); System.out.flush();
            raw.close(); check(!FileDescriptor.out.valid());
            System.out.print("must not appear"); check(System.out.checkError());
            report.println("stdout aliases closed");
        } else if(args[0].equals("error")) {
            FileOutputStream raw=new FileOutputStream(FileDescriptor.err);
            System.err.print("error console\n"); System.err.flush(); raw.write(33);
            System.err.close(); check(!FileDescriptor.err.valid());
            try { raw.write(0); throw new AssertionError(); }
            catch(IOException e) { System.out.println("stderr aliases closed"); }
        } else throw new AssertionError();
    }
}
