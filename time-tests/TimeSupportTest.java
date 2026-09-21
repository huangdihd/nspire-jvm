import java.io.*;
import java.util.*;
import java.util.zip.CRC32;
public class TimeSupportTest {
    public static void main(String[] args)throws Exception {
        for(long x:new long[]{Long.MIN_VALUE,-1001,-1,0,1,1001,Long.MAX_VALUE})for(long y:new long[]{Long.MIN_VALUE,-1000,-1,0,1,1000,Long.MAX_VALUE}) {
            try{System.out.println(Math.addExact(x,y));}catch(ArithmeticException e){System.out.println("add overflow");}
            try{System.out.println(Math.subtractExact(x,y));}catch(ArithmeticException e){System.out.println("subtract overflow");}
            try{System.out.println(Math.multiplyExact(x,y));}catch(ArithmeticException e){System.out.println("multiply overflow");}
            try{System.out.println(Math.floorDiv(x,y)+"|"+Math.floorMod(x,y));}catch(ArithmeticException e){System.out.println("zero divisor");}
        }
        for(int x:new int[]{Integer.MIN_VALUE,-1,0,1,Integer.MAX_VALUE}){
            try{System.out.println(Math.incrementExact(x));}catch(ArithmeticException e){System.out.println("increment overflow");}
            try{System.out.println(Math.negateExact(x));}catch(ArithmeticException e){System.out.println("negate overflow");}
        }
        Locale old=Locale.getDefault();Locale.setDefault(Locale.US);Locale.setDefault(Locale.Category.FORMAT,Locale.FRANCE);
        System.out.println(Locale.getDefault()+"|"+Locale.getDefault(Locale.Category.DISPLAY)+"|"+Locale.getDefault(Locale.Category.FORMAT));
        Locale.setDefault(Locale.Category.DISPLAY,Locale.JAPAN);System.out.println(Locale.getDefault(Locale.Category.DISPLAY));
        Locale.setDefault(old);
        try{Locale.getDefault((Locale.Category)null);}catch(NullPointerException e){System.out.println("null category");}
        byte[] bytes=new byte[4099];for(int i=0;i<bytes.length;i++)bytes[i]=(byte)(i*19);
        CRC32 crc=new CRC32();crc.update(bytes);System.out.println(crc.getValue());crc.reset();
        for(byte b:bytes)crc.update(b);System.out.println(crc.getValue());crc.update(bytes,10,500);System.out.println(crc.getValue());
        byte[] encoded={0,3,65,(byte)0xc0,(byte)0x80,0,0,0,42,(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xff,(byte)0xff};
        DataInputStream in=new DataInputStream(new ByteArrayInputStream(encoded));String utf=in.readUTF();System.out.println(utf.length()+"|"+(int)utf.charAt(1));System.out.println(in.readInt()+"|"+in.readLong());
        try{in.readByte();}catch(EOFException e){System.out.println("EOF");}
    }
}
