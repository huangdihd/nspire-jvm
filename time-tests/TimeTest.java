import java.time.*;
import java.time.format.*;
import java.time.temporal.*;
import java.util.*;
public class TimeTest {
    public static void main(String[] args) {
        System.out.println(ZoneId.systemDefault().getId());
        for(long millis:new long[]{0,-1,1,951782400123L,1710055800987L,1730615400000L,-2208988800000L}) {
            Instant instant=Instant.ofEpochMilli(millis);
            System.out.println(instant.getEpochSecond()+"|"+instant.getNano()+"|"+instant.toEpochMilli());
            for(String id:new String[]{"UTC","Asia/Hong_Kong","America/New_York","Europe/Paris","+05:45"}) {
                DateTimeFormatter f=DateTimeFormatter.ofPattern("uuuu-MM-dd HH:mm:ss.SSS XXX '['VV']'",Locale.US).withZone(ZoneId.of(id));
                System.out.println(f.format(instant));
            }
        }
        for(String text:new String[]{"2000-02-29","1900-02-28","2024-12-31","0000-01-01","-0001-12-31"}) {
            LocalDate d=LocalDate.parse(text);System.out.println(d+"|"+d.toEpochDay()+"|"+d.plusDays(1)+"|"+d.getDayOfWeek());
        }
        ZoneId ny=ZoneId.of("America/New_York");
        ZonedDateTime gap=LocalDateTime.of(2024,3,10,2,30).atZone(ny);
        ZonedDateTime overlap=LocalDateTime.of(2024,11,3,1,30).atZone(ny);
        System.out.println(gap);System.out.println(overlap);System.out.println(overlap.withLaterOffsetAtOverlap());
        System.out.println(Instant.parse("2024-02-29T12:34:56.123456789Z"));
        System.out.println(Duration.between(Instant.ofEpochMilli(-1),Instant.ofEpochMilli(1001)));
        System.out.println(Period.between(LocalDate.of(2020,2,29),LocalDate.of(2024,3,1)));
        try{LocalDate.of(2023,2,29);}catch(DateTimeException e){System.out.println("invalid leap day");}
        try{ZoneId.of("Invalid/Region");}catch(DateTimeException e){System.out.println("invalid zone");}
        try{DateTimeFormatter.ofPattern("HHHH");}catch(IllegalArgumentException e){System.out.println("invalid pattern");}
        System.out.println(ZoneId.getAvailableZoneIds().contains("Asia/Hong_Kong"));
        System.out.println(ZoneOffset.ofHours(18));
        try{ZoneOffset.ofHours(19);}catch(DateTimeException e){System.out.println("invalid offset");}
    }
}
