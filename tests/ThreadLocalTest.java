public class ThreadLocalTest {
    static final ThreadLocal<String> plain = new ThreadLocal<String>() {
        protected String initialValue() { return "initial"; }
    };
    static final InheritableThreadLocal<String> inherited = new InheritableThreadLocal<String>() {
        protected String childValue(String parent) {
            // The implementation must not traverse freed entries if callbacks
            // remove parent entries while preparing the child's inherited map.
            remove(); System.gc(); return parent + "-child";
        }
    };
    public static void main(String[] args) throws Exception {
        plain.set("parent"); inherited.set("first");
        Thread child = new Thread(new Runnable() {
            public void run() {
                System.out.println(plain.get());System.out.println(inherited.get());
                plain.set("local");System.gc();System.out.println(plain.get());
                plain.remove();System.out.println(plain.get());
            }
        });
        inherited.set("later");child.start();child.join();
        System.out.println(plain.get());System.out.println(inherited.get());
        for(int i=0;i<2000;i++){ThreadLocal<int[]> local=new ThreadLocal<int[]>();local.set(new int[8]);if(i%8==0)System.gc();}
        System.gc();System.out.println(plain.get());
    }
}
