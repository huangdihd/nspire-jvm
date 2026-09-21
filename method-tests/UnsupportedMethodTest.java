public class UnsupportedMethodTest {
    public static void main(String[] args)throws Exception {String.class.getMethod("getBytes").invoke("unsupported");}
}
