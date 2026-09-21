// Generate factual Character API ranges from the host JDK (tested with JDK 17).
// javac -d build/tools tools/GenerateIdentifiers.java
// java -cp build/tools GenerateIdentifiers > src/identifiers.inc
public class GenerateIdentifiers {
    public static void main(String[] args) {
        System.out.println("/* Generated with Java " + System.getProperty("java.version") + " Character APIs. */");
        System.out.println("static const struct { uint32_t lo,hi;unsigned flags; } identifier_ranges[] = {");
        int start=0, flags=0;
        for(int cp=0;cp<=0x110000;cp++) {
            int f=cp==0x110000?0:(Character.isJavaIdentifierStart(cp)?1:0)|(Character.isJavaIdentifierPart(cp)?2:0);
            if(f!=flags) {
                if(flags!=0)System.out.println("    {"+start+","+(cp-1)+","+flags+"},");
                flags=f;start=cp;
            }
        }
        System.out.println("};");
    }
}
