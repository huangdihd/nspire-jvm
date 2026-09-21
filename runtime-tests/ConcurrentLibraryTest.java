import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import java.util.concurrent.locks.*;

public class ConcurrentLibraryTest {
    static final AtomicInteger counter = new AtomicInteger();
    static final AtomicLong longCounter = new AtomicLong();
    static final ConcurrentHashMap<String,String> map = new ConcurrentHashMap<String,String>();
    static final LinkedBlockingQueue<String> queue = new LinkedBlockingQueue<String>(1);
    static final ReentrantLock lock = new ReentrantLock();
    static final Condition changed = lock.newCondition();
    static boolean ready;
    static final AtomicBoolean gate = new AtomicBoolean();
    static int guarded;
    static final class BooleanWorker extends Thread {
        public void run(){for(int i=0;i<100;i++){while(!gate.compareAndSet(false,true))Thread.yield();try{int old=guarded;Thread.yield();guarded=old+1;}finally{gate.set(false);}}}
    }
    static final class Worker extends Thread {
        final String prefix;Worker(String prefix){this.prefix=prefix;}
        public void run() { for(int i=0;i<2000;i++) { counter.incrementAndGet();longCounter.incrementAndGet();map.put(prefix+i,"v"+i); if(i%17==0) Thread.yield(); } }
    }
    public static void main(String[] args) throws Exception {
        Thread a=new Worker("a"), b=new Worker("b"); a.start(); b.start(); a.join(); b.join();
        System.out.println(counter.get());
        System.out.println(longCounter.get());System.out.println(map.size());System.out.println(map.get("b1999"));
        System.out.println(counter.compareAndSet(4000, 9));
        System.out.println(counter.getAndAdd(3)); System.out.println(counter.get());
        AtomicReference<String> ref = new AtomicReference<String>("a");
        System.out.println(ref.compareAndSet("a","b")); System.out.println(ref.getAndSet("c"));
        Thread producer = new Thread(new Runnable() {
            public void run() { try { queue.put("first"); queue.put("second"); } catch(InterruptedException e) { throw new RuntimeException(); } }
        });
        producer.start(); System.out.println(queue.take()); System.out.println(queue.take()); producer.join();
        System.out.println(queue.poll(2,TimeUnit.MILLISECONDS)==null);
        Thread waiter = new Thread(new Runnable() {
            public void run() {
                lock.lock();lock.lock();
                try { while(!ready)changed.await(); System.out.println(lock.getHoldCount()); }
                catch(InterruptedException e) { throw new RuntimeException(); }
                finally {lock.unlock();lock.unlock();}
            }
        });
        waiter.start();
        while(!lock.hasQueuedThreads()) { // Use the condition queue handshake instead.
            lock.lock();
            try { if(lock.hasWaiters(changed))break; } finally { lock.unlock(); }
            Thread.yield();
        }
        lock.lock();try{ready=true;changed.signal();}finally{lock.unlock();}
        waiter.join();System.out.println(lock.isLocked());
        Thread me=Thread.currentThread();LockSupport.unpark(me);LockSupport.park();
        Thread.currentThread().interrupt();LockSupport.park();System.out.println(Thread.interrupted());
        Thread x=new BooleanWorker(),y=new BooleanWorker();x.start();y.start();x.join();y.join();System.out.println(guarded);
        System.out.println(gate.getAndSet(true));System.out.println(gate.get());gate.lazySet(false);System.out.println(gate.toString());
    }
}
