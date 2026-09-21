public class ThreadTest {
    static final Object lock=new Object();static int count,stage;static boolean ready;
    static synchronized void increment(){count++;Thread.yield();}
    static class Worker extends Thread {
        final int loops;Worker(int n){loops=n;}
        public void run(){for(int i=0;i<loops;i++)increment();}
    }
    static class Waiter implements Runnable {
        public void run() {
            synchronized(lock) {synchronized(lock) {
                ready=true;lock.notifyAll();
                try {while(stage==0)lock.wait();}catch(InterruptedException e){stage=-1;}
                if(!Thread.holdsLock(lock))stage=-2;
                else stage++;
            }}
        }
    }
    static class Sleeper extends Thread {
        volatile boolean entered,caught,cleared;
        public void run(){entered=true;try{Thread.sleep(10000);}catch(InterruptedException e){caught=true;cleared=!isInterrupted();}}
    }
    public static void main(String[] args)throws Exception {
        System.out.println(Thread.currentThread().getName());
        Worker a=new Worker(100),b=new Worker(100);
        a.setName("worker-a");System.out.println(a.getName());System.out.println(a.isAlive());
        a.start();b.start();a.join();b.join();System.out.println(count);System.out.println(a.isAlive());
        try{a.start();}catch(IllegalThreadStateException e){System.out.println("restart caught");}
        Thread waiter=new Thread(new Waiter(),"waiter");waiter.start();
        synchronized(lock){while(!ready)lock.wait();stage=10;lock.notify();}
        waiter.join();System.out.println(stage);
        System.out.println(Thread.holdsLock(lock));
        try{lock.notify();}catch(IllegalMonitorStateException e){System.out.println("notify ownership caught");}
        try{lock.wait(1);}catch(IllegalMonitorStateException e){System.out.println("wait ownership caught");}
        Sleeper s=new Sleeper();s.start();while(!s.entered)Thread.yield();s.interrupt();s.join();
        System.out.println(s.caught);System.out.println(s.cleared);
        Thread.currentThread().interrupt();System.out.println(Thread.interrupted());System.out.println(Thread.interrupted());
        Thread.currentThread().interrupt();
        try{Thread.sleep(1);}catch(InterruptedException e){System.out.println("sleep preinterrupt caught");}
        System.out.println(Thread.currentThread().isInterrupted());
        synchronized(lock){lock.wait(2);System.out.println(Thread.holdsLock(lock));}
        try{Thread.sleep(-1);}catch(IllegalArgumentException e){System.out.println("negative sleep caught");}
        System.gc();System.out.println("threads done");
    }
}
