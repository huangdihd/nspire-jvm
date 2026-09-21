package svc;
import java.util.*;
public class BadServiceTest {
    public static void main(String[] args) {
        try {
            ServiceLoader<ServiceTest.Greeting> loader=ServiceLoader.load(ServiceTest.Greeting.class);
            loader.iterator().next();System.out.println("ERROR: invalid provider accepted");
        } catch(ServiceConfigurationError e) {
            System.out.println("service error");
            System.out.println(e.getCause()!=null);
            if(e.getCause()!=null)System.out.println(e.getCause().getMessage());
        }
    }
}
