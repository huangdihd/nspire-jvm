public class UnsupportedTest {
    public static void main(String[] args) {
        // StringConcatFactory is supported; LambdaMetafactory remains unsupported.
        Runnable task = () -> System.out.println("args=" + args.length);
        task.run();
    }
}
