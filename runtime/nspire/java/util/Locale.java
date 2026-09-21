/* Original project code, MIT license. A small Locale value/default API.
 * Language tags, locale service providers and display names are not implemented. */
package java.util;

public final class Locale implements Cloneable, java.io.Serializable {
    private final String language, country, variant;
    public static final Locale ROOT = new Locale("", "");
    public static final Locale ENGLISH = new Locale("en", "");
    public static final Locale FRENCH = new Locale("fr", "");
    public static final Locale GERMAN = new Locale("de", "");
    public static final Locale ITALIAN = new Locale("it", "");
    public static final Locale JAPANESE = new Locale("ja", "");
    public static final Locale KOREAN = new Locale("ko", "");
    public static final Locale CHINESE = new Locale("zh", "");
    public static final Locale US = new Locale("en", "US");
    public static final Locale UK = new Locale("en", "GB");
    public static final Locale CANADA = new Locale("en", "CA");
    public static final Locale CANADA_FRENCH = new Locale("fr", "CA");
    public static final Locale FRANCE = new Locale("fr", "FR");
    public static final Locale GERMANY = new Locale("de", "DE");
    public static final Locale ITALY = new Locale("it", "IT");
    public static final Locale JAPAN = new Locale("ja", "JP");
    public static final Locale KOREA = new Locale("ko", "KR");
    public static final Locale CHINA = new Locale("zh", "CN");
    public static final Locale PRC = CHINA;
    public static final Locale SIMPLIFIED_CHINESE = CHINA;
    public static final Locale TAIWAN = new Locale("zh", "TW");
    public static final Locale TRADITIONAL_CHINESE = TAIWAN;
    private static Locale defaultLocale = new Locale(
        System.getProperty("user.language", "en"),
        System.getProperty("user.country", ""),
        System.getProperty("user.variant", ""));

    private static String asciiCase(String s, boolean upper) {
        if (s == null) throw new NullPointerException();
        char[] chars = new char[s.length()];
        for (int i = 0; i < chars.length; i++) {
            char ch = s.charAt(i);
            if (upper && ch >= 'a' && ch <= 'z') ch -= 32;
            if (!upper && ch >= 'A' && ch <= 'Z') ch += 32;
            chars[i] = ch;
        }
        return new String(chars);
    }
    public Locale(String language) { this(language, "", ""); }
    public Locale(String language, String country) { this(language, country, ""); }
    public Locale(String language, String country, String variant) {
        String lang = asciiCase(language, false);
        if (lang.equals("iw")) lang = "he";
        if (lang.equals("ji")) lang = "yi";
        if (lang.equals("in")) lang = "id";
        if (variant == null) throw new NullPointerException();
        this.language = lang; this.country = asciiCase(country, true); this.variant = variant;
    }
    public static Locale getDefault() { return defaultLocale; }
    public static synchronized void setDefault(Locale locale) {
        if (locale == null) throw new NullPointerException();
        defaultLocale = locale;
    }
    public String getLanguage() { return language; }
    public String getCountry() { return country; }
    public String getVariant() { return variant; }
    public String getScript() { return ""; }
    public Object clone() {
        try { return super.clone(); }
        catch (CloneNotSupportedException impossible) { throw new AssertionError(); }
    }
    public boolean equals(Object other) {
        if (!(other instanceof Locale)) return false;
        Locale l = (Locale) other;
        return language.equals(l.language) && country.equals(l.country) && variant.equals(l.variant);
    }
    public int hashCode() {
        return ((31 * language.hashCode()) * 31 + country.hashCode()) * 31 + variant.hashCode();
    }
    public String toString() {
        String s = language;
        if (!country.isEmpty() || (!language.isEmpty() && !variant.isEmpty())) s += "_" + country;
        if (!variant.isEmpty() && (!language.isEmpty() || !country.isEmpty())) s += "_" + variant;
        return s;
    }
}
