import java.io.File;
public class NativeLoadTest {
    static void call(int mode,String name) {
        switch(mode) {
            case 0:System.load(name);break;
            case 1:Runtime.getRuntime().load(name);break;
            case 2:System.loadLibrary(name);break;
            case 3:Runtime.getRuntime().loadLibrary(name);break;
            default:throw new AssertionError();
        }
    }
    public static void main(String[] args) {
        for(int mode=0;mode<4;mode++) {
            try { call(mode,null);throw new AssertionError(); }
            catch(NullPointerException e) { System.out.println("null "+mode); }
            String[] names=mode<2?new String[]{"relative.so",new File("absent.so").getAbsolutePath(),new File("invalid.so").getAbsolutePath(),"/bad\u0000path"}:
                new String[]{"nspire_missing_library_7262cda","a/b","","bad\u0000name"};
            for(String name:names) {
                try { call(mode,name);throw new AssertionError(); }
                catch(UnsatisfiedLinkError e) { System.out.println("link error "+mode); }
            }
        }
    }
}
