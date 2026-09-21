public class SyncTest {
    static synchronized void locked() { System.out.println("must not silently execute"); }
    public static void main(String[] args) { locked(); }
}
