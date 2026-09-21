public class ThreadFailureTest {
    private static native void missingNative();
    static class Worker extends Thread {
        public void run() { missingNative(); }
    }
    public static void main(String[] args) throws Exception {
        Thread t = new Worker(); t.start(); t.join();
    }
}
