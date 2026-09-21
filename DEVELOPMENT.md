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
- Imported 67 unchanged OpenJDK 8 source files with their full license notices,
  pinned revision and per-file hashes; build with tools/build-runtime.py.
- Implemented checked Unsafe field/array handles, 32/64-bit/reference atomics,
  park/unpark, declared-field lookup, string hashing/comparison and properties.
- Real Xinbot now gets through ConcurrentHashMap, LinkedBlockingQueue and MDC
  initialization. The next failure is Class.getClassLoader during SLF4J service
  discovery (see XINBOT-RUN.txt); Xinbot.main has not been reached.
- 38 basic checks plus 2 OpenJDK runtime programs passed on ordinary and
  ASan/UBSan/leak-detection builds. Tests include concurrent CHM resizing, tree
  bins, atomic counts, blocking queues and reentrant lock conditions.
- ARM Zehn was rebuilt and inspected. No calculator or firmware-emulator run
  has happened; successful linking does not prove device behavior.

Next work:
1. Implement actual loader objects and JAR resource access for ServiceLoader.
   Do not return a fake successful logger binding or discard service providers.
   Loader namespaces, reflection constructors and provider discovery must work
   consistently with the real upstream JAR.
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
