// Platform-independent modifier operations. MIT; see repository LICENSE.
package java.lang.reflect;
public class Modifier {
    public static final int PUBLIC=1, PRIVATE=2, PROTECTED=4, STATIC=8, FINAL=16,
        SYNCHRONIZED=32, VOLATILE=64, TRANSIENT=128, NATIVE=256,
        INTERFACE=512, ABSTRACT=1024, STRICT=2048;
    static final int BRIDGE=64, VARARGS=128, SYNTHETIC=4096, ANNOTATION=8192,
        ENUM=16384, MANDATED=32768, ACCESS_MODIFIERS=7;
    public static boolean isPublic(int flags) { return (flags & PUBLIC)!=0; }
    public static boolean isPrivate(int flags) { return (flags & PRIVATE)!=0; }
    public static boolean isProtected(int flags) { return (flags & PROTECTED)!=0; }
    public static boolean isStatic(int flags) { return (flags & STATIC)!=0; }
    public static boolean isFinal(int flags) { return (flags & FINAL)!=0; }
    public static boolean isSynchronized(int flags) { return (flags & SYNCHRONIZED)!=0; }
    public static boolean isVolatile(int flags) { return (flags & VOLATILE)!=0; }
    public static boolean isTransient(int flags) { return (flags & TRANSIENT)!=0; }
    public static boolean isNative(int flags) { return (flags & NATIVE)!=0; }
    public static boolean isInterface(int flags) { return (flags & INTERFACE)!=0; }
    public static boolean isAbstract(int flags) { return (flags & ABSTRACT)!=0; }
    public static boolean isStrict(int flags) { return (flags & STRICT)!=0; }
    static boolean isSynthetic(int flags) { return (flags & SYNTHETIC)!=0; }
    static boolean isMandated(int flags) { return (flags & MANDATED)!=0; }
    public static int classModifiers() { return 3103; }
    public static int interfaceModifiers() { return 3087; }
    public static int constructorModifiers() { return 7; }
    public static int methodModifiers() { return 3391; }
    public static int fieldModifiers() { return 223; }
    public static int parameterModifiers() { return FINAL; }
    public static String toString(int flags) {
        int[] bits={PUBLIC,PROTECTED,PRIVATE,ABSTRACT,STATIC,FINAL,TRANSIENT,VOLATILE,SYNCHRONIZED,NATIVE,STRICT,INTERFACE};
        String[] names={"public","protected","private","abstract","static","final","transient","volatile","synchronized","native","strictfp","interface"};
        StringBuilder out=new StringBuilder();
        for (int i=0;i<bits.length;i++) if ((flags & bits[i])!=0) {
            if(out.length()!=0)out.append(' ');
            out.append(names[i]);
        }
        return out.toString();
    }
}
