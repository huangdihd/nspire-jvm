# NIO paths, stream copies and seekable files

The original OpenJDK `FileSystems`, `Paths`, `Files`, provider API, file
exceptions, URI implementation and channel stream adapters execute as Java
bytecode. `DefaultFileSystemProvider` selects the interpreter's actual
filesystem adapter. `File.toPath` keeps its original cached-path behavior.

`NspirePath` preserves the lexical algorithms of the pinned OpenJDK UnixPath:
normalization, components, resolve/relativize, prefixes/suffixes, comparison,
hashing, iteration and strict UTF-8 encoding. The exact original and notices
are in `vendor/openjdk8-path`. Adaptations rename the platform classes, use a
weak encoder cache, route real-path/attribute operations through the provider,
use File/URI conversion, and explicitly reject unimplemented watch services.
Both platforms use `/` paths and UTF-8; arbitrary non-UTF-8 Unix names are not
claimed. `sun.jnu.encoding` describes this actual encoding.

The provider supplies real input/output streams and a seekable byte channel.
Native opens attach to the same owned FileDescriptor objects as java.io.
Explicit close, GC and VM destruction release these handles. Reads/writes,
positions, sizes and host truncation operate on the actual file. Buffer limits,
read-only buffers, closed channels and incompatible open options are checked.
`CREATE_NEW` uses the host's atomic O_CREAT|O_EXCL; append does not truncate an
existing file. Host SYNC/DSYNC and NOFOLLOW_LINKS map to the corresponding open
flags. DELETE_ON_CLOSE unlinks the host file after opening it.

The preserved `Files.copy(InputStream, Path, options)` performs its own option
validation, replacement, byte-copy loop and try-with-resources cleanup. It does
not close the caller's input. It can leave a partial output after an input error,
as specified by the API. Path-to-output-stream copying and readAllBytes also
execute the original Java implementation. **Path-to-path copy and NIO move
remain unsupported.**

Directory creation/deletion, filtered directory iteration, access checks and
basic attributes use real filesystem operations. Directory iteration snapshots
the names, applies the filter lazily and permits only one iterator. Host file
identity uses device/inode values. Basic timestamps preserve host nanoseconds;
creationTime falls back to lastModifiedTime where birth time is unavailable.
The attribute view captures its link options when constructed. fileKey returns
null. Permission/owner views, attribute writes, FileStore and watch services are
not implemented; no success is fabricated for them.

## Ndless boundaries

ARM compilation and ELF/Zehn structure checks are separate from device tests.
No calculator execution is verified. The pinned SDK's `_open` reverses its
O_CREAT|O_EXCL existence check; there is no verified atomic-create primitive.
The target therefore rejects CREATE_NEW instead of silently truncating an
existing file. Target DELETE_ON_CLOSE, NOFOLLOW_LINKS, SYNC/DSYNC, read/write
open with truncation, channel truncate, link attributes, distinct-path file
identity and access-mode checks also fail explicitly. Target timestamps have
the SDK's second resolution. These gaps still need platform implementation
and device evidence; host tests do not resolve them.

The channels are synchronous and non-selectable. Asynchronous interruption,
FileChannel locking/mapping, sockets, selectors and generic channel backends
are not supplied by this adapter. Imported declarations are not proof that
all their methods have executable dependencies.

## Verification

```sh
make host build/nspire-jvm-asan build/file-lifetime build/file-lifetime-asan
python3 tools/test-nio-files.py --java /path/to/java8/bin/java --lifetime build/file-lifetime
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-nio-files.py --java /path/to/java8/bin/java \
  --vm build/nspire-jvm-asan --lifetime build/file-lifetime-asan
```

Three Linux Java 8 differential programs compare path/URI behavior, actual
copied bytes and disk side effects, replacement/error/partial-copy rules,
symbolic and hard links, directory filters, attributes, and seekable channel
positions/options/close behavior. The VM uses a 2 MiB heap. Two additional VM
checks exercise abandoned NIO streams/channels under a 64-descriptor limit,
with normal and fatal returns. The optional sixth check counts descriptors
across six normal/fatal VM invocations and six console-close invocations in
one host process. Results are recorded in NIO-FILE-RESULTS.txt.

The path checks exposed a pre-existing StringBuilder(String) null-argument
bug; it now throws NullPointerException while Throwable(String) still permits
null. Both cases are compared with Java.

Original Xinbot now copies Jansi's embedded Linux x86_64 libjansi.so to disk:
18,976 bytes, SHA-256
`249095f2a73e3d2ab348d9f1de659947d253c18cf6a2049f75296b5a0d6ee079`.
The output matches the unchanged JAR entry byte for byte. It next stops at
missing Method.getParameters after delete-on-exit registration and JNI
load-failure handling, still before Xinbot.main. This host library
is not an ARM Ndless library and has not been loaded. See XINBOT-RUN.txt.

Sources: [Files.copy implementation](https://github.com/openjdk/jdk8u/blob/f826be1da079fb8d44055a0d86021d13748f9c36/jdk/src/share/classes/java/nio/file/Files.java),
[UnixPath](https://github.com/openjdk/jdk8u/blob/f826be1da079fb8d44055a0d86021d13748f9c36/jdk/src/solaris/classes/sun/nio/fs/UnixPath.java),
[UnixChannelFactory open/error behavior](https://github.com/openjdk/jdk8u/blob/f826be1da079fb8d44055a0d86021d13748f9c36/jdk/src/solaris/classes/sun/nio/fs/UnixChannelFactory.java).
