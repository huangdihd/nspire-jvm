public class ThreadFailureTest {
    static class Worker extends Thread {
        // Exercise a real fatal interpreter abort, distinct from Java errors.
        public void run() { String.format("%d",1); }
    }
    public static void main(String[] args) throws Exception {
        Thread t = new Worker(); t.start(); t.join();
    }
}
