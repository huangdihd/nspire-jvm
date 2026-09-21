public class ThreadPriorityTest {
    static void check(boolean value) { if(!value) throw new AssertionError(); }
    static int observed, inherited;
    static class Worker extends Thread {
        public void run() {
            observed=getPriority();
            setPriority(9); inherited=new Thread().getPriority();
        }
    }
    public static void main(String[] args) throws Exception {
        Thread main=Thread.currentThread(); int old=main.getPriority();
        main.setPriority(6); Worker child=new Worker();
        main.setPriority(4); check(child.getPriority()==6);
        child.setPriority(Thread.MIN_PRIORITY); check(child.getPriority()==1);
        child.setPriority(Thread.MAX_PRIORITY); check(child.getPriority()==10);
        for(int bad:new int[]{0,11,Integer.MIN_VALUE,Integer.MAX_VALUE}) {
            try { child.setPriority(bad); throw new AssertionError(); }
            catch(IllegalArgumentException expected) { check(child.getPriority()==10); }
        }
        child.setPriority(7); child.start(); child.join();
        check(observed==7 && inherited==9);
        child.setPriority(1); check(child.getPriority()==9);
        main.setPriority(old);
        System.out.println("priority inheritance, bounds, running updates and terminated thread");
    }
}
