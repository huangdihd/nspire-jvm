# Unicode casing and substring search

`String.toLowerCase` and `toUpperCase` implement Unicode 13 mappings from a
pinned open-source JDK 17. Both explicit Locale arguments and the default
Locale are supported. Mappings include supplementary code points, multi-character
expansions, Turkish/Azeri dotted-I rules, Lithuanian combining marks and Greek
final sigma. Unpaired surrogate units and embedded NULs are preserved. Unchanged
strings can return the original object, and a null Locale throws a Java NPE.

Final sigma uses JDK 17's ROOT word-boundary state machines and UTF-16
`isBoundary` traversal, including its handling of supplementary characters.
Thai's dictionary-based word boundaries are not provided: lowercasing a Thai
Locale string containing capital sigma produces an explicit VM diagnostic.
This is not a general BreakIterator API implementation.

Character simple lower/upper/title mappings and lower/upper/title predicates
use the same Unicode version. String equalsIgnoreCase and compareToIgnoreCase
apply locale-independent simple mappings; the latter preserves comparison
ordering without promising JDK-specific nonzero result magnitudes. Neither
comparison expands sharp-s to `SS`.

`runtime/nspire/java/util/Locale.java` is an original small value/default API:
constructors, language/country/variant, constants, equals/hashCode/clone,
getDefault and setDefault. A fresh VM uses `user.language=en` and empty country,
independent of the host's operating-system locale. Properties read before the
first Locale initialization can supply another default. Locale service providers,
category-specific defaults, language-tag parsing, extensions, localized display
names, serialization and security-manager behavior remain unimplemented.

Substring contains/indexOf/lastIndexOf count UTF-16 units, including surrogate
halves. Contains performs the CharSequence's actual toString call, preserving
overrides, exceptions and GC effects. This is literal searching, not regex.

## Reproduce the tables

The build-time runtime is Eclipse Temurin `17.0.20.1+1`, Linux x64 JRE, identified
by its upstream URL and archive SHA-256 in `vendor/openjdk17-casing/SOURCES.json`.
It is not installed on or required by the calculator. Existing checked-in tables
let ordinary builds run without downloading a JRE or regenerating them.

```sh
python3 tools/generate-case-data.py --java /path/to/temurin17/bin/java
```

The generator queries the actual JDK 17 APIs and internal ROOT break tables.
It requires javac 17+ and the specified JDK 17 runtime. Sources and notices are
preserved in `vendor/openjdk17-casing/`; the generated data and adapted C boundary
code use GPLv2 with the Classpath exception. See THIRD-PARTY.md.

## Verification

```sh
python3 tools/test-case.py --vm build/nspire-jvm
python3 tools/test-loader.py --vm build/nspire-jvm
```

The casing suite checks String lower/upper transformations for every Unicode
code point in bounded batches, Character mappings/properties over all valid
code points plus invalid boundary values, 5,832 constructed sigma contexts,
six Locale choices, expansions, malformed surrogate sequences, null arguments,
unchanged-string identity and Locale values. Reference results come from an
actual standard JDK 17. The substring suite covers UTF-16 offsets, extreme
fromIndex values, empty targets, NULs and user-defined CharSequence conversions.
These are host tests; calculator execution remains unverified.
