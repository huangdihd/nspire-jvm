// Original platform adapter. MIT license; see the repository LICENSE.
package nspire.text;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
public final class StringOperations {
    private StringOperations() {}
    public static String replace(String source,CharSequence target,CharSequence replacement) {
        return Pattern.compile(target.toString(),Pattern.LITERAL).matcher(source)
            .replaceAll(Matcher.quoteReplacement(replacement.toString()));
    }
}
