# Files and native stream handles

File, FileSystem, UnixFileSystem, DefaultFileSystem, ExpiringCache, FileFilter
and FilenameFilter are unchanged OpenJDK 8 sources at the revision recorded in
runtime/openjdk8/SOURCES.json. Path normalization, parent/child resolution,
invalid-NUL checks, mkdirs and directory filters execute the original bytecode.
The VM supplies its actual working directory as user.dir. Paths use `/`; the
classpath separator remains `;` on both builds.

src/filesystem.inc binds native methods to actual OS operations. The unchanged
Unix canonicalizer in vendor/openjdk8-file/canonicalize_md.c is compiled through
src/canonical.c. It resolves existing prefixes with realpath, appends unresolved
tails and collapses dot segments. Canonical caches are disabled using the
original OpenJDK properties to observe external filesystem changes. Host symlink
behavior is tested; Ndless realpath differs and is not verified on hardware.
PATH_MAX uses the platform value, falling back to 1024 for the SDK. Filenames
cross the UTF-16/UTF-8 boundary including supplementary characters; unpaired
input surrogates become `?`. Returned paths use the original UTF-8 decoder.

Host operations include attributes, access checks, last-modified time, length,
exclusive create, remove, rename, listing, mkdir, permissions, read-only status
and space queries. Native attributes read File's actual private path. Stream
constructors call virtual File.getPath, as the original Java implementation does.
Open errors produce FileNotFoundException; I/O errors produce IOException.

FileDescriptor, FileInputStream, FileOutputStream and InputStream now execute
preserved OpenJDK Java bytecode. src/descriptors.inc supplies only the native
handle operations. String/File/descriptor constructors, getFD/valid, shared
offsets, append/truncate, byte/array/slice I/O and seekable skip/available are
covered. InputStream's original partial-read error handling, skip, mark/reset,
available and close defaults run in Java; resource streams and intrinsic
ByteArrayInputStream declare their actual overrides.

Explicit close executes the original Java attach/closeAll callbacks, including
shared stream overrides and suppressed IOException aggregation. Ownership is
stored on the descriptor, so reachable aliases or the descriptor alone retain
the handle. Unreachable groups and every VM return/abort close owned handles
once. Generic Java finalizers are not executed by the collector: GC cleanup
does not claim to invoke custom finalizers or subclass close callbacks.

Host sync calls fsync and propagates failure as SyncFailedException. Standard
descriptors borrow fd 0/1/2; closing a Java alias invalidates the shared Java
descriptor but retains the embedding process/SDK UI's handle. System.in/out/err
wrap these same objects. System.setIn/setOut/setErr replace Java stream fields.
Host pipe input and available use actual read/ioctl operations. Generic JNI,
FileChannel and full nonseekable-device semantics remain unsupported.

FileOutputStream inherits the original OutputStream.flush no-op. Host writes
are unbuffered. InputStreamReader's existing malformed UTF-8 pushback limitation
on custom streams also applies to file input; this is not a complete Reader.

## Ndless boundary

The ARM build links SDK stat, open/read/write/seek/close, directory enumeration,
mkdir, rename/remove and realpath. SDK descriptors use OS stdio syscalls, so
buffering and visibility before close require device testing. Target fstat
returns ENOSYS; directories are checked before open. Host code additionally
checks the opened descriptor. Target stat only initializes timestamp seconds;
the adapter zeroes the structure and reads those supported fields. Target seek
range is constrained by off_t. Target sync throws SyncFailedException because
the SDK has no verified durable-storage synchronization primitive.

SDK standard input is line-oriented. Its single-byte _read path is unsafe, so
the bridge reads into a descriptor-owned 4096-byte buffer and serves byte/slice
requests from that shared cursor. Target stdin available reports only buffered
bytes. This adaptation builds for ARM but has not been exercised on hardware.

These target operations explicitly stop with an unsupported diagnostic:
createNewFile, access/permission checks and changes, setLastModified, setReadOnly
and disk-space queries. The pinned SDK's O_EXCL implementation has an inverted
stat check and does not provide atomic exclusive creation. Correct native
implementations and device verification remain necessary. Ordinary fopen is
not claimed to implement Java's atomic createNewFile contract.

Other missing dependencies include createTempFile/SecureRandom, deleteOnExit/
shutdown hooks, URI/NIO Path integration and serialization. Source availability
does not establish full java.io compatibility.

## Verification

```sh
make host build/file-lifetime
python3 tools/build-runtime.py --java8-home /path/to/java8
python3 tools/test-files.py --vm build/nspire-jvm --java /path/to/java8/bin/java --lifetime build/file-lifetime
```

Use a Linux Java 8 oracle; javac may be the Windows JDK discovered by the build
tools. Five programs compare output and disk contents: paths; mutation/filter
callbacks; file streaming; shared descriptors/close callbacks/errors/sync; and
InputStream defaults, partial errors and long skips. Three console modes compare
exact stdout/stderr bytes and prefilled-pipe input, including alias invalidation
and System.setIn replacement. Two VM checks abandon input/output alias groups
under a 64-descriptor limit, test retained aliases during GC, and inspect disk
bytes after normal/fatal returns. The optional eleventh check counts /proc/self/fd
across six normal/fatal runs and six console-close runs in one host process;
later runs must still have working process stdio. See FILE-RESULTS.txt.

The sanitizer embedding harness is built with `make build/file-lifetime-asan`;
pass it through `--lifetime` alongside `--vm build/nspire-jvm-asan`, with
ASAN_OPTIONS=detect_leaks=1 and UBSAN_OPTIONS=halt_on_error=1.

Original Logback headerBytes now returns bytes matching Java 17 with the same
configured line separator. Class.getInterfaces returns a fresh array of direct
interfaces; a separate oracle test covers source order, inherited-only
interfaces, arrays, primitives, annotations and reflective calls. Whole Xinbot
now reaches missing java.nio.file.FileSystems during Jansi extraction, before
Xinbot.main. Temporary-directory configuration is described in NATIVE-SUPPORT.md.
No full application or calculator run has completed. The unmodified JNI files
beside the canonicalizer are references and are not compiled.
