public class ThreadFailureTest {
    static class Worker extends Thread {
        public void run() { Runnable r = () -> System.out.println("unsupported lambda"); r.run(); }
    }
    public static void main(String[] args) throws Exception {
        Thread t = new Worker(); t.start(); t.join();
    }
}
