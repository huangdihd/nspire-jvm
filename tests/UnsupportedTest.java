public class UnsupportedTest {
    private static native void missingNative();
    public static void main(String[] args) {
        missingNative();
    }
}
