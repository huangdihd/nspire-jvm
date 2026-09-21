public class ThreadLifecycleTest {
    static final Object lock = new Object();
    static volatile boolean ready;
    static class JoinsMain extends Thread {
        final Thread parent;
        JoinsMain(Thread parent) { this.parent = parent; }
        public void run() {
            synchronized (lock) { ready = true; lock.notifyAll(); }
            try { parent.join(); } catch (InterruptedException e) { throw new RuntimeException(); }
            System.out.println("main joined");
            System.out.println(parent.isAlive());
        }
    }
    static class Nothing extends Thread { public void run() {} }
    public static void main(String[] args) throws Exception {
        // Thousands of discarded Thread objects must not retain native records.
        for (int i = 0; i < 6000; i++) {
            Thread unused = new Thread();
            if ((i & 15) == 0) System.gc();
        }
        for (int i = 0; i < 200; i++) {
            Thread done = new Nothing(); done.start(); done.join();
            if ((i & 15) == 0) System.gc();
        }
        System.out.println("thread records reclaimed");
        new JoinsMain(Thread.currentThread()).start();
        synchronized (lock) { while (!ready) lock.wait(); }
        // main is terminated before VM shutdown waits for its children.
    }
}
