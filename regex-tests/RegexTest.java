import java.util.*;
import java.util.regex.*;
public class RegexTest {
    static void check(boolean x){if(!x)throw new AssertionError();}
    static class Sequence implements CharSequence {
        String text="a\ud83d\ude00b12";int calls;
        public int length(){return text.length();}
        public char charAt(int i){calls++;System.gc();return text.charAt(i);}
        public CharSequence subSequence(int b,int e){return text.substring(b,e);}
        public String toString(){return text;}
    }
    public static void main(String[] args) {
        Matcher m=Pattern.compile("(?<word>[a-z]+)-(\\d+)",Pattern.CASE_INSENSITIVE).matcher("xx ABC-123 yy z-4");
        check(m.find()&&m.start()==3&&m.end()==10&&m.groupCount()==2&&m.group("word").equals("ABC")&&m.group(2).equals("123"));
        MatchResult saved=m.toMatchResult();check(m.find()&&m.group().equals("z-4")&&!m.find());check(saved.group().equals("ABC-123"));
        check(Pattern.matches("(ab|c)+d?","abcabd"));
        check(Pattern.matches("([a-z]+) \\1","hello hello"));
        check(Pattern.compile("(?<=a)b(?=c)").matcher("abc").find());
        check(!Pattern.compile("(?<!a)b(?!c)").matcher("abc").find());
        check(Pattern.matches("a.*?b","axxbxxb")&&!Pattern.matches("a.*+b","axxb"));
        check(Pattern.matches("[a-z&&[^aeiou]]+","bcd"));
        check(Pattern.compile("^b$",Pattern.MULTILINE).matcher("a\nb\nc").find());
        check(Pattern.compile("a.b",Pattern.DOTALL).matcher("a\nb").matches());
        check(Pattern.compile("a # comment\n b",Pattern.COMMENTS).matcher("ab").matches());
        check(Pattern.compile("a.b",Pattern.LITERAL).matcher("a.b").matches());
        check(Pattern.matches(Pattern.quote("a\\Eb[.]"),"a\\Eb[.]"));
        check(Pattern.compile("\\p{L}+\\p{Nd}+",Pattern.UNICODE_CHARACTER_CLASS).matcher("中文é١２").matches());
        check(Pattern.compile("\\w+",Pattern.UNICODE_CHARACTER_CLASS).matcher("é中\u0301").matches());
        check(Pattern.compile("é",Pattern.CASE_INSENSITIVE|Pattern.UNICODE_CASE).matcher("É").matches());
        check(Pattern.matches("\\p{javaWhitespace}+"," \t\u2003"));
        check(Pattern.matches("\\p{javaMirrored}","("));
        check(Pattern.matches("\\x{1F600}+","\ud83d\ude00\ud83d\ude00"));
        m=Pattern.compile(".").matcher("a\ud83d\ude00b");check(m.find()&&m.end()==1&&m.find()&&m.start()==1&&m.end()==3);
        m=Pattern.compile("^a$").matcher("zaq").region(1,2);check(m.matches());m.useAnchoringBounds(false);check(!m.matches());
        m=Pattern.compile("(?<=z)a").matcher("zaq").region(1,2);check(!m.find());m.useTransparentBounds(true);check(m.find());
        check(Arrays.equals(Pattern.compile("[,;]+\\s*").split("a, b;;c;",-1),new String[]{"a","b","c",""}));
        check(Arrays.equals("abc".split(""),new String[]{"a","b","c"}));
        check(Arrays.equals("a12b34".split("\\d+",2),new String[]{"a","b34"}));
        check("a1b22".replaceAll("(\\d+)","<$1>").equals("a<1>b<22>"));
        check("a1b22".replaceFirst("\\d+","x").equals("axb22")&&"123".matches("\\d+"));
        check(Pattern.compile("(?<n>\\d+)").matcher("x12y").replaceAll("${n}${n}").equals("x1212y"));
        check(Pattern.compile("x").matcher("x").replaceAll(Matcher.quoteReplacement("$\\")).equals("$\\"));
        StringBuffer out=new StringBuffer();m=Pattern.compile("[ab]").matcher("a-b-c");while(m.find())m.appendReplacement(out,"X");m.appendTail(out);check(out.toString().equals("X-X-c"));
        check(Pattern.compile(",").splitAsStream("a,b,c").count()==3);
        check(Pattern.compile("b").asPredicate().test("abc"));
        Sequence s=new Sequence();m=Pattern.compile("a.b(\\d+)").matcher(s);check(m.matches()&&m.group(1).equals("12")&&s.calls>0);
        try {Pattern.compile("[");throw new AssertionError();}catch(PatternSyntaxException e){check(e.getPattern().equals("[")&&e.getIndex()==0&&e.getMessage().contains("Unclosed character class"));}
        try {Pattern.compile("a").matcher("a").group();throw new AssertionError();}catch(IllegalStateException expected){}
        try {Pattern.compile("a").matcher("a").replaceAll("$");throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {"a".split("[");throw new AssertionError();}catch(PatternSyntaxException expected){}
        try {"a".matches(null);throw new AssertionError();}catch(NullPointerException expected){}
        for(int i=0;i<30;i++){System.gc();check(Pattern.compile("(ab){2,4}").matcher("ababab").matches());}
        System.out.println("regex groups, assertions, flags, Unicode, replacement, streams, errors and GC passed");
    }
}
