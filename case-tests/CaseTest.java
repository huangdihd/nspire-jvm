import java.util.Locale;

public class CaseTest {
    static long digest = 1;
    static void add(int value) { digest = digest * 1000003 + value; }
    static void add(String s) { add(s.length()); add(s.hashCode()); }
    static void show(String s) {
        StringBuilder b = new StringBuilder();
        for (int i=0; i<s.length(); i++) b.append((int)s.charAt(i)).append(',');
        System.out.println(b.toString());
    }
    public static void main(String[] args) {
        Locale.setDefault(Locale.ENGLISH);
        String[] values={"", "LogBack/CONFIGURATION", "Straße ﬃ İ ı ſ K", "ΟΣ ΟΣΑ Σ ΑΣ", "AΣ'B AΣ1 BΣ- Σ",
            "I\u0307 I\u0323\u0307 I\u0301\u0307 J\u0301 Į\u0301 Ì Í Ĩ i\u0307 j\u0323\u0307",
            "中文\u0000\ud801\udc00\ud801\udc28\ud800A\udc00", "AΣ\uffffAΣ", "ǅǄǆ ǲǱǳ"};
        Locale[] locales={Locale.ROOT,Locale.ENGLISH,Locale.CHINA,new Locale("tr"),new Locale("az"),new Locale("lt")};
        for(Locale locale:locales) for(String value:values) {
            show(value.toLowerCase(locale));show(value.toUpperCase(locale));
            Locale.setDefault(locale);
            System.out.println(value.toLowerCase().equals(value.toLowerCase(locale)));
            System.out.println(value.toUpperCase().equals(value.toUpperCase(locale)));
        }
        Locale.setDefault(Locale.ROOT);
        String[] pairs={"ABC","abc","İ","i","ı","I","ß","SS","ſ","S","Σ","ς","K","k",
            "\ud801\udc00","\ud801\udc28","\ud800","\ud800","\udc00","\udc01","\ue000","\ud800\udc00"};
        // Comparator promises ordering, not the magnitude of a nonzero result.
        for(String a:pairs)for(String b:pairs){add(a.equalsIgnoreCase(b)?1:0);int c=a.compareToIgnoreCase(b);add(c<0?-1:c>0?1:0);if(args.length>0){show(a);show(b);System.out.println(c);}}
        System.out.println("comparisons="+digest);
        String[] neighbors={"","A","a","Σ","σ","\u0345","\u02b0","\u00ad","\u200d","1",".","-","'","_"," ","中文","\ud801\udc00","\uffff"};
        digest=1;
        for(String a:neighbors)for(String b:neighbors)for(String c:neighbors) {
            String s=a+"Σ"+b+"Σ"+c;
            add(s.toLowerCase(Locale.ROOT));
            if(args.length>0){show(s);show(s.toLowerCase(Locale.ROOT));}
        }
        System.out.println("contexts="+digest);
        digest=1;
        // All code points, including unpaired surrogates, as singletons bounded
        // by spaces. Batches keep the complete Unicode check practical on this VM.
        for(int start=0;start<=0x10ffff;start+=256) {
            char[] chars=new char[768];int n=0;
            for(int cp=start;cp<start+256&&cp<=0x10ffff;cp++) {
                if(cp<0x10000)chars[n++]=(char)cp;
                else { chars[n++]=(char)(0xd800+((cp-0x10000)>>10));chars[n++]=(char)(0xdc00+((cp-0x10000)&1023)); }
                chars[n++]=' ';
            }
            String s=new String(chars,0,n);
            add(s.toLowerCase(Locale.ROOT));add(s.toUpperCase(Locale.ROOT));
        }
        System.out.println("all string code points="+digest);
        digest=1;
        for(int cp=-2;cp<=0x110001;cp++) {
            add(Character.toLowerCase(cp));add(Character.toUpperCase(cp));add(Character.toTitleCase(cp));
            add(Character.isLowerCase(cp)?1:0);add(Character.isUpperCase(cp)?1:0);add(Character.isTitleCase(cp)?1:0);
        }
        System.out.println("all Character code points="+digest);
        String unchanged=new String("no changes 中文");
        System.out.println(unchanged==unchanged.toLowerCase());
        System.out.println(""=="".toUpperCase());
        System.out.println("x".equalsIgnoreCase(null));
        try { "x".toLowerCase(null); } catch(NullPointerException expected) { System.out.println("null locale"); }
        try { "x".compareToIgnoreCase(null); } catch(NullPointerException expected) { System.out.println("null comparison"); }
        try { Locale.setDefault(null); } catch(NullPointerException expected) { System.out.println("null default"); }
        for(String language:new String[]{"EN","TR","iw","in","ji"}) {
            Locale l=new Locale(language,"us","variant");
            System.out.println(l.getLanguage()+"/"+l.getCountry()+"/"+l.getVariant()+"/"+l.toString()+"/"+l.hashCode());
            System.out.println(l.equals(l.clone()));
        }
    }
}
