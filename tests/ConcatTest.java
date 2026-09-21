public class ConcatTest {
    static class Named { public String toString(){ System.gc(); return "custom"; } }
    static class Broken { public String toString(){ throw new IllegalStateException("broken"); } }
    static class Empty { public String toString(){ return null; } }
    public static void main(String[] args) {
        int n=args.length+7;long big=-9223372036854775807L;
        char c='Q';boolean yes=true;Object absent=null;
        System.out.println("values="+n+":"+big+":"+c+":"+yes+":"+absent);
        System.out.println("object="+new Named()+":"+n);
        System.out.println("\u0001"+n+"\u0002"); // recipe constants containing marker characters
        String s="";for(int i=0;i<100;i++)s=s+i;
        System.out.println(s.length());
        try{System.out.println("broken="+new Broken());}catch(IllegalStateException e){System.out.println("toString exception caught");}
        System.out.println((Object)new Named());
        System.out.println(new StringBuilder().append((Object)new Named()).append((Object)new Empty()));
        System.out.println(String.valueOf((Object)new Empty())==null);
        try{System.out.println((Object)new Broken());}catch(IllegalStateException e){System.out.println("print exception caught");}
    }
}
