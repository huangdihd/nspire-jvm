import java.util.*;
public class CharacterTest {
    static void check(boolean x){if(!x)throw new AssertionError();}
    public static void main(String[] args) {
        String[] strings={"","abc","a\ud83d\ude00b","\ud800x\udc00","\ud800\ud800\udc00\udc00","\u0000中"};
        for(String s:strings) {
            char[] chars=s.toCharArray();
            for(int i=0;i<s.length();i++){System.out.println(Character.codePointAt(s,i));check(Character.codePointAt(chars,i)==Character.codePointAt(s,i));check(Character.codePointAt(chars,i,chars.length)==Character.codePointAt(s,i));}
            for(int i=1;i<=s.length();i++){System.out.println(Character.codePointBefore(s,i));check(Character.codePointBefore(chars,i)==Character.codePointBefore(s,i));check(Character.codePointBefore(chars,i,0)==Character.codePointBefore(s,i));}
            for(int b=0;b<=s.length();b++)for(int e=b;e<=s.length();e++){int n=Character.codePointCount(s,b,e);check(Character.codePointCount(chars,b,e-b)==n);System.out.println(n);}
            for(int i=0;i<=s.length();i++)for(int d=-4;d<=4;d++) {
                try {int n=Character.offsetByCodePoints(s,i,d);check(Character.offsetByCodePoints(chars,0,chars.length,i,d)==n);System.out.println(n);}
                catch(IndexOutOfBoundsException x){System.out.println("bounds");try {Character.offsetByCodePoints(chars,0,chars.length,i,d);throw new AssertionError();}catch(IndexOutOfBoundsException expected){}}
            }
        }
        for(int cp:new int[]{0,65,0xd800,0xffff,0x10000,0x1f600,0x10ffff}) {
            char[] chars=Character.toChars(cp);check(chars.length==Character.charCount(cp));check(Character.codePointAt(chars,0)==cp);
            char[] target=new char[4];check(Character.toChars(cp,target,1)==chars.length);check(target[1]==chars[0]);
        }
        char[] partial=new char[1];try {Character.toChars(0x1f600,partial,-1);throw new AssertionError();}catch(IndexOutOfBoundsException e){System.out.println((int)partial[0]);}
        try {Character.toChars(-1);throw new AssertionError();}catch(IllegalArgumentException expected){}
        try {Character.codePointAt(new char[1],0,2);throw new AssertionError();}catch(IndexOutOfBoundsException expected){}
        StringBuffer b=new StringBuffer("a\ud83d\ude00b");check(b.codePointAt(1)==0x1f600&&b.codePointBefore(3)==0x1f600&&b.codePointCount(0,4)==3&&b.offsetByCodePoints(0,2)==3);
        check(b.reverse().toString().equals("b\ud83d\ude00a"));b.appendCodePoint(0x1f600);check(b.toString().endsWith("\ud83d\ude00"));
        char[] target=new char[6];"\u0000中\ud83d\ude00".getChars(0,4,target,1);check(target[1]==0&&target[2]=='中'&&target[3]=='\ud83d');
        System.out.println("UTF-16 traversal and array bounds passed");
    }
}
