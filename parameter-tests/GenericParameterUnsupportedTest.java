import java.util.List;
public class GenericParameterUnsupportedTest {
    public static void generic(List<String> value) {}
    public static void main(String[] args) throws Exception {
        java.lang.reflect.Parameter parameter=GenericParameterUnsupportedTest.class.getMethod("generic",List.class).getParameters()[0];
        System.out.println(parameter.getType().getName());
        System.out.println(parameter.getParameterizedType());
    }
}
