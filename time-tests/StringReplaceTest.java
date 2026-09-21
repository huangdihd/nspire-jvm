public class StringReplaceTest {
    static final class Sequence implements CharSequence {
        String text;String id;Sequence(String id,String text){this.id=id;this.text=text;}
        public int length(){throw new AssertionError("must call toString");}
        public char charAt(int at){throw new AssertionError("must call toString");}
        public CharSequence subSequence(int a,int b){throw new AssertionError("must call toString");}
        public String toString(){System.out.println(id);System.gc();return text;}
    }
    static void units(String value){for(int i=0;i<value.length();i++)System.out.print((int)value.charAt(i)+",");System.out.println();}
    public static void main(String[] args) {
        String[] inputs={"","aaaa","a.b.$\\","x\u0000y","x\ud83d\ude00y","\ud800x\udc00"};
        for(String input:inputs)for(String target:new String[]{"","a","aa",".","$\\","\u0000","\ud83d\ude00","\ud800"})
            for(String replacement:new String[]{"","$\\","Q","\ud83d\ude00"})units(input.replace(target,replacement));
        units("abab".replace(new Sequence("target","ab"),new Sequence("replacement","$1\\")));
        try{"a".replace((CharSequence)null,new Sequence("must not run","x"));}catch(NullPointerException e){System.out.println("null target");}
        try{"a".replace(new Sequence("target before null","a"),(CharSequence)null);}catch(NullPointerException e){System.out.println("null replacement");}
        for(String input:inputs)for(int i=-1;i<=input.length();i++) {
            StringBuilder b=new StringBuilder(input);
            try{System.out.println(b.deleteCharAt(i)==b);units(b.toString());}
            catch(StringIndexOutOfBoundsException expected){System.out.println("invalid deletion index");}
            for(char ch:new char[]{0,'x','\u6c49','\ud800','\udc00'}) {
                b=new StringBuilder(input);
                try{b.setCharAt(i,ch);units(b.toString());}
                catch(StringIndexOutOfBoundsException expected){System.out.println("invalid replacement index");}
            }
        }
    }
}
