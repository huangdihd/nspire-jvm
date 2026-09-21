public class StringSearchTest {
    static class Sequence implements CharSequence {
        public int length(){throw new AssertionError();}
        public char charAt(int i){throw new AssertionError();}
        public CharSequence subSequence(int a,int b){throw new AssertionError();}
        public String toString(){System.gc();return new String("中文");}
    }
    static class Broken extends Sequence {
        public String toString(){throw new IllegalStateException("conversion");}
    }
    static class NullSequence extends Sequence {
        public String toString(){return null;}
    }
    public static void main(String[] args) {
        String[] inputs={"","abcabc","aaaa","a\u0000b\u0000","前中文后中文","\ud801\udc00a\ud801\udc00","\ud800X\udc00"};
        String[] needles={"","a","ab","bc","abcabcabc","\u0000","中文","\ud801\udc00","\ud801","\udc00"};
        int[] offsets={Integer.MIN_VALUE,-1,0,1,2,3,4,8,100,Integer.MAX_VALUE};
        for(String s:inputs)for(String needle:needles) {
            System.out.println(s.contains(needle));System.out.println(s.indexOf(needle));System.out.println(s.lastIndexOf(needle));
            for(int offset:offsets){System.out.println(s.indexOf(needle,offset));System.out.println(s.lastIndexOf(needle,offset));}
        }
        System.out.println("前中文后".contains(new Sequence()));
        System.out.println("abc".contains(new StringBuilder().append("b")));
        try { "abc".contains(null); } catch(NullPointerException expected){System.out.println("null sequence");}
        try { "abc".contains(new Broken()); } catch(IllegalStateException expected){System.out.println(expected.getMessage());}
        try { "abc".contains(new NullSequence()); } catch(NullPointerException expected){System.out.println("null conversion");}
        try { "abc".indexOf((String)null); } catch(NullPointerException expected){System.out.println("null needle");}
    }
}
