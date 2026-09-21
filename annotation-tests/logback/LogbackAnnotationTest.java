import ch.qos.logback.core.model.processor.PhaseIndicator;
public class LogbackAnnotationTest {
    public static void main(String[] args) throws Exception {
        for(String name:args) {
            Class<?> type=Class.forName(name,false,LogbackAnnotationTest.class.getClassLoader());
            PhaseIndicator annotation=type.getAnnotation(PhaseIndicator.class);
            System.out.println(type.getName()+"="+(annotation==null?"absent":annotation.phase().name()));
            System.gc();
        }
    }
}
