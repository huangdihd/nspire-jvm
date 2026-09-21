import java.io.*;
import java.util.Arrays;
public class FileMutationTest {
    static void names(File dir){String[] n=dir.list();Arrays.sort(n);System.out.println(Arrays.toString(n));}
    public static void main(String[] args)throws Exception {
        File dir=new File("created");System.out.println(dir.mkdir()+"|"+dir.mkdir()+"|"+dir.isDirectory());
        File f=new File(dir,"中文😀.txt");System.out.println(f.createNewFile()+"|"+f.createNewFile()+"|"+f.exists()+"|"+f.isFile()+"|"+f.length());
        System.out.println(f.canRead()+"|"+f.canWrite());
        System.out.println(f.setLastModified(1234567890123L)+"|"+f.lastModified());
        try{f.setLastModified(-1);}catch(IllegalArgumentException e){System.out.println("negative timestamp");}
        System.out.println(f.setReadable(true,false)+"|"+f.setWritable(true)+"|"+f.setExecutable(true));
        System.out.println(f.setReadOnly()+"|"+f.setWritable(true));
        System.out.println(f.getTotalSpace()>0);System.out.println(f.getFreeSpace()>0);System.out.println(f.getUsableSpace()>0);
        names(dir);
        File nested=new File(dir,"nested/a");System.out.println(nested.mkdirs()+"|"+nested.mkdirs());
        File dest=new File(dir,"moved.txt");System.out.println(f.renameTo(dest)+"|"+f.exists()+"|"+dest.exists());
        String[] selected=dir.list(new FilenameFilter(){public boolean accept(File parent,String name){System.gc();return parent.equals(new File("created"))&&name.endsWith(".txt");}});
        Arrays.sort(selected);System.out.println(Arrays.toString(selected));
        File[] dirs=dir.listFiles(new FileFilter(){public boolean accept(File f){return f.isDirectory();}});System.out.println(dirs.length+"|"+dirs[0].getName());
        System.out.println(dest.list()==null);System.out.println(new File("missing").list()==null);
        try{dir.list(new FilenameFilter(){public boolean accept(File d,String n){throw new IllegalStateException();}});}catch(IllegalStateException e){System.out.println("filter exception");}
        System.out.println(dir.delete());System.out.println(dest.delete()+"|"+dest.delete());
        System.out.println(nested.delete()+"|"+nested.getParentFile().delete()+"|"+dir.delete());
        try{new File("missing/child").createNewFile();}catch(IOException e){System.out.println("missing parent");}
        System.out.println(new File("missing").getTotalSpace());
    }
}
