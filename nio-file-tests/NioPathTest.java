import java.io.File;
import java.net.URI;
import java.nio.file.*;
import java.util.*;

public class NioPathTest {
    interface Action { void run() throws Exception; }
    static void error(Action action) throws Exception {
        try { action.run();System.out.println("no-error"); }
        catch(Exception e) {System.out.println(e.getClass().getName());}
    }
    public static void main(String[] args) throws Exception {
        FileSystem fs=FileSystems.getDefault();
        System.out.println(fs==FileSystems.getDefault());
        System.out.println(fs.provider().getScheme()+":"+fs.getSeparator()+":"+fs.isOpen()+":"+fs.isReadOnly());
        System.out.println(FileSystems.getFileSystem(URI.create("file:///"))==fs);
        String[] paths={"","/","//","a","a/b/","a//b","a/./b","a/../b","../a/..","/../../a","/a/../..","./.","中/😀/é"," a /b\\c","a.../..","/a/b/c"};
        for(String text:paths) {
            Path p=Paths.get(text);
            System.out.println(p+"|"+p.isAbsolute()+"|"+p.getRoot()+"|"+p.getFileName()+"|"+p.getParent()+"|"+p.getNameCount()+"|"+p.normalize());
            for(Path name:p) System.out.print("["+name+"]");
            System.out.println();
            for(int start=0;start<p.getNameCount();start++) for(int end=start+1;end<=p.getNameCount();end++) System.out.println(p.subpath(start,end));
            System.out.println(p.resolve("child")+"|"+p.resolveSibling("sibling")+"|"+p.resolve("/absolute"));
            System.out.println(p.equals(new File(text).toPath())+"|"+p.toFile().getPath());
            for(String other:paths) {
                Path q=Paths.get(other);
                System.out.println(p.startsWith(q)+":"+p.endsWith(q)+":"+Integer.signum(p.compareTo(q)));
                if(p.isAbsolute()==q.isAbsolute()) System.out.println(p.relativize(q));
            }
        }
        File f=new File("a/b");System.out.println(f.toPath()==f.toPath());
        System.out.println(Paths.get("a","","b","/c"));
        Path uriPath=Paths.get("/a b/中文😀#%.dat");
        System.out.println(Paths.get(uriPath.toUri()).equals(uriPath));
        System.out.println(fs.getPathMatcher("glob:**/*.{jar,tns}").matches(Paths.get("a/b.jar")));
        System.out.println(fs.getPathMatcher("regex:a.*").matches(Paths.get("abc")));
        error(()->Paths.get("a\0b"));error(()->Paths.get("\ud800"));
        error(()->Paths.get("a").relativize(Paths.get("/a")));
        error(()->Paths.get("a").subpath(0,2));
        error(()->Paths.get("/").getName(0));error(()->Paths.get((String)null));
        error(()->new StringBuilder((String)null));System.out.println(new Throwable((String)null).getMessage()==null);
        error(()->FileSystems.getFileSystem(URI.create("file:///not-root")));
        error(()->fs.close());
    }
}
