# Active objective and next work

Objective remains: run actual Xinbot on a TI-Nspire CX II CAS through Ndless.
The current interpreter and passing sample programs do not achieve that objective.

2026-09-21 progress:
- Class mirrors, basic Class APIs and supplemental bootclasspath are implemented.
- Cooperative Thread/Runnable execution, monitors, wait/notify, join/sleep and
  interrupt are supported with GC roots for suspended threads. Main-thread join,
  discarded native thread records and fatal errors on child stacks were fixed.
- Added ThreadLocal and InheritableThreadLocal: per-thread values, initialValue,
  construction-time childValue, weak-key cleanup and clearing on termination.
- Imported 211 unchanged OpenJDK 8 source files with their full license notices,
  pinned revision and per-file hashes; build with tools/build-runtime.py.
- Implemented checked Unsafe field/array handles, 32/64-bit/reference atomics,
  park/unpark, declared-field lookup, string hashing/comparison and properties.
- Added bootstrap/application loader objects, context-loader inheritance, actual
  classpath resources, UTF-8 readers, Class.newInstance and OpenJDK ServiceLoader.
  Custom loader namespaces and defineClass remain unsupported.
- Added Integer boxing/cache, Cloneable copies, copy-on-write collections and
  limited StringConcatFactory bootstrap support with virtual toString conversion.
- Added Properties/Hashtable, TimSort/legacy/primitive sorting, Boolean boxing,
  float/double min/max and bit conversions, String constructors/interfaces and
  a limited String.format implementation (%s, %%, %n, indexing/width/precision).
- Added constructor lookup/invocation, access flags, parameter mirrors,
  Integer/Boolean/Long/Double unboxing/widening and InvocationTargetException wrapping.
  Initialization precedes argument conversion, verified against standard Java.
- Real Xinbot reads version properties, emits log status messages, sorts and
  instantiates configurators and opens its XML resource through URLConnection.
  It now executes its initial lambda bootstrap and parses the XML. The next
  failure is missing Charset during StringToObjectConverter property analysis,
  after appender creation and bean discovery, still before Xinbot.main.
- Added read-only classpath file/JAR connections, settings, content length and
  close semantics, including uncached JAR streams closing their sibling streams.
  Resource contents are bounded memory snapshots; classpath JARs must remain
  unchanged during a run. HTTP and general URL creation remain unsupported.
- Added actual SAX parsing through Expat 2.8.4, with Java callbacks, namespaces,
  UTF-8/UTF-16, internal entities, disabled external entities, errors and cleanup.
  The unchanged Logback SaxEventRecorder inside Xinbot produces the same 27
  events for its original logback.xml as standard Java in a component test.
  Whole-application startup now also reaches the parser, then fails in model setup.
  Parser limits and gaps are documented in XML-SUPPORT.md.
- Added lambda capture classes and interpreted bytecode adapters, marker/bridge
  support, primitive widening, reference adaptation and constructor/method refs.
  Default interface methods dispatch by specificity and initialize correctly;
  private method references remain nonvirtual on Java 17 bytecode.
  Serializable lambdas and general method-handle APIs remain unsupported.
- Added the single-character String.split fast path with limit/empty-field and
  UTF-16 behavior; other regular expressions now use the OpenJDK regex engine.
- Added Unicode 13 String case conversion, explicit/default Locale arguments,
  contextual sigma and Turkish/Azeri/Lithuanian rules; JDK 17 ROOT word breaks
  are preserved as generated tables plus an adapted C traversal. All code points
  and 5,832 sigma contexts were checked against JDK 17. See CASE-SUPPORT.md.
- Added literal substring contains/indexOf/lastIndexOf with UTF-16 positions and
  actual CharSequence.toString calls; imported original Stack/Vector classes.
- Added original OpenJDK stream pipelines, function interfaces, optional values,
  collectors/statistics, comparators and enum collections. Sequential object and
  int/long/double stream tests pass, including genuine short-circuit execution.
  Parallel execution, serializable-lambda factories and newer Stream APIs remain.
- Added GC-rooted enum universe/name caches, defensive public copies, Enum.valueOf
  support, canonical/declaring class names, Long/Double boxing and conversions,
  integer/long bit operations and CharSequence StringBuilder append/setLength.
  Double object formatting is explicitly unsupported. See STREAM-SUPPORT.md.
- 45 basic checks, 6 OpenJDK runtime runs and 11 loader/service/reflection checks passed
  on ordinary and ASan/UBSan/leak-detection builds. Tests include concurrent CHM
  resizing, tree bins, atomic counts, queues, locks, resources and provider errors.
- Three SAX test runs and the real Logback XML component run also passed on
  ordinary and ASan/UBSan/leak-detection builds, including an active-parser VM
  abort and nested readers with thread callbacks and a 64 KiB Java heap.
- Two lambda runs (javac releases 8 and 17) passed with normal and instrumented
  builds, using a 64 KiB heap. See LAMBDA-SUPPORT.md for limits and coverage.
- Three case checks passed with ordinary and instrumented builds, including
  an explicit unsupported-Thai-boundary diagnostic and its resource cleanup.
- Six stream/enum/boxing checks also pass with ordinary and instrumented builds;
  negative cases verify missing parallel support and Double formatting fail clearly.
- Added actual runtime annotation attributes and defaults, member accessors,
  primitive/reference/enum/nested/array values, inheritance, repeated annotations,
  equality/hash and deferred errors for evolved annotation definitions. Four
  differential tests, actual Logback phase comparison and a text-failure check
  pass on ordinary and ASan/UBSan/leak-detection builds, as does the full existing
  regression suite. See ANNOTATION-SUPPORT.md for the remaining reflection gaps.
- Increased the class table from 512 to 2,048 after actual Xinbot exhausted it
  during handler creation; the 16 MiB metadata budget remains enforced.
- Added original OpenJDK regex parsing/matching, StringBuffer/AbstractStringBuilder,
  Appendable and AtomicBoolean. String regex APIs delegate to the actual engine.
  Added UTF-16/code-point operations and Unicode 13 property tables, verified
  over all code points, plus numeric string parsing and native environment lookup.
  Normalizer and Unicode script/block dependencies remain absent. See REGEX-SUPPORT.md.
- Actual Logback Duration produces the same values/text/exception classes as
  standard Java. Xinbot passes regex compilation and property substitution and
  reaches console appender construction during second-phase model processing.
- Eight regex and supporting-runtime checks passed on ordinary and
  ASan/UBSan/leak-detection builds, alongside all existing regression suites.
- Imported original OutputStream, FilterOutputStream, BufferedOutputStream,
  ByteArrayOutputStream, Flushable and InterruptedIOException. Implemented
  PrintStream's native UTF-8 and console adapter with ordinary virtual dispatch,
  callbacks, monitor ownership, close/error state, redirection and byte output.
  Throwable suppressed lists support the original Java 8 filter close path.
  The runtime JAR is now required even for the basic demo; its generated config
  and the basic-test commands include the bootclasspath. See OUTPUT-SUPPORT.md.
- Original Logback ConsoleTarget wrappers write the expected bytes and retain
  their dynamic System.out/err lookup. Actual Xinbot passes this console setup.
- Seven output checks and the complete existing regression suite passed on
  ordinary and ASan/UBSan/leak-detection builds. Direct original-Xinbot runs of
  both hosts stopped at missing Charset; the instrumented run had no sanitizer
  errors. The output tests document the known ASan ucontext warning separately.
- ARM Zehn was rebuilt and inspected. No calculator or firmware-emulator run
  has happened; successful linking does not prove device behavior.
- Added public/declared method discovery and real reflective invocation with
  access checks, primitive widening, virtual dispatch, synchronization, static
  initialization and target-exception wrapping. Added primitive wrappers and
  basic package lookup. See METHOD-SUPPORT.md for remaining limitations.
- Intrinsic reflection declarations come from a pinned Java 8 API table with
  provenance and licenses. This metadata does not implement unavailable APIs.
- Nine method/package checks pass on ordinary and ASan/UBSan/leak builds,
  alongside all existing suites. Actual Logback bean discovery and real setter
  calls match standard Java. Whole Xinbot now reaches missing Charset.

Next work:
1. Implement real Charset support for Logback's StringToObjectConverter and
   encoding paths, using the original bytecode to identify required conversion
   and buffer behavior. Do not substitute an empty class or skip configuration.
   Loader namespaces, general reflection and additional I/O APIs are still partial.
2. Replace the Ndless timing backend: the SDK's _gettimeofday reads RTC seconds
   and returns tv_usec=0. Current host CLOCK_MONOTONIC tests do not validate
   the calculator's precision or behavior when its wall clock changes.
3. Expand Java native/library coverage from actual failures. Imported methods
   still require APIs not provided here (parallel/newer streams, serializable lambdas, fork/join,
   serialization and others). Class-init failure semantics also remain partial.
4. Dynamic invocation, reflection, NIO/TLS, plugin loading and the calculator's
   real network transport remain outstanding. The user's intended network
   connection has been asked about; do not assume an answer.
5. Verify on device or a suitable emulator and measure memory/performance.

Xinbot execution must remain on the calculator. A future network bridge may
forward transport only; running Xinbot remotely does not achieve this objective.
