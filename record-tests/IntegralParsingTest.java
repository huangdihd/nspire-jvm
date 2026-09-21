import java.math.BigInteger;
public class IntegralParsingTest {
    static void one(String value,int radix) {
        try {System.out.println("int:"+Integer.parseInt(value,radix));}catch(NumberFormatException expected){System.out.println("int:"+expected.getMessage());}
        try {System.out.println("long:"+Long.parseLong(value,radix));}catch(NumberFormatException expected){System.out.println("long:"+expected.getMessage());}
    }
    public static void main(String[] args) {
        String[] values={null,"","+","-","0","+0","-0","2147483647","2147483648","-2147483648","-2147483649","9223372036854775807","9223372036854775808","-9223372036854775808","-9223372036854775809","00123","１２３","١٢٣","ＡＦ"," 3","3 ","0x10","1_2","1\u00002","\ud835\udfce","\ud800","12.3"};
        for(String value:values)for(int radix:new int[]{1,2,10,16,36,37})one(value,radix);
        for(int radix=2;radix<=36;radix++) {
            for(String value:new String[]{"2147483647","2147483648","-2147483648","-2147483649","9223372036854775807","9223372036854775808","-9223372036854775808","-9223372036854775809"})
                one(new BigInteger(value).toString(radix),radix);
        }
        System.out.println(Integer.parseInt("-2147483648"));System.out.println(Long.parseLong("-9223372036854775808"));
        for(int radix:new int[]{Integer.MIN_VALUE,1,2,10,16,36,37,Integer.MAX_VALUE}) {
            for(long value:new long[]{Long.MIN_VALUE,Long.MAX_VALUE,0,-1,1,2147483648L}) {
                System.out.println(Long.toString(value,radix));System.out.println(Integer.toString((int)value,radix));
            }
        }
    }
}
