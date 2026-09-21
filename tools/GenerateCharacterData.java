// Original project code (MIT). Data is generated from the pinned open-source
// Temurin 17 runtime; see vendor/openjdk17-casing/NOTICE and SOURCES.json.
import java.io.PrintWriter;
public class GenerateCharacterData {
    static int flags(int c) {
        return Character.getType(c)|(Character.isAlphabetic(c)?1<<5:0)|
            (Character.isIdeographic(c)?1<<6:0)|(Character.isLetter(c)?1<<7:0)|
            (Character.isDigit(c)?1<<8:0)|(Character.isLetterOrDigit(c)?1<<9:0)|
            (Character.isWhitespace(c)?1<<10:0)|(Character.isSpaceChar(c)?1<<11:0)|
            (Character.isMirrored(c)?1<<12:0)|(Character.isDefined(c)?1<<13:0)|
            (Character.isUnicodeIdentifierStart(c)?1<<14:0)|(Character.isUnicodeIdentifierPart(c)?1<<15:0)|
            (Character.isIdentifierIgnorable(c)?1<<16:0)|(Character.isISOControl(c)?1<<17:0);
    }
    static void ranges(PrintWriter out,String name,boolean digits) {
        out.println("static const CaseRange "+name+"[] = {");
        int start=0,old=digits?Character.digit(0,36)+1:flags(0);
        for(int c=1;c<=0x110000;c++) {
            int value=c==0x110000?-1:digits?Character.digit(c,36)+1:flags(c);
            if(value!=old){if(old!=0)out.println("{"+start+","+(c-1)+","+old+"},");start=c;old=value;}
        }
        out.println("};");
    }
    public static void main(String[] args) throws Exception {
        if(!System.getProperty("java.runtime.version").equals("17.0.20.1+1")||!System.getProperty("java.vendor").equals("Eclipse Adoptium"))throw new IllegalArgumentException("Use the pinned Temurin runtime");
        try(PrintWriter out=new PrintWriter(args[0],"UTF-8")) {
            out.println("/* Generated from Temurin 17.0.20.1+1; GPLv2 with Classpath exception.");
            out.println(" * See vendor/openjdk17-casing/NOTICE, LICENSE and SOURCES.json. */");
            ranges(out,"character_properties",false);ranges(out,"character_digits",true);
        }
    }
}
