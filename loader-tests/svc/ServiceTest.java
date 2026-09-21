package svc;
import java.io.*;
import java.net.URL;
import java.util.*;

public class ServiceTest {
    public interface Greeting { String name(); }
    static int initialized,constructed;
    public static class ProviderA implements Greeting {
        static { initialized++; }
        public ProviderA(){ constructed++; }
        public String name(){return "A";}
    }
    public static class ProviderB implements Greeting {
        public ProviderB(){ constructed++; }
        public String name(){return "B";}
    }
    public static class 中文Provider implements Greeting {
        public 中文Provider(){ constructed++; }
        public String name(){return "unicode";}
    }
    public static class Broken implements Greeting {
        public Broken(){throw new IllegalStateException("provider boom");}
        public String name(){return "broken";}
    }
    public static class NotProvider { public NotProvider(){} }
    static String line(InputStream in)throws Exception {
        BufferedReader reader=new BufferedReader(new InputStreamReader(in,"UTF-8"));
        String result=reader.readLine();reader.close();return result;
    }
    public static void main(String[] args)throws Exception {
        ClassLoader loader=ServiceTest.class.getClassLoader();
        System.out.println(loader==ClassLoader.getSystemClassLoader());
        System.out.println(String.class.getClassLoader()==null);
        System.out.println(ProviderA[].class.getClassLoader()==loader);
        System.out.println(int[].class.getClassLoader()==null);
        Class<?> lazy=Class.forName("svc.ServiceTest$ProviderA",false,loader);
        System.out.println(initialized);
        try {Class.forName("svc.ServiceTest",false,null);}
        catch(ClassNotFoundException e){System.out.println("bootstrap cannot see application");}
        System.out.println(Class.forName("java.util.ArrayList",false,null)==ArrayList.class);
        System.out.println(loader.loadClass("svc.ServiceTest$ProviderA")==lazy);
        System.out.println(initialized);
        System.out.println(loader.getResource("missing.txt")==null);
        System.out.println(ServiceTest.class.getResource("absent.txt")==null);
        System.out.println(line(ServiceTest.class.getResourceAsStream("shared.txt")));
        System.out.println(line(ServiceTest.class.getResourceAsStream("/svc/shared.txt")));
        Enumeration<URL> urls=loader.getResources("svc/shared.txt");
        while(urls.hasMoreElements()) {URL u=urls.nextElement();System.gc();System.out.println(line(u.openStream()));}
        try {urls.nextElement();}catch(NoSuchElementException e){System.out.println("enumeration exhausted");}
        ServiceLoader<Greeting> services=ServiceLoader.load(Greeting.class,loader);
        Iterator<Greeting> it=services.iterator();System.out.println(constructed);
        System.out.println(it.hasNext());System.out.println(constructed);
        Greeting first=it.next();System.out.println(first.name());System.out.println(initialized);
        while(it.hasNext())System.out.println(it.next().name());
        System.out.println(constructed);
        System.out.println(services.iterator().next()==first);
        services.reload();System.out.println(services.iterator().next()==first);
        System.out.println(constructed);
        Thread.currentThread().setContextClassLoader(null);
        Thread child=new Thread(new Runnable(){public void run(){System.out.println(Thread.currentThread().getContextClassLoader()==null);}});
        Thread.currentThread().setContextClassLoader(loader);child.start();child.join();
    }
}
