# Character encodings and heap buffers

The supplemental library now runs the original OpenJDK 8 Charset,
CharsetEncoder/Decoder, CoderResult and Unicode codec implementations. The
provider supplies US-ASCII, ISO-8859-1, UTF-8, UTF-16, UTF-16BE and UTF-16LE.
Aliases come from preserved upstream data. Charset name validation, lookup,
comparison, immutable alias sets, sorted enumeration, cached default charset
and application CharsetProvider discovery run as actual Java bytecode.
This implements the tested paths of the [Charset API](https://docs.oracle.com/javase/8/docs/api/java/nio/charset/Charset.html).

String.getBytes() and its charset-name/Charset overloads delegate to those
encoders. Byte-array String constructors, with offsets and either charset form,
delegate to the decoders. Replacement, malformed/unmappable results, REPORT,
REPLACE and IGNORE, streaming underflow/overflow, end-of-input, flush and reset
use the original coder state machines. Custom codecs run their actual callbacks,
including GC, exceptions and per-thread cached encoding. Default encoding is
UTF-8 under this VM's startup properties.

The preserved generator supplies heap byte/char/short/int/long/float/double
buffers, read-only variants, and big/little-endian byte-buffer views. Original
Buffer state checks, sharing, slice, duplicate, compact, bulk operations and
CharBuffer sequence/stream operations are present. Project Bits code performs
byte access through these Java buffers; it has no absolute-address interface.
Native byte order comes from a C probe saved at VM startup, independent of later
System.setProperty calls. Loading these sources does not make all NIO APIs work.

The codec caches use actual unqueued WeakReferences. The GC skips their referent
slot, marks other fields normally, and clears unreachable referents before
freeing them. get/clear and the unqueued enqueue/isEnqueued behavior are supplied.
ReferenceQueue registration, SoftReference, PhantomReference and finalization
are not implemented. Unsupported reference constructors still fail explicitly.

The original Java sources are pinned in runtime/openjdk8/SOURCES.json. Templates,
make rules, Spp, exception-generation scripts, alias data, licenses and 55 generated
Java files are preserved in vendor/openjdk8-nio. Reproduction runs the upstream
generators and checks input/output SHA-256:

```sh
python3 tools/generate-nio.py --java /path/to/java8/bin/java
python3 tools/build-runtime.py --java8-home /path/to/java8
python3 tools/test-charset.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-charset.py --vm build/nspire-jvm --java /path/to/java8/bin/java --xinbot /path/to/xinbot.jar
```

Use Linux/WSL for generation. javac may be the Windows JDK discovered by the
existing build tools; the generator's Java executable must be a Linux runtime.
--update refreshes generated hashes only for a reviewed generation change.
The generated classes retain GPLv2 with Classpath-exception terms. The project's
provider, Unicode containment adapter, heap Bits and String bridge are separate
MIT code; they do not modify the imported codecs or hide missing dependencies.

Six differential checks cover charset names/aliases, all six encodings,
supplementary characters and unpaired surrogates, byte-order marks, malformed
input, error precedence, coder state, shared/read-only buffers, primitive byte
views, weak-reference clearing, service discovery and custom codec callbacks
with threads and GC. The oracle is Java 8 because these are Java 8 codecs:
for example, their UTF-16 decoder treats a reversed byte-order mark differently
from Java 17. This difference is preserved rather than described as Java 17
compatibility. The test heap is 512 KiB.

A seventh check compares the unchanged Logback StringToObjectConverter,
reflective setCharset and LayoutWrappingEncoder.encode against Java 17 using
four encodings. An eighth check now compares actual headerBytes output with
Java 17 after both runs set the same LF line separator. It exercises the original
CoreConstants initialization, which now loads File successfully.
Successful body encoding is not successful logger or application startup.
All eight checks and the complete existing host suite pass on ordinary and
ASan/UBSan builds with leak detection. The rebuilt ARM ELF and Zehn structure
also pass inspection; this does not establish calculator runtime behavior.

Remaining gaps include additional built-in encodings, direct/mapped buffers,
NIO channels/files/selectors, non-UTF-8 InputStreamReader/PrintStream adapters,
general writers, reference queues and complete buffer/library API coverage.
No direct-memory operation is silently replaced with a heap allocation.

The original whole Xinbot JAR now passes the earlier missing-Charset point,
passes File initialization and component interface inspection, then stops at
java.time.ZoneId in the original date formatter, still before Xinbot.main.
Calculator and firmware-emulator
execution remain unverified.
