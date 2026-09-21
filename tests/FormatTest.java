public class FormatTest {
    static class Named { public String toString(){System.gc();return "named";} }
    static class Broken { public String toString(){throw new IllegalStateException("broken");} }
    static class Empty { public String toString(){return null;} }
    public static void main(String[] args) {
        System.out.println(String.format("Found %s version %s","logback-core","1.5"));
        System.out.println(String.format("%2$s:%s:%<s:%1$s:%s", "A", "B"));
        System.out.println(String.format("[%8.3s][%-8.3s][%.0s]", "abcdef", "abcdef",new Named()));
        System.out.println(String.format("%s/%s/%s",new Named(),null,Integer.valueOf(23)));
        System.out.println(String.format("%s/%s/%<s",(Object[])null));
        System.out.println(String.format("%s/%s",(Object[])new String[2][]));
        System.out.println(String.format("[%4%][%-4%] %%"));
        // Count UTF-16 code units; do not compare host-specific line separators.
        String unicode=String.format("[%5.3s]","中\ud83d\ude00x");
        System.out.println(unicode.length());for(int i=0;i<unicode.length();i++)System.out.println((int)unicode.charAt(i));
        String line=String.format("x%ny");System.out.println(line.charAt(1)=='\r'||line.charAt(1)=='\n');
        try{System.out.println(String.format("%s",new Broken()));}catch(IllegalStateException e){System.out.println("format exception caught");}
        System.out.println(String.format("%s",new Empty()));
        try{String.format((String)null);}catch(NullPointerException e){System.out.println("null format rejected");}
    }
}
