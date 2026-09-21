# Supplemental class library

The sources in `openjdk8/` are preserved upstream OpenJDK sources, not a complete
runtime. The build deliberately packages only classes compiled from these files.
Missing classes, unimplemented natives and unsupported invokedynamic bootstraps
still fail explicitly; supported lambda paths are described in LAMBDA-SUPPORT.md. A limited StringConcatFactory
intrinsic handles strings/references and integral primitives, not float/double.

Build with a Java 8 JDK compiler and its compatible API signatures:

```sh
python3 tools/build-runtime.py --java8-home /path/to/java8
python3 tools/test-runtime.py --vm build/nspire-jvm
python3 tools/test-loader.py --vm build/nspire-jvm
```

Neither the build-time rt.jar nor desktop HotSpot runs on the calculator.
The resulting `dist/runtime.jar.tns` is interpreted by the Ndless C interpreter.
See `openjdk8/SOURCES.json` for the exact revision, paths and source checksums.
Licensing is described in `openjdk8/LICENSE` and the repository's THIRD-PARTY.md.

The differential programs cover map resizing and tree bins, equal string keys,
concurrent insertion and count updates, 32/64-bit and reference atomics, blocking
queue operations, timed polling, reentrant locks, conditions and parking.
They also cover copy-on-write list snapshot iterators and cloning, resource
lookup across JARs/directories, UTF-8 service descriptors, lazy provider creation,
duplicate filtering, reload, context loader inheritance and provider errors.
Additional programs exercise Properties.load with Latin-1/UTF-8 readers,
escapes, continuations, long lines and defaults; stable object and primitive
sorting; actual PrivilegedAction execution; and constructor reflection including
initialization order, argument checks, access and target exception wrapping.
They do not establish Java SE compatibility, serialization support, or support
for all methods present in the JAR (including streams, lambdas and fork/join).
The VM has no protection-domain/SecurityManager policy. The no-context
doPrivileged(PrivilegedAction/PrivilegedExceptionAction) overloads execute the
provided action; the latter wraps checked exceptions using preserved Java code. Other access-control operations
remain unsupported. Properties.store and XML persistence also need further I/O.

Original InputStream, FileDescriptor and file streams are now included. Rebuild
the supplemental runtime when rebuilding the native VM: standard console
initialization depends on those classes. File/console limits, alias lifetime
and host-versus-calculator behavior are described in FILE-SUPPORT.md.


NIO paths, stream copying and a platform seekable channel are described in
NIO-FILE-SUPPORT.md. The runtime preserves the original Files API and channel
stream adapters; unsupported FileChannel, selector, watch and attribute paths
remain incomplete. NspirePath is adapted OpenJDK code with its original license.


Preserved OpenJDK shutdown hooks and delete-on-exit now execute in the VM.
The runtime includes IdentityHashMap and ThreadDeath dependencies; native
lifetime integration and host verification are described in SHUTDOWN-SUPPORT.md.
StringOperations.join delegates both overloads to the original StringJoiner.
