/* Nspire JVM standard provider. MIT license; see project LICENSE.
 * The six codecs and alias data are preserved OpenJDK code/data. */
package sun.nio.cs;
import java.nio.charset.Charset;
import java.nio.charset.spi.CharsetProvider;
import java.util.Arrays;
import java.util.Iterator;

public final class StandardCharsets extends CharsetProvider {
    static final String[] aliases_US_ASCII=NspireAliases.US_ASCII;
    static final String[] aliases_ISO_8859_1=NspireAliases.ISO_8859_1;
    static final String[] aliases_UTF_8=NspireAliases.UTF_8;
    static final String[] aliases_UTF_16=NspireAliases.UTF_16;
    static final String[] aliases_UTF_16BE=NspireAliases.UTF_16BE;
    static final String[] aliases_UTF_16LE=NspireAliases.UTF_16LE;
    private final String[] names={"US-ASCII","ISO-8859-1","UTF-8","UTF-16","UTF-16BE","UTF-16LE"};
    private final String[][] aliases={aliases_US_ASCII,aliases_ISO_8859_1,aliases_UTF_8,aliases_UTF_16,aliases_UTF_16BE,aliases_UTF_16LE};
    private final Charset[] cache=new Charset[6];
    private static boolean matches(String a,String b) {
        if(a.length()!=b.length())return false;
        for(int i=0;i<a.length();i++) {
            char x=a.charAt(i),y=b.charAt(i);
            if(x>='A'&&x<='Z')x=(char)(x+32);
            if(y>='A'&&y<='Z')y=(char)(y+32);
            if(x!=y)return false;
        }
        return true;
    }
    private Charset get(int i) {
        if(cache[i]==null) {
            switch(i) {
                case 0:cache[i]=new US_ASCII();break;
                case 1:cache[i]=new ISO_8859_1();break;
                case 2:cache[i]=new UTF_8();break;
                case 3:cache[i]=new UTF_16();break;
                case 4:cache[i]=new UTF_16BE();break;
                case 5:cache[i]=new UTF_16LE();break;
                default:throw new AssertionError();
            }
        }
        return cache[i];
    }
    public synchronized Charset charsetForName(String name) {
        for(int i=0;i<names.length;i++) {
            if(matches(names[i],name))return get(i);
            for(String alias:aliases[i])if(matches(alias,name))return get(i);
        }
        return null;
    }
    public synchronized Iterator<Charset> charsets() {
        for(int i=0;i<cache.length;i++)get(i);
        return Arrays.asList(cache.clone()).iterator();
    }
}
