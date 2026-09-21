public class InterfacesTest {
    interface A {}
    interface B extends A {}
    interface C {}
    static class Parent implements A {}
    static class Child extends Parent implements C,B {}
    static class Inherited extends Parent {}
    @interface Marker {}
    static void show(Class<?> type) {
        Class<?>[] interfaces=type.getInterfaces();
        System.out.print(type.getName()+":");
        for(Class<?> item:interfaces)System.out.print(item.getName()+",");
        System.out.println();
        if(interfaces.length>0){interfaces[0]=null;System.gc();System.out.println(type.getInterfaces()[0]!=null);}
    }
    public static void main(String[] args)throws Exception {
        for(Class<?> c:new Class<?>[]{A.class,B.class,Child.class,Inherited.class,Marker.class,Object.class,int.class,void.class,int[].class,Child[][].class,java.io.File.class})show(c);
        Class<?>[] reflected=(Class<?>[])Class.class.getMethod("getInterfaces").invoke(Child.class);
        System.out.println(reflected.length+"|"+reflected[0].getName());
    }
}
