# Output streams and console integration

OutputStream, FilterOutputStream, BufferedOutputStream, ByteArrayOutputStream,
Flushable and InterruptedIOException now execute unchanged OpenJDK 8 sources
at the revision in `runtime/openjdk8/SOURCES.json`. Byte-array growth, buffer
draining, bounds checks and FilterOutputStream's try-with-resources close are
the original Java bytecode. Their GPLv2 + Classpath-exception notices are preserved.

`src/output.inc` is an original C PrintStream adapter. Its superclass is the
actual FilterOutputStream; it implements Appendable and inherits Closeable and
Flushable. Native method declarations participate in ordinary virtual dispatch,
including Java subclasses overriding write. Native calls root their arguments;
printing holds the stream monitor while invoking callbacks that can allocate,
yield, throw or reenter the stream.

Supported paths include:

- OutputStream constructors with optional autoFlush and UTF-8 encoding name;
  raw integer and byte-array writes, slices, flush, close, checkError and
  protected setError/clearError. Unsupported constructors fail explicitly.
- Text and character-array printing, println, append and the existing limited
  format/printf conversions. UTF-16 surrogate pairs become real four-byte UTF-8;
  NUL becomes a zero byte. A pending high surrogate survives flush and text-call
  boundaries. Unpaired surrogates use `?`; close drains any pending surrogate.
- Actual writes to the selected Java OutputStream or the host/SDK stdout and
  stderr handles, with IOException error flags and InterruptedIOException's
  write-time interrupt handling. Other Java exceptions propagate.
- System.setOut/setErr, including original references keeping their destination
  after replacement. Closing a Java console stream makes subsequent Java output
  fail through checkError. Native console handles are borrowed, so close flushes
  them without closing the VM's own diagnostic/UI handle.
- Throwable.addSuppressed/getSuppressed with null/self rejection, GC retention
  and defensive arrays. This supports the imported Java 8 close implementation;
  Java 17's changed FilterOutputStream close ordering is not substituted.

The runtime JAR is now required even for the basic console demo. Transfer
`runtime.jar.tns` alongside the executable, demo and three-line `jvm.cfg.tns`.
Host invocations likewise need `-bootclasspath dist/runtime.jar.tns`.

Reproduce the output checks:

```sh
python3 tools/test-output.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-output.py --vm build/nspire-jvm --java /path/to/java8/bin/java --xinbot /path/to/xinbot.jar
```

Five Java 8 differential runs cover memory/buffering/filtering, text and raw
output, exact stdout/stderr bytes including all 256 byte values, redirection,
close and error paths, autoFlush counts, surrogate boundaries, protected error
state, subclass callbacks, GC, large text and two concurrent writers. The test
uses a 128 KiB Java heap, or 512 KiB for the concurrent/large-text case. An
additional negative check verifies the unimplemented filename constructor fails
without creating a file. With the original Xinbot JAR, a seventh check executes
Logback's unchanged ConsoleTarget wrappers and compares with Java 17; it checks
write overloads, flush, close and replacement of System.out after obtaining the
wrapper. It does not invoke Xinbot's main method or bypass its logging setup.
All seven checks and the existing regression suites passed on ordinary and
ASan/UBSan/leak-detection builds. The concurrent output check removes only ASan's
exact known makecontext/swapcontext warning from the stderr comparison; it still
rejects sanitizer errors and compares all other bytes. This warning also limits
what sanitizer results can establish about the host context-switch mechanism.

Known limits include file-backed streams, Charset-object constructors and
non-UTF-8 encodings, the complete Formatter API, PrintWriter and general writers.
Existing float/double print conversion still uses libc formatting rather than
Java's exact numeric-to-text algorithm. Byte-based String constructors now use
the actual Java codecs described in CHARSET-SUPPORT.md. Native text
chunks are bounded independently of OpenJDK's encoder buffer size; callback
chunk boundaries for long text are not identical. The full Throwable stack,
suppression-disabled constructors and serialization remain incomplete.

The actual application now creates its JLineConsoleAppender, completes bean
discovery and next stops at missing File during property configuration. This
is before Xinbot.main. No calculator or firmware-emulator execution is proven;
host byte output does not establish the Ndless console's glyph rendering.
