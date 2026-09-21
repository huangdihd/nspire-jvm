import java.time.*;
import java.util.TimeZone;
public class DefaultZoneTest {
    public static void main(String[] args) {
        System.out.println(TimeZone.getDefault().getID());
        System.out.println(ZoneId.systemDefault().getId());
        System.out.println(Instant.parse("2024-01-01T00:00:00Z").atZone(ZoneId.systemDefault()));
        System.out.println(Instant.parse("2024-07-01T00:00:00Z").atZone(ZoneId.systemDefault()));
    }
}
