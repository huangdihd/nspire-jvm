# Files and native stream handles

**Development snapshot:** this document describes the verified `4e097f3`
checkpoint. Current source replaces the intrinsic file streams with original
OpenJDK streams and a shared FileDescriptor bridge in `src/descriptors.inc`.
Host compilation succeeds, but FileStreamTest stops at the missing inherited
InputStream.markSupported method. Descriptor constructors, getFD, alias
lifetime, standard input and synchronization are work in progress, not verified
capabilities. See CHECKPOINT.md and DESCRIPTOR-CHECKPOINT.txt.

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

FileInputStream/FileOutputStream are intrinsic classes backed by OS handles.
They support String/File constructors, append and truncate modes, byte/array/
slice I/O, close, and seekable input skip/available. Writes loop until complete;
reads may return partial results. Host descriptors are unbuffered. Explicit
close, unreachable stream collection and every VM return/abort release owned
handles. Directory handles are also tracked, and close before Java callbacks.
FileOutputStream inherits the original OutputStream.flush no-op. Descriptor
constructors, getFD, channels and shared descriptors are unsupported. Pipes and
nonseekable input lack full semantics. InputStreamReader's existing malformed
UTF-8 pushback limitation on custom streams also applies to file input; this is
not a complete Reader implementation.

## Ndless boundary

The ARM build links SDK stat, open/read/write/seek/close, directory enumeration,
mkdir, rename/remove and realpath. SDK descriptors use OS stdio syscalls, so
buffering and visibility before close require device testing. Target fstat
returns ENOSYS; directories are checked before open. Host code additionally
checks the opened descriptor. Target stat only initializes timestamp seconds;
the adapter zeroes the structure and reads those supported fields. Target seek
range is constrained by off_t.

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
tools. Three programs compare output and actual disk contents in isolated
fixtures: paths, UTF-16 search and symlinks; attributes/mutation/filter callbacks;
and streaming/append/truncation/errors/Unicode filenames. Two VM checks open
400 abandoned streams per run under a 64-descriptor limit, exercise GC and a
fatal abort, and inspect disk bytes. The optional sixth check calls vm_run six
times in the same host process and counts /proc/self/fd after each normal/fatal
return. All passed normally and with ASan/UBSan/leak detection, alongside the
existing regression suite. See FILE-RESULTS.txt.

Original Logback headerBytes now returns bytes matching Java 17 with the same
configured line separator. Class.getInterfaces returns a fresh array of direct
interfaces; a separate oracle test covers source order, inherited-only
interfaces, arrays, primitives, annotations and reflective calls. Whole Xinbot
now reaches missing java/time/ZoneId in CachingDateFormatter, before Xinbot.main.
No full application or calculator run has completed. The unmodified JNI files
beside the canonicalizer are references and are not compiled.
