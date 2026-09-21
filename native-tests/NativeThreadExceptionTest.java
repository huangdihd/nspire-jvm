public class NativeThreadExceptionTest {
    private static native void missing();
    public static void main(String[] args) throws Exception {
        Thread thread=new Thread() { public void run() { missing(); } };
        thread.start(); thread.join();
        System.out.println("main survived uncaught child linkage error");
    }
}
