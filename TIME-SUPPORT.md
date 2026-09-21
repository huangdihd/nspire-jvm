# Time and timezone support

The interpreter executes preserved OpenJDK 8 java.time and calendar bytecode.
It does not substitute formatted timestamps or skip Xinbot's logging setup.
The complete application still fails in Jansi initialization at the missing
`java/io/FileDescriptor`, before `Xinbot.main`.

## Sources and platform bridges

- `runtime/openjdk8/` preserves 372 original Java files from OpenJDK revision
  `f826be1da079fb8d44055a0d86021d13748f9c36`, including time, calendar, Math,
  StrictMath, big-number helpers and SerializedLambda dependencies.
- TZDB 2026b is packaged as `/nspire/time/tzdb.dat`. The two adapted Java
  loaders change only how that database is opened. Their original decoder
  and rule algorithms remain intact; originals, notices and hashes are under
  `vendor/openjdk8-time/`.
- Linux host default-zone discovery reads TZ, `/etc/localtime`, then
  `/etc/timezone`; if no region is discovered, it can use the current GMT offset.
  It is not a complete parser for arbitrary POSIX TZ expressions.
- The host accepts `--timezone Asia/Hong_Kong`. Ndless accepts an optional
  fourth line in `jvm.cfg.tns`; its default is UTC. The VM does not discover
  a device timezone or change the calculator clock.
- CRC32 array/single-byte natives use miniz. Locale supplies DISPLAY/FORMAT
  defaults. Original Java code performs integer overflow checking, fractions,
  date arithmetic and TZDB decoding.
- `StrictMath.log` and `sqrt` use OpenJDK fdlibm. The sqrt adaptation replaces
  one negative signed left shift with equivalent representable multiplication.
  `tools/prepare-fdlibm.py` verifies originals and the generated adaptation;
  `--update` regenerates it. Compilation disables strict aliasing and fused
  floating-point contraction. Other native StrictMath functions remain missing.
- The String.replace(CharSequence, CharSequence) adapter invokes the original
  Pattern/Matcher with literal pattern and quoted replacement text. UTF-16
  StringBuilder deletion/replacement supports the date library's actual calls.

## Differential checks

`tools/test-time.py` compares output with standard Java, covering:

- 4,585 bit-pattern inputs for log/sqrt: signed zeros, infinities, NaNs,
  subnormal and normal boundaries, values around one and deterministic samples.
  Finite results and signed zeros must match exactly; NaNs are canonicalized
  using Java's doubleToLongBits.
- Checked integer arithmetic, Long/Integer signum, Locale category defaults,
  CRC32 updates and DataInputStream modified UTF-8, numeric values and EOF.
- Literal replacement, nulls, observable CharSequence.toString calls and GC;
  StringBuilder edits around null characters and individual surrogate units.
- Instants before and after epoch, millisecond/nanosecond fractions, leap days,
  year zero/negative years, ISO parsing, durations, periods and invalid inputs.
- UTC, Hong Kong, New York, Paris and fixed-offset formats, including historical
  offsets and both the spring gap and autumn overlap in New York.
- Host defaults under controlled TZ values and explicit timezone precedence.
- The unchanged CachingDateFormatter class from the real Xinbot release,
  including its cache behavior and default-zone constructor, against Java 17.
  This is a component check, not proof of complete application startup.

Build using Java 8 javac; JDK 17 javac rejects some preserved upstream sources:

```sh
python3 tools/build-runtime.py --java8-home /path/to/jdk8
make host
python3 tools/test-time.py --java /path/to/jdk8/bin/java
# Optional original Xinbot component check:
python3 tools/test-time.py --java /path/to/jdk8/bin/java \
  --java17 /path/to/jdk17/bin/java --xinbot /path/to/xinbot.jar
make build/nspire-jvm-asan
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-time.py --vm build/nspire-jvm-asan --java /path/to/jdk8/bin/java
```

## Remaining boundaries

Importing a class does not establish that all of its APIs work. Locale service
providers, language tags, localized text formats, several non-ISO chronology
resources, full BigInteger/BigDecimal coverage, generic object-stream
serialization, direct-buffer CRC32 and other StrictMath natives are incomplete.
Serializable lambda capture/replacement/relinking is covered separately in
`LAMBDA-SUPPORT.md`; no ObjectOutputStream/ObjectInputStream round trip is claimed.

TZDB is a snapshot, not an automatic update service. The calculator RTC still
has second resolution, and its epoch conversion, timezone configuration,
floating-point execution and memory usage require device verification.
Host differential tests and a successful ARM link do not prove those properties.
