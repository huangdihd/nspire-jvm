# Active objective and next work

Objective remains: run actual Xinbot on a TI-Nspire CX II CAS through Ndless.
It is NOT achieved by the current interpreter or by passing sample programs.

2026-09-21 progress:
- Implemented class mirrors, primitive/array class identity and basic Class APIs.
- Differential ClassTest covers static-init timing, nested/anonymous class names,
  array covariance, primitive identity, missing forName exceptions and GC identity.
- Added semicolon-separated classpaths and supplemental bootclasspath, including
  calculator config's optional third line. Built-ins remain VM-owned.
- Actual Xinbot now enters SLF4J initialization; next missing class is
  java/util/concurrent/ConcurrentHashMap (see XINBOT-RUN.txt).
- Implemented cooperative Thread/Runnable execution, reentrant monitors,
  synchronized methods/blocks, wait/notify, sleep/join/interrupt and GC roots
  for suspended threads. Host uses ucontext; Ndless uses ARM stack switching.
- Host 31 checks pass. The new threading changes have not yet been checked
  with ASan/UBSan; the earlier 26-check snapshot passed those checks.
- ARM build inspection is recorded in TARGET-RESULTS.txt. Successful linking
  does not establish that context switching works on the calculator.

Next meaningful implementation work:
1. Select a runtime-class source with clear per-file licensing; do not assume
   a repository-level MIT label applies to all imported Java classes.
   Inspected miniJVM's ConcurrentHashMap is GPLv2 with Classpath exception, but
   its ReentrantLock header carries a different older Sun notice. No runtime
   class sources from that candidate have been copied into this deliverable.
2. Validate ARM context switching and timing on device. Expand threading checks
   for lifecycle edge cases (including joining the main thread), class-init
   contention, failure cleanup and sanitizer fiber-stack integration.
3. Supply collections/concurrency runtime classes and required native atomic,
   parking and time operations; re-run the upstream JAR after each step.
4. Dynamic invocation/lambdas, reflection, service/resource discovery, NIO/TLS,
   plugin loading and a real calculator network transport remain outstanding.
5. Verify on device or a suitable emulator; neither has happened yet.

No claim is made that a boot JAR from desktop Java can just be copied in and work.
The loader currently supplies one class namespace, not loader-specific identity
or full parent delegation. The network transport must keep Xinbot execution on
the calculator as requested; remote execution is not a replacement objective.
