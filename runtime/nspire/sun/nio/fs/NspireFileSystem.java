package sun.nio.fs;

import java.nio.file.*;
import java.nio.file.attribute.UserPrincipalLookupService;
import java.nio.file.spi.FileSystemProvider;
import java.util.*;
import java.util.regex.Pattern;

/** Default filesystem; path syntax is Unix on both host and Ndless. */
final class NspireFileSystem extends FileSystem {
    private final NspireFileSystemProvider provider;
    private final byte[] directory;
    private final NspirePath root;
    NspireFileSystem(NspireFileSystemProvider provider) {
        this.provider=provider;
        directory=Util.toBytes(NspirePath.normalizeAndCheck(System.getProperty("user.dir")));
        if(directory.length==0 || directory[0]!='/') throw new IllegalArgumentException("user.dir must be absolute");
        root=new NspirePath(this,"/");
    }
    byte[] defaultDirectory() { return directory; }
    boolean needToResolveAgainstDefaultDirectory() { return true; }
    NspirePath rootDirectory() { return root; }
    char[] normalizeNativePath(char[] path) { return path; }
    String normalizeJavaPath(String path) { return path; }
    public FileSystemProvider provider() { return provider; }
    public boolean isOpen() { return true; }
    public boolean isReadOnly() { return false; }
    public void close() { throw new UnsupportedOperationException("Cannot close the default filesystem"); }
    public String getSeparator() { return "/"; }
    public Iterable<Path> getRootDirectories() { return Collections.<Path>singletonList(root); }
    public Iterable<FileStore> getFileStores() { throw new UnsupportedOperationException("File stores are not implemented"); }
    public Set<String> supportedFileAttributeViews() { return Collections.singleton("basic"); }
    public Path getPath(String first,String... more) {
        StringBuilder path=new StringBuilder(first);
        for(String part:more) if(part.length()>0) {
            if(path.length()>0) path.append('/');
            path.append(part);
        }
        return new NspirePath(this,path.toString());
    }
    public PathMatcher getPathMatcher(String expression) {
        int colon=expression.indexOf(':');
        if(colon<=0) throw new IllegalArgumentException();
        String syntax=expression.substring(0,colon), pattern=expression.substring(colon+1);
        if(syntax.equals("glob")) pattern=Globs.toUnixRegexPattern(pattern);
        else if(!syntax.equals("regex")) throw new UnsupportedOperationException(syntax);
        final Pattern compiled=Pattern.compile(pattern);
        return new PathMatcher() { public boolean matches(Path path) { return compiled.matcher(path.toString()).matches(); } };
    }
    public UserPrincipalLookupService getUserPrincipalLookupService() { throw new UnsupportedOperationException("User lookup is not implemented"); }
    public WatchService newWatchService() { throw new UnsupportedOperationException("File watches are not implemented"); }
}
