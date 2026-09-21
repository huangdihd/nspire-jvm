package foreign;
import java.lang.reflect.*;
import access.Parent;
public class Child extends Parent {
    public static Object protectedInvoke(Object receiver)throws Exception {
        return Parent.class.getDeclaredMethod("protectedCall").invoke(receiver);
    }
    public static Object protectedStaticInvoke()throws Exception {return Parent.class.getDeclaredMethod("protectedStatic").invoke(null);}
}
