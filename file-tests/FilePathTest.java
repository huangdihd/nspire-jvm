import java.io.*;
import java.util.Arrays;
public class FilePathTest {
    public static void main(String[] args)throws Exception {
        String search="a😀/\u0000a😀";
        for(int ch:new int[]{'a','/',0,0xd83d,0xde00,0x1f600,-1,0x110000}) {
            System.out.println(search.lastIndexOf(ch));
            for(int from:new int[]{-1,0,1,2,5,6,99})System.out.println(search.indexOf(ch,from)+"|"+search.lastIndexOf(ch,from));
        }
        String[] names={"","/","///","a//b///","./a/../b","a\\b","中文😀/x",".hidden"};
        for(String name:names){File f=new File(name);System.out.println(f.getPath()+"|"+f.getName()+"|"+f.getParent()+"|"+f.isAbsolute()+"|"+f.isHidden());}
        System.out.println(new File("a","/b"));System.out.println(new File("","b"));System.out.println(new File((File)null,"b"));
        System.out.println(new File("a//b/").equals(new File("a/b")));System.out.println(new File("a/b").hashCode());
        System.out.println(new File("a").compareTo(new File("b")));System.out.println(Arrays.toString(File.listRoots()));
        System.out.println(new File("").getAbsolutePath().equals(System.getProperty("user.dir")));
        String cwd=new File(".").getCanonicalPath();
        for(String path:new String[]{".","missing/../x","dir/../data","alias/../data","dangling/../data"}) {
            File f=new File(path);System.out.println(f.getCanonicalPath().substring(cwd.length()));
            System.out.println(f.getCanonicalFile().getPath().equals(f.getCanonicalPath()));
        }
        String old=System.setProperty("user.dir","/example/root");System.out.println(new File("relative").getAbsolutePath());System.setProperty("user.dir",old);
        File nul=new File("data\u0000ignored");System.out.println(nul.exists()+"|"+nul.isDirectory()+"|"+nul.length()+"|"+nul.delete());
        try{nul.getCanonicalPath();}catch(IOException e){System.out.println("nul canonical IOException");}
        try{new File((String)null);}catch(NullPointerException e){System.out.println("null constructor");}
    }
}
