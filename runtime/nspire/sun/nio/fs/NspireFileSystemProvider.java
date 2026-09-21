package sun.nio.fs;

import java.io.*;
import java.net.URI;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.*;
import java.nio.file.attribute.*;
import java.nio.file.spi.FileSystemProvider;
import java.util.*;

/** Native filesystem adapter, with unsupported platform capabilities explicit. */
public final class NspireFileSystemProvider extends FileSystemProvider {
    private final NspireFileSystem fs=new NspireFileSystem(this);
    public String getScheme() { return "file"; }
    private void checkUri(URI uri) {
        if(!getScheme().equalsIgnoreCase(uri.getScheme()) || uri.getAuthority()!=null ||
           !"/".equals(uri.getPath()) || uri.getQuery()!=null || uri.getFragment()!=null)
            throw new IllegalArgumentException("Expected file:/// URI");
    }
    public FileSystem getFileSystem(URI uri) { checkUri(uri);return fs; }
    public FileSystem newFileSystem(URI uri,Map<String,?> env) {
        checkUri(uri);throw new FileSystemAlreadyExistsException();
    }
    public Path getPath(URI uri) { return fs.getPath(new File(uri).getPath()); }
    private NspirePath check(Path path) {
        Objects.requireNonNull(path);
        if(!(path instanceof NspirePath) || path.getFileSystem()!=fs) throw new ProviderMismatchException();
        return (NspirePath)path;
    }
    private String nativePath(Path path) { return check(path).toAbsolutePath().toString(); }
    private static void noAttributes(FileAttribute<?>[] attrs) {
        for(FileAttribute<?> attr:attrs) {
            Objects.requireNonNull(attr);
            throw new UnsupportedOperationException("Atomic creation attributes are not implemented: "+attr.name());
        }
    }
    private static int flags(Iterable<? extends OpenOption> options) {
        int flags=0;
        for(OpenOption option:options) {
            Objects.requireNonNull(option);
            if(option==StandardOpenOption.READ) flags|=1;
            else if(option==StandardOpenOption.WRITE) flags|=2;
            else if(option==StandardOpenOption.CREATE) flags|=4;
            else if(option==StandardOpenOption.CREATE_NEW) flags|=8;
            else if(option==StandardOpenOption.TRUNCATE_EXISTING) flags|=16;
            else if(option==StandardOpenOption.APPEND) flags|=32|2;
            else if(option==StandardOpenOption.DELETE_ON_CLOSE) flags|=64;
            else if(option==LinkOption.NOFOLLOW_LINKS) flags|=128;
            else if(option==StandardOpenOption.SYNC) flags|=256;
            else if(option==StandardOpenOption.DSYNC) flags|=512;
            else if(option!=StandardOpenOption.SPARSE) throw new UnsupportedOperationException(option.toString());
        }
        if((flags&3)==0) flags|=1;
        if((flags&32)!=0 && (flags&(1|16))!=0) throw new IllegalArgumentException("APPEND with READ or TRUNCATE_EXISTING");
        return flags;
    }
    public InputStream newInputStream(Path path,OpenOption... options) throws IOException {
        for(OpenOption option:options) if(option==StandardOpenOption.WRITE || option==StandardOpenOption.APPEND)
            throw new UnsupportedOperationException(option.toString());
        int flags=flags(Arrays.asList(options));
        return new FileInputStream(NspireNative.open(nativePath(path),flags,path.toString()));
    }
    public OutputStream newOutputStream(Path path,OpenOption... options) throws IOException {
        Set<OpenOption> set=new HashSet<OpenOption>();
        if(options.length==0) {set.add(StandardOpenOption.CREATE);set.add(StandardOpenOption.TRUNCATE_EXISTING);}
        for(OpenOption option:options) {
            if(option==StandardOpenOption.READ) throw new IllegalArgumentException("READ is not allowed");
            set.add(option);
        }
        set.add(StandardOpenOption.WRITE);
        return new FileOutputStream(NspireNative.open(nativePath(path),flags(set),path.toString()));
    }
    public SeekableByteChannel newByteChannel(Path path,Set<? extends OpenOption> options,FileAttribute<?>... attrs) throws IOException {
        noAttributes(attrs);int flags=flags(options);
        return new NspireChannel(NspireNative.open(nativePath(path),flags,path.toString()),flags);
    }
    public void delete(Path path) throws IOException { NspireNative.delete(nativePath(path),false,path.toString()); }
    public boolean deleteIfExists(Path path) throws IOException { return NspireNative.delete(nativePath(path),true,path.toString()); }
    public void createDirectory(Path path,FileAttribute<?>... attrs) throws IOException {
        noAttributes(attrs);NspireNative.mkdir(nativePath(path),path.toString());
    }
    public void checkAccess(Path path,AccessMode... modes) throws IOException {
        int flags=0;
        for(AccessMode mode:modes) {
            Objects.requireNonNull(mode);
            flags|=mode==AccessMode.READ?4:mode==AccessMode.WRITE?2:1;
        }
        NspireNative.access(nativePath(path),flags,path.toString());
    }
    public boolean isHidden(Path path) { return check(path).toFile().isHidden(); }
    public boolean isSameFile(Path a,Path b) throws IOException {
        NspirePath first=check(a);
        if(first.equals(b)) return true;
        if(!(b instanceof NspirePath) || b.getFileSystem()!=fs) return false;
        if(!NspireNative.fileKeysSupported()) throw new UnsupportedOperationException("Platform file identity is not verified");
        long[] x=NspireNative.attributes(nativePath(a),true,a.toString()),y=NspireNative.attributes(nativePath(b),true,b.toString());
        return x[6]==y[6] && x[7]==y[7];
    }
    public DirectoryStream<Path> newDirectoryStream(Path path,DirectoryStream.Filter<? super Path> filter) throws IOException {
        Objects.requireNonNull(filter);final NspirePath parent=check(path);
        if(!readAttributes(parent,BasicFileAttributes.class).isDirectory()) throw new NotDirectoryException(parent.toString());
        final String[] names=parent.toAbsolutePath().toFile().list();
        if(names==null) throw new FileSystemException(parent.toString(),null,"Cannot list directory");
        return new DirectoryStream<Path>() {
            private boolean closed,iterated;
            public synchronized void close() { closed=true; }
            public synchronized Iterator<Path> iterator() {
                if(closed || iterated) throw new IllegalStateException();
                iterated=true;
                return new Iterator<Path>() {
                    private int index;private Path next;
                    public boolean hasNext() {
                        if(next!=null) return true;
                        while(!closed && index<names.length) {
                            Path candidate=parent.resolve(names[index++]);
                            try { if(filter.accept(candidate)) {next=candidate;return true;} }
                            catch(IOException e) { throw new DirectoryIteratorException(e); }
                        }
                        return false;
                    }
                    public Path next() { if(!hasNext()) throw new NoSuchElementException();Path result=next;next=null;return result; }
                    public void remove() { throw new UnsupportedOperationException(); }
                };
            }
        };
    }
    private static final class Attributes implements BasicFileAttributes {
        private final long[] data;
        Attributes(long[] data) { this.data=data; }
        public FileTime lastModifiedTime() { return FileTime.from(java.time.Instant.ofEpochSecond(data[2],data[3])); }
        public FileTime lastAccessTime() { return FileTime.from(java.time.Instant.ofEpochSecond(data[4],data[5])); }
        public FileTime creationTime() { return lastModifiedTime(); }
        public boolean isRegularFile() { return (data[0]&1)!=0; }
        public boolean isDirectory() { return (data[0]&2)!=0; }
        public boolean isSymbolicLink() { return (data[0]&4)!=0; }
        public boolean isOther() { return (data[0]&8)!=0; }
        public long size() { return data[1]; }
        public Object fileKey() { return null; }
    }
    public <A extends BasicFileAttributes> A readAttributes(Path path,Class<A> type,LinkOption... options) throws IOException {
        check(path);Objects.requireNonNull(type);
        if(type!=BasicFileAttributes.class) throw new UnsupportedOperationException(type.getName());
        return type.cast(new Attributes(NspireNative.attributes(nativePath(path),Util.followLinks(options),path.toString())));
    }
    public <V extends FileAttributeView> V getFileAttributeView(final Path path,Class<V> type,final LinkOption... options) {
        check(path);Objects.requireNonNull(type);Util.followLinks(options);
        final LinkOption[] saved=options.clone();
        if(type!=BasicFileAttributeView.class) return null;
        return type.cast(new BasicFileAttributeView() {
            public String name() { return "basic"; }
            public BasicFileAttributes readAttributes() throws IOException { return NspireFileSystemProvider.this.readAttributes(path,BasicFileAttributes.class,saved); }
            public void setTimes(FileTime modified,FileTime access,FileTime created) { throw new UnsupportedOperationException("NIO setTimes is not implemented"); }
        });
    }
    public Map<String,Object> readAttributes(Path path,String attributes,LinkOption... options) throws IOException {
        int colon=attributes.indexOf(':');
        if(colon>=0) {if(!attributes.substring(0,colon).equals("basic")) throw new UnsupportedOperationException(attributes);attributes=attributes.substring(colon+1);}
        BasicFileAttributes a=readAttributes(path,BasicFileAttributes.class,options);
        Map<String,Object> all=new HashMap<String,Object>();
        all.put("size",a.size());all.put("lastModifiedTime",a.lastModifiedTime());all.put("lastAccessTime",a.lastAccessTime());all.put("creationTime",a.creationTime());
        all.put("isRegularFile",a.isRegularFile());all.put("isDirectory",a.isDirectory());all.put("isSymbolicLink",a.isSymbolicLink());all.put("isOther",a.isOther());all.put("fileKey",a.fileKey());
        Map<String,Object> result=new HashMap<String,Object>();
        for(String name:Util.split(attributes,',')) {
            if(name.equals("*")) result.putAll(all);
            else {if(!all.containsKey(name)) throw new IllegalArgumentException(name);result.put(name,all.get(name));}
        }
        return result;
    }
    public void setAttribute(Path path,String attribute,Object value,LinkOption... options) { check(path);throw new UnsupportedOperationException("NIO attribute writes are not implemented"); }
    public FileStore getFileStore(Path path) { check(path);throw new UnsupportedOperationException("File stores are not implemented"); }
    public void copy(Path source,Path target,CopyOption... options) { check(source);check(target);throw new UnsupportedOperationException("Path-to-path copy is not implemented"); }
    public void move(Path source,Path target,CopyOption... options) { check(source);check(target);throw new UnsupportedOperationException("NIO move is not implemented"); }
}
