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
- Imported 102 unchanged OpenJDK 8 source files with their full license notices,
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
  Integer/Boolean unboxing/widening and InvocationTargetException wrapping.
  Initialization precedes argument conversion, verified against standard Java.
- Real Xinbot reads version properties, emits log status messages, sorts and
  instantiates configurators and opens its XML resource through URLConnection.
  It now executes its initial lambda bootstrap and parses the XML. The next
  failure is String.toLowerCase in ElementSelector.hashCode while building
  configuration rules (see XINBOT-RUN.txt), before Xinbot.main.
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
  UTF-16 behavior; other regular expressions still report an explicit failure.
- 45 basic checks, 5 OpenJDK runtime runs and 10 loader/service/reflection checks passed
  on ordinary and ASan/UBSan/leak-detection builds. Tests include concurrent CHM
  resizing, tree bins, atomic counts, queues, locks, resources and provider errors.
- Three SAX test runs and the real Logback XML component run also passed on
  ordinary and ASan/UBSan/leak-detection builds, including an active-parser VM
  abort and nested readers with thread callbacks and a 64 KiB Java heap.
- Two lambda runs (javac releases 8 and 17) passed with normal and instrumented
  builds, using a 64 KiB heap. See LAMBDA-SUPPORT.md for limits and coverage.
- ARM Zehn was rebuilt and inspected. No calculator or firmware-emulator run
  has happened; successful linking does not prove device behavior.

Next work:
1. Implement String case conversion and continue through Logback XML model
   construction. Preserve real initialization; do not skip logging.
   Loader namespaces, general reflection and additional I/O APIs are still partial.
2. Replace the Ndless timing backend: the SDK's _gettimeofday reads RTC seconds
   and returns tv_usec=0. Current host CLOCK_MONOTONIC tests do not validate
   the calculator's precision or behavior when its wall clock changes.
3. Expand Java native/library coverage from actual failures. Imported methods
   still require APIs not provided here (streams, serializable lambdas, fork/join,
   serialization and others). Class-init failure semantics also remain partial.
4. Dynamic invocation, reflection, NIO/TLS, plugin loading and the calculator's
   real network transport remain outstanding. The user's intended network
   connection has been asked about; do not assume an answer.
5. Verify on device or a suitable emulator and measure memory/performance.

Xinbot execution must remain on the calculator. A future network bridge may
forward transport only; running Xinbot remotely does not achieve this objective.
