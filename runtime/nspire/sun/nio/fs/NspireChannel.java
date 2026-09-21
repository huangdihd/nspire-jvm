package sun.nio.fs;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.channels.*;

/** Seekable channel backed by the same owned descriptor used by java.io. */
final class NspireChannel implements SeekableByteChannel {
    private final FileDescriptor fd;
    private final FileInputStream in;
    private final FileOutputStream out;
    private final boolean append;
    private boolean open=true;
    NspireChannel(FileDescriptor fd,int flags) {
        this.fd=fd;
        append=(flags&32)!=0;
        in=(flags&1)!=0?new FileInputStream(fd):null;
        out=(flags&2)!=0?new FileOutputStream(fd):null;
    }
    private void checkOpen() throws ClosedChannelException { if(!open) throw new ClosedChannelException(); }
    public synchronized int read(ByteBuffer dst) throws IOException {
        checkOpen();
        if(in==null) throw new NonReadableChannelException();
        if(dst.isReadOnly()) throw new IllegalArgumentException("Read-only buffer");
        if(!dst.hasRemaining()) return 0;
        byte[] bytes=new byte[Math.min(dst.remaining(),8192)];
        int count=in.read(bytes);
        if(count>0) dst.put(bytes,0,count);
        return count;
    }
    public synchronized int write(ByteBuffer src) throws IOException {
        checkOpen();
        if(out==null) throw new NonWritableChannelException();
        int count=Math.min(src.remaining(),8192);
        if(count==0) return 0;
        byte[] bytes=new byte[count];
        int position=src.position();
        // Advance the caller's buffer only after the stream write succeeds.
        for(int i=0;i<count;i++) bytes[i]=src.get(position+i);
        out.write(bytes);
        src.position(position+count);
        return count;
    }
    public synchronized long position() throws IOException { checkOpen();return append?NspireNative.size(fd):NspireNative.position(fd,0,false); }
    public synchronized SeekableByteChannel position(long value) throws IOException {
        if(value<0) throw new IllegalArgumentException();
        checkOpen();NspireNative.position(fd,value,true);return this;
    }
    public synchronized long size() throws IOException { checkOpen();return NspireNative.size(fd); }
    public synchronized SeekableByteChannel truncate(long value) throws IOException {
        if(value<0) throw new IllegalArgumentException();
        checkOpen();if(out==null) throw new NonWritableChannelException();
        NspireNative.truncate(fd,value);return this;
    }
    public synchronized boolean isOpen() { return open; }
    public synchronized void close() throws IOException {
        if(open) { open=false;if(in!=null) in.close();else out.close(); }
    }
}
