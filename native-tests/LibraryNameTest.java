public class LibraryNameTest {
    static void check(boolean ok) { if(!ok) throw new AssertionError(); }
    static void one(String name) {
        String result=System.mapLibraryName(name);
        check(result.equals("lib"+name+".so"));
        System.out.println(result.length()+":"+result.hashCode());
    }
    public static void main(String[] args) {
        for(String name:new String[]{"","jansi","liba.so","a/b","a\\b","汉😀","x\u0000y","\ud800"}) one(name);
        char[] edge=new char[240]; for(int i=0;i<edge.length;i++)edge[i]=(char)(i+0x700);
        one(new String(edge));
        for(int length:new int[]{241,1000}) {
            try { System.mapLibraryName(new String(new char[length]));throw new AssertionError(); }
            catch(IllegalArgumentException e) { System.out.println("length rejected "+length); }
        }
        try { System.mapLibraryName(null);throw new AssertionError(); }
        catch(NullPointerException e) { System.out.println("null rejected"); }
    }
}
