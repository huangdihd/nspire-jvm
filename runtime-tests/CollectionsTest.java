import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class CollectionsTest {
    static final class Key {
        final int id;
        Key(int id) { this.id = id; }
        public int hashCode() { return 7; }
        public boolean equals(Object o) { return o instanceof Key && ((Key)o).id == id; }
    }
    static void check(boolean value) { if (!value) throw new RuntimeException("map invariant failed"); }
    public static void main(String[] args) {
        ConcurrentHashMap<Key,String> map = new ConcurrentHashMap<Key,String>();
        // Deliberate collisions exercise resizing, tree bins and deletion.
        for (int i=0;i<100;i++) map.put(new Key(i), "value" + i);
        for (int i=0;i<100;i++) check(("value"+i).equals(map.get(new Key(i))));
        check(map.putIfAbsent(new Key(50), "changed").equals("value50"));
        check(map.replace(new Key(50), "value50", "changed"));
        check(!map.remove(new Key(50), "wrong"));
        check(map.remove(new Key(50), "changed"));
        System.out.println(map.size());
        int count=0; for (Map.Entry<Key,String> entry : map.entrySet()) count++;
        System.out.println(count);
        map.clear(); System.out.println(map.isEmpty());
        try { map.put(null,"x"); } catch (NullPointerException e) { System.out.println("null key rejected"); }
        try { map.put(new Key(1),null); } catch (NullPointerException e) { System.out.println("null value rejected"); }

        HashMap<String,String> plain = new HashMap<String,String>();
        for(int i=0;i<60;i++) plain.put("key"+i,"value"+i);
        plain.put(null,"null-key");
        check(plain.get(new StringBuilder().append("key").append(17).toString()).equals("value17"));
        System.out.println(plain.size()); System.out.println(plain.remove(null));
        ArrayList<String> list = new ArrayList<String>(plain.values());
        System.out.println(list.size()); System.out.println(list.contains("value30"));
        HashSet<String> set = new HashSet<String>(list);
        System.out.println(set.add("value30")); System.out.println(set.size());
        System.gc(); System.out.println(plain.get("key17"));
    }
}
