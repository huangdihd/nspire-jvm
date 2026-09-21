@SuppressWarnings("deprecation")
public class HighByteStringTest {
    public static void main(String[] args) {
        byte[] bytes=new byte[256];for(int i=0;i<256;i++)bytes[i]=(byte)i;
        for(int high:new int[]{0,1,127,128,255,256,65535,-1,Integer.MIN_VALUE,Integer.MAX_VALUE}) {
            String all=new String(bytes,high),slice=new String(bytes,high,3,240);
            System.out.println(all.length()+":"+all.hashCode()+":"+slice.hashCode()+":"+(int)all.charAt(0)+":"+(int)all.charAt(255));
        }
        for(byte[] source:new byte[][]{bytes,null})for(int[] range:new int[][]{{0,0},{256,0},{255,1},{257,0},{-1,0},{0,-1},{-1,-1},{Integer.MAX_VALUE,1},{0,Integer.MAX_VALUE}}) {
            try {System.out.println(new String(source,0,range[0],range[1]).length());}
            catch(RuntimeException e){System.out.println(e.getClass().getName());}
        }
        try{new String((byte[])null,0);throw new AssertionError();}catch(NullPointerException expected){System.out.println("null");}
    }
}
