import java.util.*;

public class EnumTest {
    enum Color { RED { public String toString(){return "red override";} }, GREEN, BLUE }
    enum Empty {}
    enum Many {
        V00,V01,V02,V03,V04,V05,V06,V07,V08,V09,V10,V11,V12,V13,V14,V15,
        V16,V17,V18,V19,V20,V21,V22,V23,V24,V25,V26,V27,V28,V29,V30,V31,
        V32,V33,V34,V35,V36,V37,V38,V39,V40,V41,V42,V43,V44,V45,V46,V47,
        V48,V49,V50,V51,V52,V53,V54,V55,V56,V57,V58,V59,V60,V61,V62,V63,V64,V65
    }
    static int initialized;
    enum Init { A; static {initialized++;} }
    public static void main(String[] args) {
        System.out.println(Color.class.isEnum());System.out.println(Color.RED.getClass().isEnum());
        System.out.println(Enum.class.isEnum());System.out.println(String.class.getEnumConstants()==null);
        System.out.println(Color[].class.getEnumConstants()==null);System.out.println(Empty.class.getEnumConstants().length);
        Color[] a=Color.class.getEnumConstants(),b=Color.class.getEnumConstants();
        System.out.println(a!=b);a[0]=null;System.gc();System.out.println(b[0].name());
        System.out.println(Color.class.getEnumConstants()[0].name());
        System.out.println(Color.RED.getDeclaringClass()==Color.class);
        System.out.println(Enum.valueOf(Color.class,"RED")==Color.RED);
        try { Enum.valueOf(Color.class,"red"); } catch(IllegalArgumentException e){System.out.println(e.getMessage());}
        try { Enum.valueOf(Color.class,null); } catch(NullPointerException e){System.out.println("null name");}
        System.out.println(initialized);System.out.println(Init.class.getEnumConstants().length);System.out.println(initialized);
        EnumMap<Color,String> map=new EnumMap<>(Color.class);map.put(Color.BLUE,"b");map.put(Color.RED,null);map.put(Color.GREEN,"g");
        for(Color color:map.keySet())System.out.println(color.name()+"="+map.get(color));
        EnumMap<Color,String> clone=map.clone();clone.remove(Color.BLUE);System.out.println(map.size()+"/"+clone.size());
        EnumSet<Color> colors=EnumSet.allOf(Color.class);colors.remove(Color.GREEN);System.out.println(colors.size());
        for(Color c:colors)System.out.println(c.name());
        EnumSet<Many> many=EnumSet.range(Many.V01,Many.V65);many.remove(Many.V64);System.gc();
        System.out.println(many.size());System.out.println(EnumSet.complementOf(many).size());
        for(Many c:EnumSet.complementOf(many))System.out.println(c.name());
        class Local {}
        Object anonymous=new Object(){};
        System.out.println(Local.class.getCanonicalName());System.out.println(anonymous.getClass().getCanonicalName());
        System.out.println(Color.class.getCanonicalName());System.out.println(Color[][].class.getCanonicalName());
        System.out.println(Color.class.getDeclaringClass()==EnumTest.class);
        System.out.println(EnumTest.class.getDeclaringClass());System.out.println(int.class.getCanonicalName());
    }
}
