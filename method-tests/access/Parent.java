package access;
public class Parent {
    public String visible(){return "public";}
    protected String protectedCall(){return "protected";}
    protected static String protectedStatic(){return "protected-static";}
    String packageCall(){return "package";}
    private String privateCall(){return "private";}
}
