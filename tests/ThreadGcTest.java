public class ThreadGcTest {
    static volatile boolean held,release;
    static int result;
    static class Holder extends Thread {
        public void run(){int[] local=new int[128];local[127]=739;held=true;while(!release)Thread.yield();result=local[127];}
    }
    public static void main(String[] args)throws Exception {
        Holder h=new Holder();h.start();while(!held)Thread.yield();
        for(int i=0;i<1000;i++){int[] trash=new int[60];trash[0]=i;if(i%20==0)System.gc();}
        release=true;h.join();System.out.println(result);
        Thread t=new Thread(new Runnable(){public void run(){System.out.println("non-daemon completed");}});
        t.start(); // VM must drain non-daemon threads after main returns.
    }
}
