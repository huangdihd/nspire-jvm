import java.io.*;
public class FileStreamTest {
    static void check(boolean ok){if(!ok)throw new AssertionError();}
    static class Redirect extends File {
        Redirect(){super("absent");}
        public String getPath(){System.gc();return "payload";}
    }
    public static void main(String[] args)throws Exception {
        byte[] data=new byte[13001];for(int i=0;i<data.length;i++)data[i]=(byte)(i*17);
        OutputStream out=new FileOutputStream("payload/");out.write(data);out.close();
        FileOutputStream append=new FileOutputStream(new File("payload"),true);append.write(254);append.write(new byte[]{10,20,30,40},1,2);append.flush();append.close();append.close();
        check(new File("payload").length()==13004);
        InputStream in=new FileInputStream(new Redirect());check(in.available()==13004);check(in.read()==0);
        check(in.skip(9)==9);check(in.skip(-5)==-5);check(in.read()==(data[5]&255));
        try{in.skip(-100);}catch(IOException e){System.out.println("negative seek");}
        check(!in.markSupported());in.mark(10);try{in.reset();}catch(IOException e){System.out.println("reset unsupported");}
        check(in.skip(12995)==12995);check(in.read()==254);check(in.read()==20);check(in.read()==30);check(in.read()==-1);
        check(in.skip(10)==10);check(in.available()==0);in.close();in.close();
        check(in.read(new byte[0])==0);append.write(new byte[0]);
        try{in.read();}catch(IOException e){System.out.println("closed read");}
        try{append.write(1);}catch(IOException e){System.out.println("closed write");}
        try{in.read(null);}catch(NullPointerException e){System.out.println("null read");}
        try{append.write(new byte[2],1,2);}catch(IndexOutOfBoundsException e){System.out.println("write bounds");}
        try{in.read(new byte[2],-1,0);}catch(IndexOutOfBoundsException e){System.out.println("read bounds");}
        in=new FileInputStream("payload");byte[] got=new byte[13004];int total=0,n;while((n=in.read(got,total,got.length-total))>0)total+=n;check(total==got.length);check(in.read()==-1);in.close();
        for(int i=0;i<data.length;i++)check(got[i]==data[i]);System.out.println("contents match");
        try{new FileInputStream("missing");}catch(FileNotFoundException e){System.out.println("missing file");}
        try{new FileInputStream("dir");}catch(FileNotFoundException e){System.out.println("read directory");}
        try{new FileOutputStream("dir");}catch(FileNotFoundException e){System.out.println("write directory");}
        try{new FileInputStream("payload\u0000suffix");}catch(FileNotFoundException e){System.out.println("nul path");}
        try{new FileOutputStream((File)null);}catch(NullPointerException e){System.out.println("null file");}
        FileOutputStream unicode=new FileOutputStream("文字😀.bin");unicode.write(new byte[]{0,-1,42});unicode.close();
        in=new FileInputStream("文字😀.bin");System.out.println(in.read()+"|"+in.read()+"|"+in.read()+"|"+in.read());in.close();
        new FileOutputStream("payload").close();System.out.println(new File("payload").length());
    }
}
