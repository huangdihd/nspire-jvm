public class ParseNumberTest {
    public static void main(String[] args) {
        String[] inputs={"0","-0","+1.5",".1","1.","1e3","0x1.fp4","-0x.8p+2","NaN","-NaN","+Infinity","-Infinity","1f","1D","\u0000 \t1.25\r\n", "2.2250738585072014e-308","4.9406564584124654e-324","2.4703282292062327e-324","2.4703282292062328e-324","1.7976931348623157e308","1.7976931348623159e308","1e99999","-1e-99999","9007199254740993","0x1.00000000000008p0","0x1.00000000000018p0","1.0000000596046448","3.4028236e38","1.401298464324817e-45","0x1p-149","0x1p-150", ""," ",".","+","--1","NaNf","nan","Inf","1_0","1,2","0x1","0x1p","1e+","1\u0000x","１２","\u20031"};
        for(String s:inputs) {
            try {System.out.println(Double.doubleToLongBits(Double.parseDouble(s)));System.out.println(Double.doubleToLongBits(Double.valueOf(s).doubleValue()));}
            catch(NumberFormatException e){System.out.println("invalid double");}
            try {System.out.println(Float.floatToIntBits(Float.parseFloat(s)));}catch(NumberFormatException e){System.out.println("invalid float");}
        }
        try {Double.parseDouble(null);throw new AssertionError();}catch(NullPointerException expected){}
        long state=123456789;
        for(int i=0;i<500;i++) {
            state=state*2862933555777941757L+3037000493L;
            String s=new StringBuilder().append(state).append('.').append(i).append('e').append(i%800-400).toString();
            System.out.println(Double.doubleToLongBits(Double.parseDouble(s)));System.out.println(Float.floatToIntBits(Float.parseFloat(s)));
        }
        System.out.println("floating lexical rules and boundary rounding passed");
    }
}
