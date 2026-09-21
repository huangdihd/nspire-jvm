import java.lang.reflect.*;
import java.util.function.IntSupplier;

public class NativeLinkageTest {
    public static native int missingStatic();
    public native long missingInstance(Object value);
    public static synchronized native void missingSynchronized();
    private native void missingPrivate();
    static int initialized;
    static class DuringInit {
        static { try { missingStatic(); } catch(UnsatisfiedLinkError e) { initialized=42; } }
        static void touch() {}
    }
    static class Override extends NativeLinkageTest {
        public long missingInstance(Object value) { return 17; }
        void invokeSuper() { super.missingInstance(this); }
    }
    static void check(boolean ok) { if(!ok) throw new AssertionError(); }
    static void caught(UnsatisfiedLinkError error,String method) {
        check(error instanceof LinkageError && error instanceof Error);
        check(error.getMessage().contains(method));
        System.gc();
    }
    public static void main(String[] args) throws Exception {
        NativeLinkageTest object=new NativeLinkageTest();
        try { missingStatic(); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { caught(e,"missingStatic"); }
        try { object.missingPrivate(); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { caught(e,"missingPrivate"); }
        try { object.missingInstance(new Object()); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { caught(e,"missingInstance"); }
        check(new Override().missingInstance(object)==17);
        try { new Override().invokeSuper(); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { caught(e,"missingInstance"); }
        try { missingSynchronized(); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { check(!Thread.holdsLock(NativeLinkageTest.class)); }
        synchronized(NativeLinkageTest.class) {
            try { missingSynchronized(); throw new AssertionError(); }
            catch(UnsatisfiedLinkError e) { check(Thread.holdsLock(NativeLinkageTest.class)); }
        }
        Method method=NativeLinkageTest.class.getMethod("missingInstance",Object.class);
        try { method.invoke(object,new Object()); throw new AssertionError(); }
        catch(InvocationTargetException e) { check(e.getCause() instanceof UnsatisfiedLinkError); }
        IntSupplier lambda=NativeLinkageTest::missingStatic;
        try { lambda.getAsInt(); throw new AssertionError(); }
        catch(UnsatisfiedLinkError e) { caught(e,"missingStatic"); }
        DuringInit.touch(); check(initialized==42);
        for(int i=0;i<200;i++) {
            byte[] retained=new byte[129]; retained[0]=7;
            try { object.missingInstance(retained); throw new AssertionError(); }
            catch(UnsatisfiedLinkError e) { System.gc();check(retained[0]==7); }
        }
        System.out.println("native linkage: virtual/private/super/static, monitors, reflection, lambda, initialization and GC");
    }
}
