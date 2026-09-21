/* Nspire JVM Unicode charset base. MIT license; see project LICENSE.
 * contains() recognizes this provider's six encodings; custom encodings may
 * conservatively return false, as permitted by Charset.contains. */
package sun.nio.cs;
import java.nio.charset.Charset;
abstract class Unicode extends Charset implements HistoricallyNamedCharset {
    Unicode(String name,String[] aliases) {super(name,aliases);}
    public boolean contains(Charset other) {
        return other instanceof US_ASCII || other instanceof ISO_8859_1 ||
            other instanceof UTF_8 || other instanceof UTF_16 ||
            other instanceof UTF_16BE || other instanceof UTF_16LE;
    }
}
