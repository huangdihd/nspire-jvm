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

`runtime/openjdk8/` contains 102 unmodified Java source files, their per-file
copyright notices, LICENSE (GPLv2 with the Classpath exception for these files),
ASSEMBLY_EXCEPTION and THIRD_PARTY_README. SOURCES.json records every upstream
path and SHA-256. These files are not relicensed under this project's MIT license.
`dist/runtime.jar.tns` is compiled from those sources using tools/build-runtime.py;
it embeds the notices and source manifest. The corresponding sources are in
this repository and the source ZIP. A separate Java 8 rt.jar is used only for
compile-time API signatures and is not redistributed or copied into the JAR.

The SAX sources also preserve their historical SAX notices. The OpenJDK source
headers and root license files remain intact. The additional Java adapter and
provider selector in `runtime/nspire/` are original MIT-licensed project code,
compiled into the same supplemental JAR with their own license and source hashes.

Expat XML parser:
https://github.com/libexpat/libexpat/releases/tag/R_2_8_4

`vendor/expat/` contains unchanged library source files and upstream COPYING,
README.md and Changes from the official 2.8.4 release archive. Its SOURCES.json
records the archive URL, SHA-256 and per-file hashes. The original MIT notices
are preserved. Project-specific configuration is in `src/expat_config.h`;
the upstream source is not modified. Expat is statically linked into the host
and Ndless VM builds. The Java bridge is original code, not Xerces.

Xinbot release audited (not redistributed in this package):
https://github.com/huangdihd/xinbot/releases/tag/2.4.3-RELEASE

The audit is a static class-pool inventory and a separate attempted host run.
Optional Java 22 classes in the release JAR do not, by themselves, establish
that Java 22 is required on every execution path.
