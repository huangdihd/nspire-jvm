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
- Imported 79 unchanged OpenJDK 8 source files with their full license notices,
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
  instantiates configurators and finds its XML resource. The next failure is
  URL.openConnection in GenericXMLConfigurator (see XINBOT-RUN.txt), still
  before Xinbot.main.
- 45 basic checks, 5 OpenJDK runtime runs and 8 loader/service/reflection checks passed
  on ordinary and ASan/UBSan/leak-detection builds. Tests include concurrent CHM
  resizing, tree bins, atomic counts, queues, locks, resources and provider errors.
- ARM Zehn was rebuilt and inspected. No calculator or firmware-emulator run
  has happened; successful linking does not prove device behavior.

Next work:
1. Implement classpath URLConnection and the actual XML configuration-reading
   path used by Logback. Preserve real initialization; do not skip logging.
   Loader namespaces, general reflection and additional I/O APIs are still partial.
2. Replace the Ndless timing backend: the SDK's _gettimeofday reads RTC seconds
   and returns tv_usec=0. Current host CLOCK_MONOTONIC tests do not validate
   the calculator's precision or behavior when its wall clock changes.
3. Expand Java native/library coverage from actual failures. Imported methods
   still require APIs not provided here (streams, lambdas, fork/join,
   serialization and others). Class-init failure semantics also remain partial.
4. Dynamic invocation, reflection, NIO/TLS, plugin loading and the calculator's
   real network transport remain outstanding. The user's intended network
   connection has been asked about; do not assume an answer.
5. Verify on device or a suitable emulator and measure memory/performance.

Xinbot execution must remain on the calculator. A future network bridge may
forward transport only; running Xinbot remotely does not achieve this objective.
