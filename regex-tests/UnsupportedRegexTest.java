import java.util.regex.Pattern;
public class UnsupportedRegexTest {
    public static void main(String[] args) {
        if(args[0].equals("normalize"))Pattern.compile("é",Pattern.CANON_EQ);
        else Pattern.compile("\\p{IsLatin}");
    }
}
