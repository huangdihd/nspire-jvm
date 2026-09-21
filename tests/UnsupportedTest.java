public class UnsupportedTest {
    public static void main(String[] args) {
        // Serializable lambda protocol remains unsupported.
        Runnable task = (Runnable & java.io.Serializable) () -> System.out.println("args=" + args.length);
        task.run();
    }
}
