# Third-party code and build references

`vendor/miniz.c` and `vendor/miniz.h` were copied, without source changes, from
digitalgust/miniJVM at commit `ac94e62781deda037875ff69d78f272a327a72bc`:
https://github.com/digitalgust/miniJVM/tree/ac94e62781deda037875ff69d78f272a327a72bc/minijvm/c/utils

Their original copyright/license notices are preserved in the files. Only the
ZIP reader/inflater is used. This project does **not** use miniJVM's interpreter
or claim miniJVM's Java compatibility. Our VM implementation is in `src/vm.c`.

Ndless SDK build reference:
https://github.com/ndless-nspire/Ndless/tree/9484d8da7c7a4dde9766138c2e42e1d1e3acfcd4

nspire-io dependency:
https://github.com/Vogtinator/nspire-io/tree/d084a245286c073d161e1ed602788946aff79aee

Ndless and nspire-io are external build dependencies with their own licenses.
The source archive does not relicense either project. See BUILD-NOTES.md for
the exact task-local build procedure and any compatibility changes.

OpenJDK supplemental class library:
https://github.com/openjdk/jdk8u/tree/f826be1da079fb8d44055a0d86021d13748f9c36

`runtime/openjdk8/` contains 451 unmodified Java source files, their per-file
copyright notices, LICENSE (GPLv2 with the Classpath exception for these files),
ASSEMBLY_EXCEPTION and THIRD_PARTY_README. SOURCES.json records every upstream
path and SHA-256. These files are not relicensed under this project's MIT license.
`dist/runtime.jar.tns` is compiled from those sources using tools/build-runtime.py;
it embeds the notices and source manifest. The corresponding sources are in
this repository and the source ZIP. A separate Java 8 rt.jar is used only for
compile-time API signatures and is not redistributed or copied into the JAR.

`vendor/openjdk8-nio/` preserves templates, make rules, Spp, exception scripts,
charset alias data and 80 generated Java sources from that same fixed revision.
SOURCES.json records original and generated hashes. tools/generate-nio.py runs
the upstream generators; the compiled runtime embeds their notices and manifest.
These sources retain GPLv2 with the Classpath exception. Project charset-provider,
Unicode-containment, heap-Bits and StringCoding adapters in runtime/nspire are
separate MIT code. The imported Unicode codecs remain unmodified.

`vendor/openjdk8-file/` preserves the original Unix canonicalizer, two JNI
filesystem/stream reference files and their GPLv2 + Classpath-exception notices
at the same pinned revision. Only canonicalize_md.c is compiled, through an
original C wrapper. SOURCES.json records all six source/notice hashes. The
canonicalizer is not relicensed under MIT; the new filesystem bridge is original
MIT code. See FILE-SUPPORT.md for the host and Ndless boundaries.

The SAX sources also preserve their historical SAX notices. The OpenJDK source
headers and root license files remain intact. The additional Java adapter and
provider selector and Locale subset in `runtime/nspire/` are original MIT-licensed project code,
compiled into the same supplemental JAR with their own license and source hashes.

The adapted `runtime/nspire/sun/nio/fs/NspirePath.java` also retains GPLv2 with
the Classpath exception. Its exact UnixPath original and notices are preserved
in `vendor/openjdk8-path/`, with pinned source hashes embedded in the runtime.
The NIO provider, native bridge and channel adapter are original MIT project code.

Other exceptions in `runtime/nspire/` are `java/time/zone/TzdbZoneRulesProvider.java`
and `sun/util/calendar/ZoneInfoFile.java`: these are adapted OpenJDK files,
retain GPLv2 with the Classpath exception, and are not relicensed under MIT.
Only database loading was changed to the packaged `nspire/time/tzdb.dat`
resource. `vendor/openjdk8-time/upstream/` preserves their exact originals
from the pinned OpenJDK revision. The accompanying TZDB 2026b database comes
from the Temurin 8u504-b01 JRE; SOURCES.json records its download URL,
archive hash and individual file hashes. All source and data notices are
preserved in `vendor/openjdk8-time/` and embedded in the supplemental JAR.
The new `nspire.time.ZoneData` resource bridge is original MIT project code.

`vendor/openjdk8-fdlibm/` preserves the original log/sqrt algorithms, headers,
JNI mapping reference, build reference and notices from the same pinned
OpenJDK revision. The generated sqrt source changes one signed negative shift
to an equivalent representable multiplication; tools/prepare-fdlibm.py verifies
and regenerates that documented change. All upstream and generated source
hashes are recorded in SOURCES.json. These algorithms retain GPLv2 with the
Classpath exception and are linked into the native VM. The thin wrappers in
src/strictmath_log.c and src/strictmath_sqrt.c are original MIT code.

Intrinsic reflection API declarations are preserved in `vendor/openjdk8-api/`.
They are generated from the pinned Temurin 8u504-b01 build-time rt.jar and contain
names, descriptors, access flags and declared exceptions, not method bytecode.
Its NOTICE records the upstream artifact and source repository; SOURCES.json
records input/output hashes. GPLv2 with the Classpath exception and the original
license notices accompany the table. The original generator and C adapter use
this project's MIT license. This data does not implement every declared API.

Expat XML parser:

https://github.com/libexpat/libexpat/releases/tag/R_2_8_4

`vendor/expat/` contains unchanged library source files and upstream COPYING,
README.md and Changes from the official 2.8.4 release archive. Its SOURCES.json
records the archive URL, SHA-256 and per-file hashes. The original MIT notices
are preserved. Project-specific configuration is in `src/expat_config.h`;
the upstream source is not modified. Expat is statically linked into the host
and Ndless VM builds. The Java bridge is original code, not Xerces.

JDK 17 Unicode case data and word boundaries:

`vendor/openjdk17-casing/` preserves three unmodified OpenJDK source files,
GPLv2 + Classpath exception LICENSE, Unicode/ICU notices, generated tables and
source/runtime hashes. The data were generated with the open-source Temurin
17.0.20.1+1 runtime identified by its archive hash in SOURCES.json. The Temurin
runtime is a local build dependency and is not redistributed.

`src/case.inc` adapts the OpenJDK RuleBasedBreakIterator traversal and preserves
its Oracle/Taligent/IBM notices. That file and the generated tables retain GPLv2
with the Classpath exception; they are not relicensed under the root MIT license.
The corresponding C source, upstream Java references and original MIT-licensed
table generators are all included. See CASE-SUPPORT.md and REGEX-SUPPORT.md for
regeneration and scope. character.inc adds Unicode character classifications
and radix digits from the same pinned runtime; its native adapter is original
MIT-licensed project code.

Xinbot release audited (not redistributed in this package):
https://github.com/huangdihd/xinbot/releases/tag/2.4.3-RELEASE

The audit is a static class-pool inventory and a separate attempted host run.
Optional Java 22 classes in the release JAR do not, by themselves, establish
that Java 22 is required on every execution path.
