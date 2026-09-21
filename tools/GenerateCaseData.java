// Original project code (MIT). Run on the pinned open-source JDK 17 runtime.
// The generated JDK word-break tables retain upstream licensing; see CASE-SUPPORT.md.
import java.io.PrintWriter;
import java.lang.reflect.*;
import java.text.BreakIterator;
import java.util.Locale;

public final class GenerateCaseData {
    static PrintWriter out;
    static Object field(Object object, String name) throws Exception {
        Field f=object.getClass().getDeclaredField(name);f.setAccessible(true);return f.get(object);
    }
    static void array(String type,String name,Object values) {
        out.println("static const "+type+" "+name+"[] = {");
        for(int i=0;i<Array.getLength(values);i++) {
            Object v=Array.get(values,i);out.print((v instanceof Boolean?((Boolean)v?1:0):v)+",");
            if(i%24==23)out.println();
        }
        out.println("\n};");
    }
    static void range(String name,int[] values) {
        out.println("static const CaseRange "+name+"[] = {");
        int start=0;
        for(int end=1;end<=values.length;end++)if(end==values.length||values[end]!=values[start]) {
            out.println("{"+start+","+(end-1)+","+values[start]+"},");start=end;
        }
        out.println("};");
    }
    public static void main(String[] args) throws Exception {
        if(Runtime.version().feature()!=17)throw new IllegalArgumentException("Use JDK 17");
        if(!System.getProperty("java.vendor").equals("Eclipse Adoptium"))throw new IllegalArgumentException("Use the pinned open-source Temurin runtime");
        out=new PrintWriter(args[0],"UTF-8");
        out.println("/* Generated from "+System.getProperty("java.runtime.version")+" ("+System.getProperty("java.vendor")+").");
        out.println(" * Do not edit. See vendor/openjdk17-casing/NOTICE and CASE-SUPPORT.md. */");
        out.println("static const CaseMap case_maps[] = {");
        for(int cp=0;cp<=0x10ffff;cp++) {
            int lo=Character.toLowerCase(cp),up=Character.toUpperCase(cp),ti=Character.toTitleCase(cp);
            if(cp!=lo||cp!=up||cp!=ti)out.println("{"+cp+","+lo+","+up+","+ti+"},");
        }
        out.println("};\nstatic const CaseExpansion case_expansions[] = {");
        for(int cp=0;cp<=0x10ffff;cp++) {
            String s=new String(Character.toChars(cp));
            for(int direction=0;direction<2;direction++) {
                String mapped=direction==0?s.toLowerCase(Locale.ROOT):s.toUpperCase(Locale.ROOT);
                int simple=direction==0?Character.toLowerCase(cp):Character.toUpperCase(cp);
                if(!mapped.equals(new String(Character.toChars(simple)))) {
                    if(mapped.length()>3)throw new AssertionError("Mapping too long");
                    out.print("{"+cp+","+direction+","+mapped.length()+",{");
                    for(int i=0;i<3;i++)out.print((i<mapped.length()?(int)mapped.charAt(i):0)+(i==2?"":","));
                    out.println("}},");
                }
            }
        }
        out.println("};");
        Class<?> special=Class.forName("java.lang.ConditionalSpecialCasing");
        Method cased=special.getDeclaredMethod("isCased",int.class);cased.setAccessible(true);
        Method dotted=special.getDeclaredMethod("isSoftDotted",int.class);dotted.setAccessible(true);
        Method combining=Class.forName("sun.text.Normalizer").getDeclaredMethod("getCombiningClass",int.class);
        combining.setAccessible(true);
        BreakIterator word=BreakIterator.getWordInstance(Locale.ROOT);
        if(!word.getClass().getName().equals("sun.text.RuleBasedBreakIterator"))throw new AssertionError(word.getClass());
        Method category=word.getClass().getDeclaredMethod("lookupCategory",int.class);category.setAccessible(true);
        int[] flags=new int[0x110000],classes=new int[0x110000];
        for(int cp=0;cp<=0x10ffff;cp++) {
            int cc=(Integer)combining.invoke(null,cp);
            flags[cp]=((Boolean)cased.invoke(null,cp)?1:0)|((Boolean)dotted.invoke(null,cp)?2:0)|
                (cc==230?4:cc!=0?8:0)|(Character.isLowerCase(cp)?16:0)|
                (Character.isUpperCase(cp)?32:0)|(Character.isTitleCase(cp)?64:0);
            classes[cp]=(Integer)category.invoke(word,cp);
        }
        range("case_properties",flags);range("case_word_categories",classes);
        out.println("static const unsigned case_word_columns="+field(word,"numCategories")+";");
        array("uint16_t","case_word_states",field(word,"stateTable"));
        array("uint16_t","case_word_backwards",field(word,"backwardsStateTable"));
        array("uint8_t","case_word_ends",field(word,"endStates"));
        array("uint8_t","case_word_lookahead",field(word,"lookaheadStates"));
        out.close();
    }
}
