# JVM shutdown and temporary-file cleanup

The unchanged OpenJDK 8 Shutdown, ApplicationShutdownHooks and DeleteOnExitHook
classes execute their actual bytecode. IdentityHashMap and ThreadDeath are
also preserved from the same pinned revision; the runtime now contains 443
original Java sources. Per-file hashes and licenses remain in runtime/openjdk8.

System.exit and Runtime.exit run registered application hooks as interpreter
threads and wait for them, then run delete-on-exit cleanup. Natural termination
waits for the last non-daemon worker first. Hooks may themselves be daemon
threads; the shutdown sequence still waits for them. It rejects duplicate/live
hook registration, supports removal before shutdown, and rejects add/remove
during shutdown through the original Java state machine. A hook's uncaught
Java exception ends that hook and does not prevent the others from finishing.

File.deleteOnExit keeps unique paths in registration order and deletes them
in reverse order after application hooks. It can be first called from a hook.
This delegates to real File.delete; it does not claim deletion succeeded when
the underlying filesystem refuses it. Ndless deletion uses the existing SDK
adapter, which is compiled but has not been verified on a calculator.

Runtime.halt skips hooks and deletion, including when called during a hook.
Both exit and halt return their status through vm_run to its C caller; they do
not call the host process exit function. The Ndless entry can therefore display
the result and return to the calculator interface. Worker termination first
switches back to the root C stack, preserving sanitizer fiber bookkeeping.
The result is stored in the VM so it remains defined across longjmp. Owned
native handles and interpreter allocations are reclaimed on every return.

For natural shutdown, a separate DestroyJavaVM thread identity uses the root
C stack. The original Java main thread stays terminated: hooks can test its
liveness and join it. Explicit exit keeps its calling thread alive while it
runs hooks. Registered hooks and file paths are ordinary Java GC roots through
the preserved static fields; no independent C callback list substitutes for
the library state machine.

Limits remain: cooperative scheduling, 32 live threads and 256 KiB per worker
stack, no ThreadGroup/custom uncaught-exception dispatch, no SecurityManager,
no finalizer service, and no OS signal/shutdown-event integration. Unsupported
VM operations, instruction exhaustion and cancellation use fatal cleanup and
do not run Java hooks. Deadlocked Java hooks remain a possible failure; the VM
can diagnose a scheduler deadlock rather than wait indefinitely. beforeHalt
flushes the C console; there is no OS/profiler notification service in this VM.
These distinctions matter when comparing with the
[Java 8 Runtime API](https://docs.oracle.com/javase/8/docs/api/java/lang/Runtime.html#addShutdownHook-java.lang.Thread-).

## Validation

```sh
make host build/nspire-jvm-asan build/shutdown-lifetime build/shutdown-lifetime-asan
python3 tools/test-shutdown.py --java /path/to/java8/bin/java --lifetime build/shutdown-lifetime
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-shutdown.py --java /path/to/java8/bin/java --vm build/nspire-jvm-asan --lifetime build/shutdown-lifetime-asan
```

Ten subprocess scenarios compare status and real disk contents with Linux
Java 8. They check main/worker exit, halt before/during shutdown, uncaught main
and hook exceptions, concurrent hooks, daemon hooks, identity registration,
removal, registration failures, retained references during GC, main liveness,
worker draining, late file registration, duplicates and reverse deletion order.
Three VM-only cases check fatal errors before shutdown, in a hook and in a
worker after main has returned. An embedding harness runs twenty VMs in one
process under a 64-descriptor limit, checking statuses and /proc/self/fd counts.
The results are in SHUTDOWN-RESULTS.txt. They establish host behavior only.

## String joining and actual Xinbot

Both String.join overloads use a Java adapter and the preserved StringJoiner.
STRING-JOIN-RESULTS.txt records Java 8 comparisons for arrays and iterables,
null elements/arguments, UTF-16, callback exceptions, custom CharSequence and
iterator behavior, and GC during callbacks. Run tools/test-string-join.py with
--java and optionally --vm to reproduce them.

The unchanged Xinbot JAR now passes delete-on-exit registration and String.join.
Jansi extracts the exact original Linux x86_64 library, then handles the real
UnsatisfiedLinkError from unsupported dynamic JNI loading. Execution reaches
Logback's Method.getParameters call, which remains unimplemented. This occurs
before Xinbot.main. Because this is a fatal interpreter error, its extracted
files remain until the test driver's disposable-directory cleanup. This does
not demonstrate Jansi initialization, a working native library, network access
or calculator execution. See XINBOT-RUN.txt for the actual attempt.

A separate component check invokes the original JansiLoader.initialize from
that same JAR. It preserves the extracted bytes outside the temporary directory,
checks their exact identity with the embedded library, and observes both library
and lock removal after normal return and System.exit(17). An application hook
checks that deletion has not happened too early. No property disables Jansi or
changes its graceful-failure default. Reproduce with:

```sh
python3 tools/test-jansi-cleanup.py --xinbot /path/to/xinbot-2.4.3-RELEASE.jar
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-jansi-cleanup.py --xinbot /path/to/xinbot-2.4.3-RELEASE.jar --vm build/nspire-jvm-asan
```

JANSI-CLEANUP-RESULTS.txt records these two host component checks. Its successful
cleanup does not change the full Xinbot attempt's fatal-error stopping point.
