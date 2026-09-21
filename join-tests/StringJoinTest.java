import java.util.*;
public class StringJoinTest {
    static int reads;
    static class Sequence implements CharSequence {
        final String text;
        Sequence(String text) { this.text = text; }
        public int length() { return text.length(); }
        public char charAt(int i) { reads++; System.gc(); return text.charAt(i); }
        public CharSequence subSequence(int a, int b) { return text.substring(a, b); }
        public String toString() { return "converted:" + text; }
    }
    static void show(String s) {
        for (char c : s.toCharArray()) System.out.print((int)c + ",");
        System.out.println();
    }
    interface Action { void run(); }
    static void failure(Action action) {
        try { action.run(); throw new AssertionError(); }
        catch (RuntimeException e) { System.out.println(e.getClass().getName()); }
    }
    public static void main(String[] args) {
        show(String.join(",", new CharSequence[0]));
        show(String.join(",", Collections.<CharSequence>emptyList()));
        show(String.join("", "one", null, "two"));
        show(String.join("\u0000\ud83d\ude80", Arrays.asList("中", null, "", "\ud800")));
        show(String.join(new StringBuilder(" / "), new StringBuilder("a"), "b"));
        Sequence element = new Sequence("abc");
        show(String.join(new Sequence("|"), element, element));
        show(String.join("-", Arrays.asList(element, null, element)));
        System.out.println(reads);
        failure(() -> String.join(null, new CharSequence[0]));
        failure(() -> String.join("x", (CharSequence[])null));
        failure(() -> String.join("x", (Iterable<CharSequence>)null));
        Iterable<CharSequence> bad = () -> { throw new IllegalStateException(); };
        failure(() -> String.join(null, bad)); // Null delimiter checked before iterator.
        failure(() -> String.join("x", bad));
        Iterable<CharSequence> dynamic = () -> new Iterator<CharSequence>() {
            int index;
            public boolean hasNext() { return index < 3; }
            public CharSequence next() { System.gc(); return new StringBuilder("item" + index++); }
        };
        show(String.join("|", dynamic));
    }
}
