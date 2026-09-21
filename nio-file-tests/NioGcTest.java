import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.file.*;

/** VM resource-lifetime stress; host driver also limits native descriptors. */
public class NioGcTest {
    private static void abandoned() throws Exception {
        InputStream in=Files.newInputStream(Paths.get("original"));
        OutputStream out=Files.newOutputStream(Paths.get("gc-output"),StandardOpenOption.CREATE,StandardOpenOption.APPEND);
        SeekableByteChannel channel=Files.newByteChannel(Paths.get("original"));
        if(in.read()!='o' || channel.size()!=19) throw new AssertionError();
        out.write(7);
    }
    public static void main(String[] args) throws Exception {
        SeekableByteChannel retained=Files.newByteChannel(Paths.get("original"));
        for(int i=0;i<200;i++) {
            abandoned();
            if(i%4==0) System.gc();
        }
        System.gc();
        ByteBuffer byteBuffer=ByteBuffer.allocate(1);
        if(retained.read(byteBuffer)!=1 || byteBuffer.get(0)!='o') throw new AssertionError();
        retained.close();
        OutputStream atExit=Files.newOutputStream(Paths.get("exit-output"));atExit.write(11);
        System.out.println(200);
        if(args.length!=0) String.format("%d",1);
    }
}
