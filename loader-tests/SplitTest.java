public class SplitTest {
    static void show(String input,String regex,int limit){
        String[] parts=input.split(regex,limit);System.gc();System.out.println(parts.length);
        for(String s:parts){System.out.print(s.length());System.out.print(":");for(int i=0;i<s.length();i++){System.out.print((int)s.charAt(i));System.out.print(",");}System.out.println();}
    }
    public static void main(String[] args){
        String[] inputs={"","/","//","a/b/","/a//b//","plain","\u0000/a","😀/中文","x中y中"};
        for(String input:inputs)for(int limit:new int[]{0,1,2,3,-1})show(input,"/",limit);
        show("x|y||","\\|",0);show("a.b.","\\.",-1);show("x中y中","中",0);show("x中y中","中",2);
        show("a\u0000b\u0000","\u0000",-1);
        String same="unchanged";System.out.println(same.split("/",0)[0]==same);
        try{same.split(null);}catch(NullPointerException e){System.out.println("null regex");}
    }
}
