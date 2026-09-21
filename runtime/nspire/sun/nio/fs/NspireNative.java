package sun.nio.fs;

import java.io.FileDescriptor;
import java.io.IOException;

/** Only these operations cross into the VM; Java owns the surrounding protocol. */
final class NspireNative {
    static native boolean fileKeysSupported();
    static native FileDescriptor open(String path,int flags,String display) throws IOException;
    static native boolean delete(String path,boolean onlyIfExists,String display) throws IOException;
    static native void mkdir(String path,String display) throws IOException;
    static native void access(String path,int mode,String display) throws IOException;
    static native long[] attributes(String path,boolean follow,String display) throws IOException;
    static native String realPath(String path,String display) throws IOException;
    static native long position(FileDescriptor fd,long offset,boolean set) throws IOException;
    static native long size(FileDescriptor fd) throws IOException;
    static native void truncate(FileDescriptor fd,long size) throws IOException;
}
