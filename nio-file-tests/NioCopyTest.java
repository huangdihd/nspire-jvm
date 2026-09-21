import java.io.*;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.util.*;

public class NioCopyTest {
    interface Action { void run() throws Exception; }
    static void error(Action action) throws Exception {
        try { action.run();System.out.println("no-error"); }
        catch(Exception e) {
            System.out.println(e.getClass().getName());
            if(e instanceof FileSystemException) System.out.println("file="+((FileSystemException)e).getFile());
        }
    }
    static class Source extends ByteArrayInputStream {
        boolean closed;
        Source(byte[] data) {super(data);}
        public void close() {closed=true;}
    }
    public static void main(String[] args) throws Exception {
        byte[] data=new byte[32791];for(int i=0;i<data.length;i++) data[i]=(byte)(i*73+17);
        Source input=new Source(data);Path p=Paths.get("copied-中文😀.bin");
        System.out.println(Files.copy(input,p));
        System.out.println("input-open="+!input.closed);
        System.out.println(Arrays.equals(data,Files.readAllBytes(p)));
        System.out.println(Files.size(p)+":"+Files.isRegularFile(p)+":"+Files.isDirectory(p));
        BasicFileAttributes attrs=Files.readAttributes(p,BasicFileAttributes.class);
        System.out.println(attrs.size()+":"+attrs.isRegularFile()+":"+attrs.isSymbolicLink());
        System.out.println(Files.readAttributes(p,"basic:size,isDirectory").get("size"));
        error(()->Files.copy(new ByteArrayInputStream(data),p));
        System.out.println(Files.copy(new ByteArrayInputStream(new byte[]{3,2,1}),p,StandardCopyOption.REPLACE_EXISTING));
        System.out.println(Arrays.toString(Files.readAllBytes(p)));
        error(()->Files.copy(new ByteArrayInputStream(data),p,StandardCopyOption.COPY_ATTRIBUTES));
        error(()->Files.copy((InputStream)null,p));
        error(()->Files.copy(new ByteArrayInputStream(data),p,(CopyOption)null));
        error(()->Files.copy(new ByteArrayInputStream(data),Paths.get("missing/target")));
        error(()->Files.copy(new ByteArrayInputStream(data),Paths.get("nonempty"),StandardCopyOption.REPLACE_EXISTING));
        error(()->Files.copy(new ByteArrayInputStream(data),Paths.get("emptydir")));
        System.out.println(Files.copy(new ByteArrayInputStream(new byte[]{8}),Paths.get("emptydir"),StandardCopyOption.REPLACE_EXISTING));
        System.out.println(Files.copy(new ByteArrayInputStream(new byte[]{9}),Paths.get("link"),StandardCopyOption.REPLACE_EXISTING));
        System.out.println(Files.isSymbolicLink(Paths.get("link")));
        System.out.println(Files.isSymbolicLink(Paths.get("dangling")));
        LinkOption[] options={LinkOption.NOFOLLOW_LINKS};
        BasicFileAttributeView view=Files.getFileAttributeView(Paths.get("symlink"),BasicFileAttributeView.class,options);
        options[0]=null;System.out.println("saved-link-option="+view.readAttributes().isSymbolicLink());
        System.out.println(Files.deleteIfExists(Paths.get("dangling")));
        System.out.println(Files.deleteIfExists(Paths.get("absent")));
        error(()->Files.delete(Paths.get("absent")));
        error(()->Files.newInputStream(Paths.get("absent")));
        final InputStream failing=new InputStream() {
            int used;
            public int read() throws IOException {if(used++<9000)return 42;throw new IOException("source failed");}
        };
        error(()->Files.copy(failing,Paths.get("partial")));
        System.out.println("partial="+Files.size(Paths.get("partial")));
        ByteArrayOutputStream output=new ByteArrayOutputStream();
        System.out.println(Files.copy(p,output)+":"+Arrays.toString(output.toByteArray()));
        Files.createDirectory(Paths.get("made"));
        error(()->Files.createDirectory(Paths.get("made")));
        List<String> names=new ArrayList<String>();
        try(DirectoryStream<Path> dir=Files.newDirectoryStream(Paths.get("."),x->x.toString().endsWith(".bin"))) {
            for(Path name:dir)names.add(name.getFileName().toString());
            error(()->dir.iterator());
        }
        Collections.sort(names);System.out.println(names);
        System.out.println(Files.isSameFile(Paths.get("original"),Paths.get("hardlink")));
        System.out.println(Paths.get("original").toRealPath().equals(Paths.get("original").toAbsolutePath()));
        System.out.println(Paths.get("symlink").toRealPath().equals(Paths.get("original").toRealPath()));
        System.out.println(Paths.get("symlink").toRealPath(LinkOption.NOFOLLOW_LINKS).getFileName());
        Files.delete(Paths.get("made"));
    }
}
