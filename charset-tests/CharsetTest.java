import java.nio.*;
import java.nio.charset.*;
import java.util.*;
public class CharsetTest {
    static final String[] names={"US-ASCII","ISO-8859-1","UTF-8","UTF-16","UTF-16BE","UTF-16LE"};
    interface Op {void run()throws Exception;}
    static void error(Op op){try{op.run();System.out.println("ok");}catch(Exception e){System.out.println(e.getClass().getName());}}
    public static void main(String[] args)throws Exception {
        System.out.println(Charset.defaultCharset().name());
        for(String name:names) {
            Charset c=Charset.forName(name);
            System.out.println(c.name()+":"+c.displayName()+":"+c.displayName(Locale.ROOT)+":"+c.isRegistered()+":"+c.canEncode()+":"+c.hashCode());
            List<String> aliases=new ArrayList<String>(c.aliases());Collections.sort(aliases);
            System.out.println(aliases);
            for(String alias:aliases)if(!Charset.forName(alias).equals(c))throw new AssertionError();
            System.out.println(c.equals(Charset.forName(name.toLowerCase(Locale.ROOT))));
            System.out.println(Charset.availableCharsets().get(name).equals(c));
            for(String other:names)System.out.println(c.contains(Charset.forName(other))+":"+c.compareTo(Charset.forName(other)));
            error(()->c.aliases().add("bad"));
        }
        System.out.println(StandardCharsets.UTF_8==Charset.forName("utf8"));
        for(String name:new String[]{"","-a","a b","a\u0000","U\u017f-ASCII","I\u017fO-8859-1","UTF-\uff18","x-no-such-nspire-charset"}) {
            error(()->Charset.forName(name));error(()->Charset.isSupported(name));
        }
        error(()->Charset.forName(null));
        error(()->Charset.availableCharsets().clear());
    }
}
