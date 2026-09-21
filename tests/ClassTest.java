interface Marker {}
class Parent {}
class Child extends Parent implements Marker {}
class InitCounter { static int value; }
class LazyTarget { static { InitCounter.value++; } }
public class ClassTest {
    static class Inner {}
    public static void main(String[] args) throws Exception {
        Class<?> c=ClassTest.class;
        System.out.println(c.getName());System.out.println(c.getSimpleName());
        System.out.println(c==ClassTest.class);System.out.println(c.getClass()==Class.class);
        System.out.println(new ClassTest().getClass()==c);
        System.out.println(Inner.class.getName());System.out.println(Inner.class.getSimpleName());
        class Local {}
        System.out.println(Local.class.getSimpleName());
        System.out.println(new Object() {}.getClass().getSimpleName());
        Class<?> lazy=LazyTarget.class;System.out.println(InitCounter.value);
        System.out.println(Class.forName("LazyTarget")==lazy);System.out.println(InitCounter.value);
        System.out.println(Class.forName("LazyTarget")==lazy);System.out.println(InitCounter.value);
        System.out.println(Class.forName("java.lang.String")==String.class);
        System.out.println(Class.forName("[[I")==int[][].class);
        System.out.println(Class.forName("[Ljava.lang.String;")==String[].class);
        System.out.println(Object.class.getSuperclass()==null);
        System.out.println(Marker.class.getSuperclass()==null);System.out.println(Marker.class.isInterface());
        System.out.println(Child.class.getSuperclass()==Parent.class);
        System.out.println(Marker.class.isAssignableFrom(Child.class));
        System.out.println(Parent.class.isAssignableFrom(Child.class));
        System.out.println(Child.class.isAssignableFrom(Parent.class));
        System.out.println(Marker.class.isInstance(new Child()));System.out.println(Marker.class.isInstance(null));
        System.out.println(int.class.getName());System.out.println(void.class.getName());
        System.out.println(int.class.isPrimitive());System.out.println(Integer.class.isPrimitive());
        System.out.println(int.class.isAssignableFrom(int.class));System.out.println(long.class.isAssignableFrom(int.class));
        System.out.println(int.class.getSuperclass()==null);System.out.println(void.class.getComponentType()==null);
        System.out.println(int[][].class.getName());System.out.println(int[][].class.getSimpleName());
        System.out.println(Inner[][].class.getSimpleName());
        System.out.println(int[][].class.getComponentType()==int[].class);
        System.out.println(int[].class.getComponentType()==int.class);
        System.out.println(int[].class.isArray());System.out.println(String.class.isArray());
        System.out.println(new int[1].getClass()==int[].class);
        System.out.println(new String[1].getClass()==String[].class);
        System.out.println(Object[].class.isAssignableFrom(String[].class));
        System.out.println(Object[].class.isAssignableFrom(int[].class));
        System.out.println(Object[].class.isAssignableFrom(int[][].class));
        System.out.println(Cloneable.class.isAssignableFrom(int[].class));
        System.out.println(java.io.Serializable.class.isAssignableFrom(String[].class));
        System.out.println(int[].class.getSuperclass()==Object.class);
        System.out.println(int.class.getModifiers());System.out.println(int[].class.getModifiers());
        System.out.println(c.cast(null)==null);
        try { c.cast(new Object()); }catch(ClassCastException e){System.out.println("cast caught");}
        try { c.isAssignableFrom(null); }catch(NullPointerException e){System.out.println("null caught");}
        try { Class.forName("not.present.Anywhere"); }catch(ClassNotFoundException e){System.out.println("missing caught");}
        try { Class.forName("int"); }catch(ClassNotFoundException e){System.out.println("primitive name caught");}
        try { Class.forName("java/lang/String"); }catch(ClassNotFoundException e){System.out.println("slash caught");}
        System.gc();System.out.println(c==ClassTest.class);System.out.println(int[][].class.getComponentType()==int[].class);
    }
}
