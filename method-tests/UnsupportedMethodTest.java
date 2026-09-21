public class UnsupportedMethodTest {
    public static void main(String[] args)throws Exception {String.class.getMethod("contentEquals",StringBuffer.class).invoke("unsupported",new StringBuffer());}
}
