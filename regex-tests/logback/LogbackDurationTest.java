import ch.qos.logback.core.util.Duration;
public class LogbackDurationTest {
    public static void main(String[] args) {
        for(String s:new String[]{"0","12","1.5 seconds","2 MINUTES",".25 hour","3 days","42 milliseconds","1 seconde","2.5","1x5 seconds","","garbage","1 month","-2 seconds"}) {
            try {Duration d=Duration.valueOf(s);System.out.println(d.getMilliseconds());System.out.println(d.toString());}
            catch(IllegalArgumentException e){System.out.println(e.getClass().getName());}
            System.gc();
        }
    }
}
