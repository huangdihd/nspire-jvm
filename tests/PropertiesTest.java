public class PropertiesTest {
    public static void main(String[] args) {
        String key="nspire.test.property";
        System.out.println(System.getProperty(key)==null);
        System.out.println(System.getProperty(key,"default"));
        System.out.println(System.setProperty(key,"one")==null);
        System.out.println(System.setProperty(key,"two"));
        System.gc();System.out.println(System.getProperty(key));
        System.out.println(System.clearProperty(key));
        System.out.println(System.clearProperty(key)==null);
        try{System.getProperty("");}catch(IllegalArgumentException e){System.out.println("empty rejected");}
        try{System.setProperty(key,null);}catch(NullPointerException e){System.out.println("null rejected");}
        System.out.println(Boolean.parseBoolean("TrUe"));System.out.println(Boolean.parseBoolean(null));
    }
}
