import java.io.*;
import java.util.Arrays;
public class OutputStreamTest {
    static class Sink extends OutputStream {
        final ByteArrayOutputStream bytes = new ByteArrayOutputStream(0);
        int flushes, closes;
        public void write(int b) { bytes.write(b); System.gc(); }
        public void flush() { flushes++; }
        public void close() { closes++; }
    }
    interface Attempt { void run() throws Exception; }
    static void attempt(Attempt a) { try { a.run(); System.out.println("ok"); } catch (Exception e) { System.out.println(e.getClass().getName()); } }
    public static void main(String[] args) throws Exception {
        Sink sink = new Sink();
        sink.write(new byte[]{1, -1, 3}); sink.write(new byte[]{4, 5, 6}, 1, 2);
        sink.close(); sink.write(256);
        System.out.println(Arrays.toString(sink.bytes.toByteArray()));
        attempt(() -> sink.write(null)); attempt(() -> sink.write(new byte[0], -1, 0));
        attempt(() -> sink.write(new byte[2], 1, Integer.MAX_VALUE));
        attempt(() -> sink.write(new byte[2], 2, 0));
        FilterOutputStream filter = new FilterOutputStream(sink) {
            public void write(int b) throws IOException { super.write(b + 1); }
        };
        filter.write(new byte[]{10, 20}); filter.flush(); filter.close();
        System.out.println(Arrays.toString(sink.bytes.toByteArray()));
        System.out.println(sink.flushes); System.out.println(sink.closes);
        Sink bufferedSink = new Sink();
        BufferedOutputStream buffered = new BufferedOutputStream(bufferedSink, 4);
        buffered.write(1); buffered.write(new byte[]{2, 3});
        System.out.println(bufferedSink.bytes.size());
        buffered.write(new byte[]{4, 5, 6, 7, 8}); buffered.flush();
        System.out.println(Arrays.toString(bufferedSink.bytes.toByteArray()));
        buffered.close(); System.out.println(bufferedSink.closes);
        attempt(() -> new BufferedOutputStream(sink, 0));
        ByteArrayOutputStream memory = new ByteArrayOutputStream(0);
        for (int i=0;i<1025;i++) memory.write(i);
        byte[] copy = memory.toByteArray(); copy[0] = 12;
        System.out.println(memory.toByteArray()[0]); System.out.println(memory.size());
        ByteArrayOutputStream copySink = new ByteArrayOutputStream(); memory.writeTo(copySink);
        System.out.println(Arrays.equals(memory.toByteArray(), copySink.toByteArray()));
        memory.reset(); memory.close(); memory.write(42); System.out.println(memory.size());
        attempt(() -> new ByteArrayOutputStream(-1));
        IOException first = new IOException("first"), second = new IOException("second");
        FilterOutputStream failing = new FilterOutputStream(new OutputStream() {
            public void write(int b) {}
            public void flush() throws IOException { throw first; }
            public void close() throws IOException { throw second; }
        });
        try { failing.close(); } catch (IOException e) {
            // Java 8's try-with-resources close preserves the flush exception.
            System.out.println(e == first); System.out.println(e.getSuppressed().length);
            System.out.println(e.getSuppressed()[0] == second);
        }
        first.getSuppressed()[0] = null; System.gc(); System.out.println(first.getSuppressed()[0] == second);
        attempt(() -> first.addSuppressed(first)); attempt(() -> first.addSuppressed(null));
    }
}
