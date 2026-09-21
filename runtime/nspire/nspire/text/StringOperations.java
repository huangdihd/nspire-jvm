// Original platform adapter. MIT license; see the repository LICENSE.
package nspire.text;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.Objects;
import java.util.StringJoiner;
public final class StringOperations {
    private StringOperations() {}
    public static String join(CharSequence delimiter, CharSequence... elements) {
        Objects.requireNonNull(delimiter);
        Objects.requireNonNull(elements);
        StringJoiner result = new StringJoiner(delimiter);
        for (CharSequence element : elements) result.add(element);
        return result.toString();
    }
    public static String join(CharSequence delimiter, Iterable<? extends CharSequence> elements) {
        Objects.requireNonNull(delimiter);
        Objects.requireNonNull(elements);
        StringJoiner result = new StringJoiner(delimiter);
        for (CharSequence element : elements) result.add(element);
        return result.toString();
    }
    public static String replace(String source,CharSequence target,CharSequence replacement) {
        return Pattern.compile(target.toString(),Pattern.LITERAL).matcher(source)
            .replaceAll(Matcher.quoteReplacement(replacement.toString()));
    }
}
