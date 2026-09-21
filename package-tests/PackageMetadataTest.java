public class PackageMetadataTest {
    static void show(Package p) {
        System.out.println(p.getName());
        System.out.println(p.getSpecificationTitle());
        System.out.println(p.getSpecificationVersion());
        System.out.println(p.getSpecificationVendor());
        System.out.println(p.getImplementationTitle());
        System.out.println(p.getImplementationVersion());
        System.out.println(p.getImplementationVendor());
    }
    public static void main(String[] args) throws Exception {
        ClassLoader loader=ClassLoader.getSystemClassLoader();
        Class<?> first=Class.forName(args[0],false,loader);
        Class<?> second=Class.forName(args[1],false,loader);
        Package later=second.getPackage();
        if(later!=first.getPackage())throw new AssertionError("package identity");
        show(later);System.gc();show(first.getPackage());
        show(Class.forName("sample.sub.Third",false,loader).getPackage());
        show(Class.forName("loose.Loose",false,loader).getPackage());
        show(Class.forName("absent.Absent",false,loader).getPackage());
        if(System.getProperty("package-test-initialized")!=null)throw new AssertionError("metadata initialized the class");
    }
}
