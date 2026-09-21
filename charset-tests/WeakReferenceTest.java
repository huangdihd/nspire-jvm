import java.lang.ref.WeakReference;
public class WeakReferenceTest {
    static class WithStrongField extends WeakReference<Object> {
        Object strong;
        WithStrongField(Object o){super(o);strong=o;}
    }
    static WeakReference<Object>[] unreachable() {
        Object[] cycle=new Object[1];cycle[0]=cycle;
        return new WeakReference[]{new WeakReference<Object>(cycle),new WeakReference<Object>(cycle)};
    }
    public static void main(String[] args) {
        Object strong=new Object();WeakReference<Object> w=new WeakReference<Object>(strong);
        System.gc();System.out.println(w.get()==strong);System.out.println(w.isEnqueued());System.out.println(w.enqueue());
        w.clear();System.out.println(w.get()==null);System.out.println(strong!=null);
        WeakReference<Object>[] dead=unreachable();for(int i=0;i<8;i++)System.gc();System.out.println(dead[0].get()==null&&dead[1].get()==null);
        WithStrongField subclass=new WithStrongField(new Object());System.gc();System.out.println(subclass.get()==subclass.strong);
        subclass.strong=null;for(int i=0;i<8;i++)System.gc();System.out.println(subclass.get()==null);
        System.out.println(new WeakReference<Object>(null).get()==null);
    }
}
