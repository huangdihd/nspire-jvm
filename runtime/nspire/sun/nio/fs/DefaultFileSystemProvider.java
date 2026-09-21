package sun.nio.fs;

/** Selects the interpreter's real platform file provider. */
public final class DefaultFileSystemProvider {
    private DefaultFileSystemProvider() {}
    public static java.nio.file.spi.FileSystemProvider create() {
        return new NspireFileSystemProvider();
    }
}
