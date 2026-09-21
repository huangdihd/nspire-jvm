import access.Parent;
public class PackageTest {
    public static void main(String[] args)throws Exception {
        System.out.println(PackageTest.class.getPackage().getName());
        System.out.println(Parent.class.getPackage().getName());
        System.out.println(Object.class.getPackage().getName());
        System.out.println(String.class.getPackage()==Object.class.getPackage());
        System.out.println(int.class.getPackage()==null);System.out.println(String[].class.getPackage()==null);
        Package saved=Parent.class.getPackage();System.gc();System.out.println(saved==Parent.class.getPackage());
    }
}
