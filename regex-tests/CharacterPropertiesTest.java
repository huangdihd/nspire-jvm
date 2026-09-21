public class CharacterPropertiesTest {
    public static void main(String[] args) {
        long hash=0;
        for(int c=-1;c<=0x110000;c++) {
            int f=Character.getType(c)|(Character.isAlphabetic(c)?1<<5:0)|(Character.isIdeographic(c)?1<<6:0)|
                (Character.isLetter(c)?1<<7:0)|(Character.isDigit(c)?1<<8:0)|(Character.isLetterOrDigit(c)?1<<9:0)|
                (Character.isWhitespace(c)?1<<10:0)|(Character.isSpaceChar(c)?1<<11:0)|(Character.isMirrored(c)?1<<12:0)|
                (Character.isDefined(c)?1<<13:0)|(Character.isUnicodeIdentifierStart(c)?1<<14:0)|(Character.isUnicodeIdentifierPart(c)?1<<15:0)|
                (Character.isIdentifierIgnorable(c)?1<<16:0)|(Character.isISOControl(c)?1<<17:0);
            hash=hash*31+f;hash=hash*31+Character.digit(c,36);hash=hash*31+Character.digit(c,10);hash=hash*31+Character.digit(c,2);
            if((c&65535)==65535)System.out.println(hash);
        }
        System.out.println(hash);System.out.println("all code point properties passed");
    }
}
