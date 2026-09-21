// VM-specific scheduling policy test; Java SE does not prescribe a share ratio.
public class ThreadPriorityScheduleTest {
    static volatile boolean ready,stop;
    static class Worker extends Thread {
        int count;
        public void run() {
            while(!ready) Thread.yield();
            while(!stop) { if(++count==20000) stop=true; }
        }
    }
    public static void main(String[] args) throws Exception {
        Worker low=new Worker(),high=new Worker();
        low.setPriority(1); high.setPriority(10);
        low.start(); high.start(); ready=true;
        low.join(); high.join();
        if(low.count<=0 || high.count<=low.count) throw new AssertionError();
        System.out.println("priority affects CPU share; low priority still progresses");
    }
}
