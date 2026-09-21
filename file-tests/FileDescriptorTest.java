import java.io.*;

public class FileDescriptorTest {
    static void check(boolean ok) { if (!ok) throw new AssertionError(); }
    static class Tracked extends FileOutputStream {
        final String label;
        final boolean fail;
        int closes;
        Tracked(String path, String label) throws IOException {
            super(path); this.label=label; this.fail=false;
        }
        Tracked(FileDescriptor fd, String label, boolean fail) {
            super(fd); this.label=label; this.fail=fail;
        }
        public void close() throws IOException {
            closes++;
            System.gc();
            super.close();
            if (fail) throw new IOException(label);
        }
    }
    static FileDescriptor retained;
    static void retainOnlyDescriptor() throws IOException {
        FileOutputStream owner=new FileOutputStream("retained");
        owner.write(17); retained=owner.getFD();
    }
    public static void main(String[] args) throws Exception {
        FileDescriptor invalid=new FileDescriptor();
        check(!invalid.valid());
        FileInputStream bad=new FileInputStream(invalid);
        check(bad.getFD()==invalid);
        try { bad.read(); throw new AssertionError(); }
        catch(IOException e) { System.out.println("invalid read"); }
        try { invalid.sync(); throw new AssertionError(); }
        catch(SyncFailedException e) { System.out.println("invalid sync"); }
        bad.close();
        try { new FileInputStream((FileDescriptor)null); throw new AssertionError(); }
        catch(NullPointerException e) { System.out.println("null input descriptor"); }
        try { new FileOutputStream((FileDescriptor)null); throw new AssertionError(); }
        catch(NullPointerException e) { System.out.println("null output descriptor"); }

        Tracked first=new Tracked("shared","first");
        FileDescriptor fd=first.getFD();
        Tracked second=new Tracked(fd,"second",false);
        check(fd.valid() && second.getFD()==fd);
        first.write(new byte[]{1,2}); second.write(3); fd.sync();
        first.close(); check(!fd.valid());
        System.out.println("close callbacks "+first.closes+" "+second.closes);
        second.write(new byte[0]);
        try { second.write(4); throw new AssertionError(); }
        catch(IOException e) { System.out.println("shared output closed"); }
        try { fd.sync(); throw new AssertionError(); }
        catch(SyncFailedException e) { System.out.println("closed sync"); }

        FileOutputStream append=new FileOutputStream("shared",true);
        FileOutputStream alias=new FileOutputStream(append.getFD());
        alias.write(4); append.write(5); alias.close();
        FileInputStream a=new FileInputStream("shared");
        FileInputStream b=new FileInputStream(a.getFD());
        check(a.read()==1 && b.read()==2 && a.available()==3);
        check(b.skip(1)==1 && a.read()==4 && b.read()==5 && a.read()==-1);
        b.close(); check(!a.getFD().valid());
        try { a.read(); throw new AssertionError(); }
        catch(IOException e) { System.out.println("shared input closed"); }

        FileOutputStream owner=new FileOutputStream("close-errors");
        FileDescriptor shared=owner.getFD();
        Tracked e1=new Tracked(shared,"first error",true);
        Tracked e2=new Tracked(shared,"second error",true);
        try { owner.close(); throw new AssertionError(); }
        catch(IOException e) {
            check(e.getSuppressed().length==1);
            System.out.println(e.getMessage()+" | "+e.getSuppressed()[0].getMessage());
        }
        check(!shared.valid() && e1.closes==1 && e2.closes==1);
        // A read-only descriptor rejects output, yet still shares ownership.
        FileInputStream readOnly=new FileInputStream("shared");
        FileOutputStream wrongMode=new FileOutputStream(readOnly.getFD());
        try { wrongMode.write(9); throw new AssertionError(); }
        catch(IOException e) { System.out.println("descriptor mode preserved"); }
        wrongMode.close(); check(!readOnly.getFD().valid());

        retainOnlyDescriptor(); System.gc();
        check(retained.valid());
        FileOutputStream afterGc=new FileOutputStream(retained);
        afterGc.write(18); afterGc.close(); check(!retained.valid()); retained=null;
        System.out.println("descriptor remains live without stream local");
    }
}
