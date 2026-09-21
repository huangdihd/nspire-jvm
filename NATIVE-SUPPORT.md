# Native linkage and library naming

Unbound native methods in loaded classfiles now throw the preserved OpenJDK
`UnsatisfiedLinkError`. Direct static/virtual/private/super calls, reflection
and generated lambda bridges use the same execution entry. Reflection wraps
the error in InvocationTargetException; an uncaught child-thread error ends
that thread while main can continue. Native argument references stay rooted
while the error is allocated. The existing VM bindings for filesystem, time,
CRC32, StrictMath log/sqrt, AtomicLong and Expat remain real native operations.

This corrects the earlier fatal interpreter abort for an unbound application
native method. Internal unsupported interpreter operations still report a VM
diagnostic, and the child-stack fatal cleanup test still exercises that path.
Full class-resolution, initialization-error and JNI semantics are not claimed.

`System.mapLibraryName` uses `lib<name>.so`. It preserves UTF-16 characters,
including embedded NUL and surrogate code units, rejects null, and uses the
OpenJDK 8 limit of 240 UTF-16 code units. The name convention is also used on
Ndless; it does not imply a shared-object loader exists there. Reference:
[pinned OpenJDK System.c](https://github.com/openjdk/jdk8u/blob/f826be1da079fb8d44055a0d86021d13748f9c36/jdk/src/share/native/java/lang/System.c).

**Dynamic JNI loading is not implemented.** System.load/loadLibrary and the
Runtime equivalents check null, absolute paths and library-name separators,
then report the actual unsupported capability as UnsatisfiedLinkError. They
never report a successful load or invoke a desktop library. This permits Java
code to handle a failed load through its own exception logic; it supplies no
working JNI environment, native symbols or library lifecycle.

The linkage exception behavior follows the invocation rules in the
[JVM specification](https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-6.html#jvms-6.5.invokestatic).
The public entry points are described in the
[Java 8 System API](https://docs.oracle.com/javase/8/docs/api/java/lang/System.html#load-java.lang.String-).
The Ndless SDK checked for this port is the pinned source in THIRD-PARTY.md;
neither its ARM executable loader nor a Linux .so is treated as a JNI loader.

## Temporary directory and integer conversion

`java.io.tmpdir` defaults to `/tmp` on the Linux host and the launch working
directory on Ndless. Supply `--tmpdir directory` on the host or an optional
fifth line in jvm.cfg.tns on the calculator. The existing fourth line is the
timezone; use a blank fourth line if only the fifth is needed. The directory
must already exist when an application needs to write into it. The VM preserves
UTF-8 command-line paths as Java UTF-16 strings and does not create or clear the
directory on startup. Tests and recorded Xinbot attempts use disposable task
directories so application cleanup stays within their own fixtures.

Integer/Long.toHexString, toOctalString and toBinaryString format unsigned bit
patterns, including negative arguments and the full 32/64-bit boundaries.
Jansi uses Long.toHexString when constructing its extracted library name.

## Verification

```sh
make host build/nspire-jvm-asan
python3 tools/test-native.py --java /path/to/java8/bin/java
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-native.py --vm build/nspire-jvm-asan --java /path/to/java8/bin/java
```

Six Linux Java oracle checks cover name mapping and limits; load-failure
classes; native invocation through direct/reflective/lambda paths; monitor
ownership, initialization and GC; uncaught child errors; real file I/O through
a Unicode temporary directory; and unsigned integer conversions. The VM uses
a 64 KiB heap in these tests. NATIVE-RESULTS.txt records the results. These
checks do not exercise a successfully loaded native library or calculator I/O.

The original Xinbot attempt now gets past mapLibraryName and the missing
temporary-directory property, and enters Jansi's normal resource extraction.
The current failure is missing java.nio.file.FileSystems in File.toPath,
before Files.copy can copy the embedded library. See XINBOT-RUN.txt. The
earlier Method.getParameters stopping point occurred only because the missing
temp-directory property caused a caught NullPointerException; it is not used
as evidence of successful Jansi initialization.
