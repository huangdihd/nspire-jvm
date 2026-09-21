public class Demo {
    static int fibonacci(int n) {
        if (n < 2) return n;
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
    public static void main(String[] args) {
        System.out.println("Nspire JVM: Java bytecode is running.");
        System.out.print("fib(12) = ");
        System.out.println(fibonacci(12));
        long x = 1;
        for (int i = 2; i <= 20; i++) x *= i;
        System.out.print("20! = "); System.out.println(x);
        int[] data = new int[8];
        for (int i = 0; i < data.length; i++) data[i] = i * i;
        System.out.print("array[7] = "); System.out.println(data[7]);
        try { int zero = args.length; System.out.println(42 / zero); }
        catch (ArithmeticException e) { System.out.println("ArithmeticException caught."); }
        System.out.println("This is an experimental JVM, not yet a Xinbot runtime.");
    }
}
