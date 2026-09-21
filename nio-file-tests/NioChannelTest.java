import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.file.*;
import java.util.*;

public class NioChannelTest {
    static void error(NioCopyTest.Action action) throws Exception { NioCopyTest.error(action); }
    public static void main(String[] args) throws Exception {
        Path p=Paths.get("channel");
        SeekableByteChannel channel=Files.newByteChannel(p,StandardOpenOption.CREATE_NEW,StandardOpenOption.READ,StandardOpenOption.WRITE);
        ByteBuffer bytes=ByteBuffer.wrap(new byte[]{0,1,2,3,4,5});bytes.position(1);bytes.limit(5);
        System.out.println(channel.write(bytes)+":"+bytes.position()+":"+channel.size()+":"+channel.position());
        channel.position(1);ByteBuffer out=ByteBuffer.allocate(5);out.position(2);out.limit(4);
        System.out.println(channel.read(out)+":"+out.position()+":"+Arrays.toString(out.array()));
        channel.position(8);System.out.println(channel.read(ByteBuffer.allocate(2)));
        System.out.println(channel.write(ByteBuffer.wrap(new byte[]{7})));
        System.out.println(Arrays.toString(Files.readAllBytes(p)));
        channel.truncate(4);System.out.println(channel.position()+":"+channel.size());
        channel.position(12);channel.truncate(20);System.out.println(channel.position()+":"+channel.size());
        channel.truncate(2);System.out.println(channel.position()+":"+channel.size());
        error(()->channel.position(-1));error(()->channel.truncate(-1));
        channel.close();channel.close();System.out.println(channel.isOpen());
        error(()->channel.read(ByteBuffer.allocate(0)));error(()->channel.position());
        try(SeekableByteChannel read=Files.newByteChannel(p)) {
            error(()->read.write(ByteBuffer.allocate(0)));error(()->read.truncate(0));
            error(()->read.read(ByteBuffer.allocate(3).asReadOnlyBuffer()));
        }
        try(SeekableByteChannel append=Files.newByteChannel(p,StandardOpenOption.APPEND)) {
            System.out.println("append="+append.position());append.position(0);append.write(ByteBuffer.wrap(new byte[]{9}));
            System.out.println("append="+append.position());error(()->append.read(ByteBuffer.allocate(0)));
        }
        System.out.println(Arrays.toString(Files.readAllBytes(p)));
        error(()->Files.newByteChannel(p,StandardOpenOption.CREATE_NEW,StandardOpenOption.WRITE));
        error(()->Files.newByteChannel(p,StandardOpenOption.APPEND,StandardOpenOption.READ));
        error(()->Files.newByteChannel(p,StandardOpenOption.APPEND,StandardOpenOption.TRUNCATE_EXISTING));
        try(OutputStream overwrite=Files.newOutputStream(p,StandardOpenOption.WRITE)) {overwrite.write(5);}
        System.out.println(Arrays.toString(Files.readAllBytes(p)));
        try(OutputStream truncate=Files.newOutputStream(p)) {truncate.write(6);}
        System.out.println(Arrays.toString(Files.readAllBytes(p)));
        try(SeekableByteChannel temporary=Files.newByteChannel(Paths.get("delete-open"),StandardOpenOption.CREATE,StandardOpenOption.WRITE,StandardOpenOption.DELETE_ON_CLOSE)) {
            temporary.write(ByteBuffer.wrap(new byte[]{3}));System.out.println(Files.exists(Paths.get("delete-open")));
        }
        error(()->Files.newInputStream(Paths.get("symlink"),LinkOption.NOFOLLOW_LINKS));
        for(int i=0;i<40;i++) { try(SeekableByteChannel c=Files.newByteChannel(p)) {if(c.size()!=1)throw new AssertionError();} }
        System.out.println("channel reuse OK");
    }
}
