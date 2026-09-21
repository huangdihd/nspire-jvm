import java.io.*;
public class ConsoleCloseTest {
    public static void main(String[] args) throws Exception {
        new FileInputStream(FileDescriptor.in).close();
        new FileOutputStream(FileDescriptor.out).close();
        new FileOutputStream(FileDescriptor.err).close();
        if(FileDescriptor.in.valid() || FileDescriptor.out.valid() || FileDescriptor.err.valid())
            throw new AssertionError();
    }
}
