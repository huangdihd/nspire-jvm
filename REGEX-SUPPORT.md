# Regex and supporting runtime APIs

The supplemental library includes unchanged OpenJDK 8 Pattern, Matcher,
MatchResult, PatternSyntaxException, ASCII and UnicodeProp sources at the
revision recorded in `runtime/openjdk8/SOURCES.json`. Regex parsing and
matching execute their actual Java bytecode. String.matches, replaceAll,
replaceFirst and the general String.split path call that implementation;
single-character literal splitting retains its existing fast path.

The differential tests cover capture groups and named groups, backreferences,
alternation, quantifiers, lookarounds, character-class intersections, quoting,
case/multiline/dotall/comments/literal flags, Unicode categories and Java
character properties, supplementary characters, match regions and bounds,
replacement expansion, splitting, splitAsStream/asPredicate, custom
CharSequence callbacks with GC and syntax/state/replacement errors.

The original StringBuffer and AbstractStringBuilder bytecode supplies the
synchronized buffer used by Matcher replacement and syntax-error messages.
Native additions cover character copying, code-point String construction,
StringBuilder char-array append and capacity construction, and Integer/Long
decimal digit helpers. General StringBuilder and StringBuffer API coverage
remains limited by other unavailable runtime methods.

`src/character.inc` supplies UTF-16 surrogate/code-point traversal, char-array
and CharSequence overloads, offset/count operations, Unicode categories,
alphabetic/ideographic/letter/digit/space/mirroring/identifier properties and
Character.digit. Properties are Unicode 13, generated from the pinned open-source
Temurin 17 runtime already used for case mapping. All valid code points and
invalid boundary values are compared with Java 17; surrogate and array-bound
tests also compare observable partial writes on failing toChars calls.

Reproduce the table (the result must match its checked-in SHA-256):

```sh
python3 tools/generate-character-data.py --java /path/to/temurin17/bin/java
```

The additional `character.inc` data retain GPLv2 with the Classpath exception;
the original adapter and generator retain this project's MIT license. See
`vendor/openjdk17-casing/NOTICE`, LICENSE and SOURCES.json.

Double.parseDouble, Double.valueOf(String) and Float.parseFloat validate Java's
decimal/hexadecimal grammar, signs, suffixes, trimmed ASCII controls, NaN and
Infinity before libc conversion. Tests compare bits for subnormals, signed zero,
overflow, halfway rounding, malformed input and 500 deterministic decimal
inputs. Linux conversion passes; newlib's conversion still requires calculator
execution verification. NumberFormatException messages are generic, and this
does not implement floating-point output formatting or all wrapper APIs.

System.getenv(String) queries the native process environment, converts UTF-8
names/values and distinguishes absent and empty values. Its test supplies only
synthetic variables, including Unicode; it does not print unrelated environment
values. Java system properties are separate. The no-argument getenv map API,
process creation and environment editing are not implemented. Ndless's actual
process environment has not been tested on hardware.

AtomicBoolean is also imported unchanged. Existing Unsafe field/CAS support
executes its operations. The concurrent-library test checks two threads using
compareAndSet to protect shared updates, plus get/set/getAndSet/lazySet.

Known regex limits:

- Canonical equivalence requires the absent Normalizer classes; Unicode script
  and block properties require absent Character.UnicodeScript/UnicodeBlock.
  Explicit negative checks verify normalization and script lookup fail clearly.
- These are Java 8 regex sources, with the VM's Unicode 13 character data;
  newer regex APIs/features and Java 17 implementation changes are not implied.
- Regex recursion runs within the interpreter's class, heap, metadata, depth
  and instruction budgets. Exhausting a VM budget gives a VM diagnostic;
  adding StackOverflowError's type does not provide full Java stack-overflow
  recovery. This is not an untrusted-regex security sandbox.
- Serialization, all character encodings, all string methods and all numeric
  conversion APIs remain incomplete.

```sh
python3 tools/test-regex.py --vm build/nspire-jvm
python3 tools/test-regex.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
```

With the original application JAR, eight checks cover four Java differential
tests, actual Logback Duration parsing, controlled environment lookup and two
explicit unsupported paths. The real Duration component produces the same
values, text and exception classes as Java. The application's regex itself is
not corrected or simplified, including its treatment of unusual input.
All eight checks passed on ordinary and ASan/UBSan/leak-detection builds,
alongside the existing regression suites.

Whole Xinbot startup now reaches second-phase log configuration and constructs
its own JLineConsoleAppender. It next fails at Class.getMethods in
BeanDescriptionFactory during property setup, before Xinbot.main.
Neither this component success nor ARM linking proves calculator execution.
