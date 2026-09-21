public class BoxingTest {
    public static void main(String[] args) {
        Integer a=Integer.valueOf(-128),b=Integer.valueOf(127);
        System.out.println(a==Integer.valueOf(-128));System.out.println(b==Integer.valueOf(127));
        System.gc();System.out.println(b==Integer.valueOf(127));
        Object x=new Integer(999), y=new Integer(999);
        System.out.println(x.equals(y));System.out.println(x.equals("999"));System.out.println(x.hashCode());
        System.out.println(x.toString());System.out.println(x);
        Number n=new Integer(-12345);
        System.out.println(n.intValue());System.out.println(n.longValue());
        System.out.println((int)n.shortValue());System.out.println((int)n.byteValue());
        Comparable<Integer> c=new Integer(0);
        System.out.println(c.compareTo(new Integer(Integer.MIN_VALUE)));
        System.out.println(c.compareTo(new Integer(Integer.MAX_VALUE)));
        Boolean yes=Boolean.valueOf(true),no=Boolean.valueOf("false");
        System.gc();System.out.println(yes==Boolean.TRUE);System.out.println(no==Boolean.FALSE);
        Object flag=new Boolean("TrUe");System.out.println(flag);System.out.println(flag.equals(yes));System.out.println(flag.hashCode());
        System.out.println(yes.booleanValue());System.out.println(((Comparable<Boolean>)no).compareTo(yes));
        System.out.println(Boolean.valueOf((String)null)==Boolean.FALSE);
        System.setProperty("test.boolean","TrUe");System.out.println(Boolean.getBoolean("test.boolean"));
        System.clearProperty("test.boolean");System.out.println(Boolean.getBoolean("test.boolean"));
        System.out.println(Boolean.getBoolean(null));System.out.println(Boolean.getBoolean(""));
    }
}
