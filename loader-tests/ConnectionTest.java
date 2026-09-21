import java.net.*;
import java.io.*;
import java.util.*;
public class ConnectionTest {
    public static void main(String[] args)throws Exception {
        Enumeration<URL> urls=ConnectionTest.class.getClassLoader().getResources("svc/shared.txt");
        while(urls.hasMoreElements()) {
            URL url=urls.nextElement();URLConnection c=url.openConnection();
            System.out.println(c.getURL()==url);System.out.println(c.getUseCaches());
            System.out.println(c.getDoInput());System.out.println(c.getDoOutput());
            c.setConnectTimeout(30);c.setReadTimeout(40);System.out.println(c.getConnectTimeout()+c.getReadTimeout());
            try{c.setReadTimeout(-1);}catch(IllegalArgumentException e){System.out.println("negative timeout");}
            c.setUseCaches(false);System.out.println(c.getUseCaches());
            if(c instanceof JarURLConnection)System.out.println(((JarURLConnection)c).getEntryName());
            else System.out.println(url.getProtocol());
            InputStream first=c.getInputStream(),second=c.getInputStream();
            System.out.println(url.getProtocol().equals("file")== (first==second));
            System.out.println(first.read());System.out.println(second.read());
            System.out.println(c.getContentLength());System.out.println(c.getContentLengthLong());System.gc();
            try{c.setUseCaches(true);}catch(IllegalStateException e){System.out.println("connected settings rejected");}
            try{c.setDoInput(false);c.setDoOutput(true);System.out.println("changed input/output");}catch(IllegalStateException e){System.out.println("connected input/output rejected");}
            first.close();first.close();
            try{first.read();}catch(IOException e){System.out.println("closed stream rejected");}
            try{System.out.println("reopened="+c.getInputStream().read());}catch(Exception e){System.out.println(e.getClass().getName());}
            second.close();
            URLConnection fresh=url.openConnection();fresh.setDoOutput(true);
            try{fresh.getOutputStream();}catch(UnknownServiceException e){System.out.println("read-only protocol");}
            InputStream in=fresh.getInputStream();System.out.println(in instanceof AutoCloseable);in.close();
            URLConnection another=url.openConnection();InputStream again=another.getInputStream();System.out.println(again.read());again.close();
            URLConnection uncached=url.openConnection();uncached.setUseCaches(false);
            InputStream one=uncached.getInputStream(),two=uncached.getInputStream();one.close();
            try{System.out.println("other="+two.read());}catch(Exception e){System.out.println(e.getClass().getName());}
            try{System.out.println("uncached="+uncached.getInputStream().read());}catch(Exception e){System.out.println(e.getClass().getName());}
            two.close();
        }
    }
}
