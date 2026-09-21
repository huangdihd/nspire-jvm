import java.util.*;

public class StackTest {
    public static void main(String[] args) throws Exception {
        Stack<String> stack=new Stack<>();
        System.out.println(stack.empty());
        try { stack.peek(); } catch(EmptyStackException expected){System.out.println("empty peek");}
        try { stack.pop(); } catch(EmptyStackException expected){System.out.println("empty pop");}
        for(int i=0;i<40;i++)stack.push("item"+i);
        stack.push(null);stack.push("item5");
        System.out.println(stack.search("item5"));System.out.println(stack.search(null));System.out.println(stack.search("absent"));
        System.out.println(stack.peek());System.out.println(stack.pop());System.out.println(stack.pop());
        System.gc();System.out.println(stack.pop());
        Stack<String> clone=(Stack<String>)stack.clone();clone.set(0,"changed");
        System.out.println(stack.get(0));System.out.println(clone.get(0));
        Vector<String> vector=new Vector<>(2,3);vector.add("a");vector.add("b");vector.add("c");
        System.out.println(vector.capacity());vector.insertElementAt("middle",1);vector.setSize(6);
        Enumeration<String> e=vector.elements();while(e.hasMoreElements())System.out.println(e.nextElement());
        Iterator<String> it=vector.iterator();vector.add("end");
        try { it.next(); } catch(ConcurrentModificationException expected){System.out.println("fail fast");}
        vector.removeAllElements();System.out.println(vector.isEmpty());
        final Vector<Integer> concurrent=new Vector<>();
        Thread worker=new Thread(new Runnable(){public void run(){for(int i=0;i<100;i++)concurrent.add(i);}});
        worker.start();for(int i=0;i<100;i++)concurrent.add(i);worker.join();
        System.out.println(concurrent.size());
    }
}
