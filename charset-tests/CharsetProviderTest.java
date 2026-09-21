import java.nio.charset.*;
import java.util.*;
public class CharsetProviderTest {
    public static void main(String[] args)throws Exception {
        Charset c=Charset.forName("shift-test");System.out.println(c.name()+":"+c.isRegistered());
        System.out.println(Charset.availableCharsets().get(c.name()).equals(c));
        byte[] bytes="hello\u0000".getBytes(c);System.out.println(Arrays.toString(bytes));System.out.println(new String(bytes,c).length());
        CharsetTest.error(()->"!".getBytes(c));
        final int[] sums=new int[2];Thread[] threads=new Thread[2];
        for(int t=0;t<2;t++){final int id=t;threads[t]=new Thread(()->{for(int i=0;i<10;i++){byte[] encoded="abc".getBytes(c);sums[id]+=encoded[0];if(!new String(encoded,c).equals("abc"))throw new AssertionError();}});threads[t].start();}
        for(Thread t:threads)t.join();System.out.println(Arrays.toString(sums));
    }
}
