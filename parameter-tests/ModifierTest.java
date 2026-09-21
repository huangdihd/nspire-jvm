import java.lang.reflect.Modifier;
public class ModifierTest {
    public static void main(String[] args) {
        System.out.println(Modifier.classModifiers()+":"+Modifier.interfaceModifiers()+":"+Modifier.constructorModifiers()+":"+Modifier.methodModifiers()+":"+Modifier.fieldModifiers()+":"+Modifier.parameterModifiers());
        for(int flags:new int[]{0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,32768,65535,-1}) {
            System.out.println(Modifier.toString(flags));
            System.out.println(Modifier.isPublic(flags)+":"+Modifier.isPrivate(flags)+":"+Modifier.isProtected(flags)+":"+Modifier.isStatic(flags)+":"+Modifier.isFinal(flags)+":"+Modifier.isSynchronized(flags)+":"+Modifier.isVolatile(flags)+":"+Modifier.isTransient(flags)+":"+Modifier.isNative(flags)+":"+Modifier.isInterface(flags)+":"+Modifier.isAbstract(flags)+":"+Modifier.isStrict(flags));
        }
    }
}
