public class EnvironmentTest {
    static void check(boolean x){if(!x)throw new AssertionError();}
    public static void main(String[] args) {
        check("env-中-\ud83d\ude00".equals(System.getenv("NSPIRE_JVM_TEST_7311")));
        check("value".equals(System.getenv("NSPIRE_JVM_TEST_中_7311")));
        check("".equals(System.getenv("NSPIRE_JVM_TEST_EMPTY_7311")));
        check(System.getenv("NSPIRE_JVM_TEST_MISSING_7311")==null);
        check(System.getenv("")==null&&System.getenv("NSPIRE_JVM_TEST_7311\u0000suffix")==null&&System.getenv("NSPIRE_JVM_TEST_7311=x")==null);
        System.setProperty("NSPIRE_JVM_TEST_7311","property");System.gc();
        check("env-中-\ud83d\ude00".equals(System.getenv("NSPIRE_JVM_TEST_7311")));
        try {System.getenv(null);throw new AssertionError();}catch(NullPointerException expected){}
        System.out.println("controlled process environment lookup passed");
    }
}
