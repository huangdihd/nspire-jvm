import java.io.*;
/** Calls the actual JansiLoader inside Xinbot, without changing its properties. */
public class JansiCleanupTest {
    static File extracted;
    static void check(boolean value) { if (!value) throw new AssertionError("Jansi cleanup invariant"); }
    public static void main(String[] args) throws Exception {
        Class<?> loader = Class.forName("org.fusesource.jansi.internal.JansiLoader");
        boolean loaded = (Boolean)loader.getMethod("initialize").invoke(null);
        check(!loaded); // This VM truthfully reports no dynamic JNI loader.
        File directory = new File(System.getProperty("java.io.tmpdir"));
        File[] files = directory.listFiles();
        check(files.length == 2);
        for (File file : files) if (file.getName().endsWith("-libjansi.so")) extracted = file;
        check(extracted != null && extracted.length() == 18976);
        // Preserve evidence outside the deletion directory for the test driver.
        try (InputStream in = new FileInputStream(extracted);
             OutputStream out = new FileOutputStream("captured-library")) {
            byte[] block = new byte[1024];
            int length;
            while ((length = in.read(block)) >= 0) out.write(block, 0, length);
        }
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            check(extracted.exists()); // Application hooks precede deletion.
            try (OutputStream out = new FileOutputStream("hook-ran")) { out.write(73); }
            catch (IOException e) { throw new RuntimeException(e); }
        }));
        if (args.length != 0) System.exit(17);
    }
}
