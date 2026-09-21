# Record bytecode and version parsing

The interpreter now loads java.lang.Record through an original MIT adapter
with its protected constructor and three abstract Object-method declarations.
Record constructors and explicitly declared accessors/methods execute their
actual bytecode. Class.isRecord checks the retained Record classfile attribute,
the final flag and direct Record superclass; it does not identify a class from
its name alone. Preview classfile versions remain unsupported.

The ObjectMethods bootstrap emitted by javac is an intrinsic. It resolves and
caches the actual record class and REF_getField handles, validates the call-site
descriptor, and uses the declared bootstrap order. Equality checks record type
and component values, with reference identity/null shortcuts and real virtual
equals callbacks. Arrays retain reference equality. Float/double NaNs are
canonicalized and signed zeros remain distinct. Hashes use the Java 17
31-based combination with unsigned C arithmetic and real component hashCode
callbacks. User-declared accessors are not substituted for field handles.

Generated text includes the simple class name, bootstrap component names and
actual field values. Primitive float/double conversion executes the unchanged
OpenJDK FloatingDecimal/FDBigInteger implementation, with FloatConsts and
DoubleConsts; the runtime now preserves 451 Java sources. References call their
actual toString, including exceptions and null results. GC roots remain on the
interpreter call-site stack, and temporary text buffers are released on Java
exceptions as well as normal return. The shared virtual-call helper now resolves
array Object methods through Object rather than seeking array-specific natives.

The preserved decimal algorithm has a power-of-five cache. Its initialization
exceeded a 256 KiB VM heap in testing; the broad floating test uses 2 MiB, below
the default 8 MiB. This does not establish memory use on the calculator.

Scope: javac-style invokedynamic with instance field getters on final record
classes. Arbitrary method-handle accessors, direct ObjectMethods API calls,
constant-dynamic bootstraps, record serialization, getRecordComponents and
RecordComponent/type-use/generic reflection are not implemented. Invalid or
unsupported bootstrap metadata produces the existing fatal VM diagnostic;
catchable BootstrapMethodError/linkage-error wrapping is not yet implemented.
Intrinsic Class reflection declarations still come from Java 8, so reflective
discovery of newer Class APIs is also incomplete. Primitive record formatting
does not fill the separate boxed Float/Double text or full Formatter gaps.

## Integer conversion needed by Xinbot

Integer.parseInt and Long.parseLong now handle String inputs with implicit
decimal or explicit radix 2 through 36. The parser uses actual UTF-16 units and
Unicode 13 digit data, supports ASCII signs, rejects whitespace and prefixes,
and detects overflow before unsigned arithmetic. It returns both minimum signed
values without signed C overflow. Failures throw NumberFormatException; messages
are compared with Java 17. Radix toString overloads for Integer/Long are also
implemented, with Java's decimal fallback for an invalid radix. Unsigned parsing,
CharSequence-range overloads and the remaining wrapper factories are outside
this change.

## Verification

```sh
python3 tools/test-records.py --java /path/to/java17/bin/java --xinbot /path/to/xinbot-2.4.3-RELEASE.jar
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-records.py --java /path/to/java17/bin/java --xinbot /path/to/xinbot-2.4.3-RELEASE.jar --vm build/nspire-jvm-asan
```

RECORD-RESULTS.txt records eight checks: record behavior (including callback
order, GC and exceptions), 312 float/double pairs, integer parsing/formatting,
three malformed bootstrap cases, a removed Record attribute, and the original
Xinbot Version class. The latter parses actual version strings and executes
accessors, comparisons, generated equals/hashCode and an invalid-version path.
It does not exercise Version's custom numeric String.format toString method.

All eight checks and the existing full regression suite pass on ordinary and
ASan/UBSan builds with leak detection. The Ndless ARM build and ELF/Zehn
inspection also complete; neither establishes a calculator runtime result.

Unchanged Xinbot now reaches Xinbot.main and fails in LangManager.<clinit> at
Map.of. The current VM error names Object.of because its missing-static-method
fallback loses the symbolic interface owner; javap confirms the original call
is java/util/Map.of with four key/value pairs. See XINBOT-RUN.txt. Complete
startup, networking, dynamic JNI and calculator execution remain unverified.

Primary references: [Record API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/Record.html),
[ObjectMethods API](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/runtime/ObjectMethods.html),
[OpenJDK 17 bootstrap implementation](https://github.com/openjdk/jdk17u/blob/jdk-17%2B35/src/java.base/share/classes/java/lang/runtime/ObjectMethods.java).
