import java.io.*;
public class TempDirectoryTest {
    public static void main(String[] args) throws Exception {
        String path=System.getProperty("java.io.tmpdir");
        if(path==null || !path.endsWith("临时😀"))throw new AssertionError(path);
        File directory=new File(path);
        if(!directory.isDirectory())throw new AssertionError();
        File file=new File(directory,"probe");
        FileOutputStream out=new FileOutputStream(file);out.write(42);out.close();
        FileInputStream in=new FileInputStream(file);
        if(in.read()!=42 || in.read()!=-1)throw new AssertionError();in.close();
        if(!file.delete())throw new AssertionError();
        System.out.println("UTF-8 temporary directory, real file I/O and cleanup");
    }
}
