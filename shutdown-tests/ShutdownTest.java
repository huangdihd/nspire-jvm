import java.io.*;

/** Process-level oracle: the driver checks exit status and the actual disk. */
public class ShutdownTest {
    static final Runtime runtime = Runtime.getRuntime();
    static final Object barrier = new Object();
    static Thread mainThread;
    static String mode;
    static int arrived;
    static volatile boolean workerStarted;

    static void check(boolean value) {
        if (!value) throw new AssertionError("shutdown invariant");
    }
    static void write(String name, String text) {
        try (FileOutputStream out = new FileOutputStream(name)) {
            out.write(text.getBytes("UTF-8"));
        } catch (IOException e) { throw new RuntimeException(e); }
    }
    static void sleep(long millis) {
        try { Thread.sleep(millis); } catch (InterruptedException e) { throw new RuntimeException(e); }
    }
    static void rendezvous() throws InterruptedException {
        synchronized (barrier) {
            arrived++;
            barrier.notifyAll();
            long deadline = System.nanoTime() + 10000000000L;
            while (arrived != 2) {
                check(System.nanoTime() < deadline);
                barrier.wait(100);
            }
        }
    }
    static class Hook extends Thread {
        final int number;
        final byte[] retained = new byte[8192];
        Hook(int number) { super("hook-" + number); this.number = number; retained[8191] = 73; }
        // Registration must use identity, even when Java equals/hashCode agree.
        public boolean equals(Object other) { return other instanceof Hook; }
        public int hashCode() { return 0; }
        public void run() {
            try {
                check(Thread.currentThread() == this);
                check(retained[8191] == 73);
                if (mode.equals("fatal-hook")) String.format("%d", 3);
                if (mode.equals("halt-hook")) {
                    write("halt-start", "yes");
                    runtime.halt(31);
                    write("halt-returned", "bad");
                }
                try { runtime.addShutdownHook(new Thread()); throw new AssertionError(); }
                catch (IllegalStateException expected) {}
                try { runtime.removeShutdownHook(this); throw new AssertionError(); }
                catch (IllegalStateException expected) {}
                if (mode.equals("late-delete")) {
                    new File("delete-me").deleteOnExit();
                } else {
                    check(new File("delete-me").exists());
                }
                if (mode.equals("natural") || mode.equals("uncaught") || mode.equals("exception-hook")) {
                    check(!mainThread.isAlive());
                    mainThread.join();
                    check(new File("worker-finished").exists());
                } else if (mode.equals("exit") || mode.equals("runtime-exit") || mode.equals("worker-exit")) {
                    check(mainThread.isAlive());
                    check(workerStarted);
                }
                if (!mode.equals("late-delete")) rendezvous();
                System.gc();
                write("hook-" + number, "retained=" + retained[8191]);
                if (mode.equals("exception-hook") && number == 1) throw new RuntimeException("hook failure");
            } catch (InterruptedException e) { throw new RuntimeException(e); }
        }
    }
    static void registrationChecks(Thread first) throws InterruptedException {
        try { runtime.addShutdownHook(null); throw new AssertionError(); }
        catch (NullPointerException expected) {}
        try { runtime.removeShutdownHook(null); throw new AssertionError(); }
        catch (NullPointerException expected) {}
        try { runtime.addShutdownHook(first); throw new AssertionError(); }
        catch (IllegalArgumentException expected) {}
        Thread removed = new Thread() { public void run() { write("removed-hook", "bad"); } };
        check(!runtime.removeShutdownHook(removed));
        runtime.addShutdownHook(removed);
        check(runtime.removeShutdownHook(removed));
        check(!runtime.removeShutdownHook(removed));
        Thread live = new Thread() { public void run() { synchronized (barrier) {} } };
        synchronized (barrier) {
            live.start();
            try { runtime.addShutdownHook(live); throw new AssertionError(); }
            catch (IllegalArgumentException expected) {}
        }
        live.join();
    }
    public static void main(String[] args) throws Exception {
        mode = args[0]; mainThread = Thread.currentThread();
        write("delete-me", "temporary");
        if (!mode.equals("late-delete")) {
            new File("delete-me").deleteOnExit();
            File parent = new File("parent"); check(parent.mkdir());
            write("parent/child", "temporary");
            parent.deleteOnExit();
            new File("parent/child").deleteOnExit();
            parent.deleteOnExit(); // Duplicate must not reorder the set.
        }
        Hook first = new Hook(1); runtime.addShutdownHook(first);
        if (!mode.equals("late-delete") && !mode.equals("halt-hook") && !mode.equals("fatal-hook")) {
            Hook second = new Hook(2); second.setDaemon(true); runtime.addShutdownHook(second);
        }
        registrationChecks(first);
        System.gc();
        if (mode.equals("halt")) runtime.halt(23);
        if (mode.equals("fatal")) String.format("%d", 3);
        if (mode.equals("late-delete") || mode.equals("halt-hook") || mode.equals("fatal-hook")) return;
        Thread worker = new Thread() {
            public void run() {
                workerStarted = true;
                if (mode.equals("worker-exit")) System.exit(9);
                if (mode.equals("worker-halt")) runtime.halt(29);
                ShutdownTest.sleep(50);
                if (mode.equals("fatal-worker")) String.format("%d", 3);
                if (mode.equals("exit") || mode.equals("runtime-exit")) {
                    while (true) ShutdownTest.sleep(10);
                }
                write("worker-finished", "yes");
            }
        };
        worker.start();
        if (mode.equals("worker-exit") || mode.equals("worker-halt")) {
            worker.join(); throw new AssertionError("termination returned");
        }
        if (mode.equals("exit") || mode.equals("runtime-exit")) {
            while (!workerStarted) Thread.yield();
            if (mode.equals("exit")) System.exit(7); else runtime.exit(11);
            throw new AssertionError("exit returned");
        }
        if (mode.equals("uncaught")) throw new RuntimeException("main failure");
    }
}
