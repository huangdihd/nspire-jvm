import java.util.stream.Stream;
public class UnsupportedParallelTest {
    public static void main(String[] args) {
        System.out.println(Stream.of("a","b").parallel().anyMatch(s->s.equals("b")));
    }
}
