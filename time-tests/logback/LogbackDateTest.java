import ch.qos.logback.core.util.CachingDateFormatter;
import java.time.ZoneId;
import java.util.Locale;
public class LogbackDateTest {
    public static void main(String[] args) {
        for(String zone:new String[]{"UTC","Asia/Hong_Kong","America/New_York","+05:45"}) {
            CachingDateFormatter formatter=new CachingDateFormatter("uuuu-MM-dd HH:mm:ss.SSS XXX",ZoneId.of(zone),Locale.US);
            for(long stamp:new long[]{-1,0,951782400123L,1710055800987L,1710055800987L,1730615400000L})System.out.println(formatter.format(stamp));
        }
        CachingDateFormatter local=new CachingDateFormatter("HH:mm:ss");System.out.println(local.format(0));
    }
}
