# Package manifest metadata

Class.getPackage shares Package objects within the existing bootstrap/application
loader scopes. For JAR-loaded classes, six specification/implementation title,
version and vendor getters read the actual META-INF/MANIFEST.MF. Package sections
override main attributes; an empty value is preserved, while a missing value
falls back to the main attributes. Missing manifests and directory-loaded
classes yield null metadata. A directory's META-INF/MANIFEST.MF is not used.

The package origin is the earliest loaded class with that package and loader
scope, even if getPackage is first called on a class from a later JAR. Metadata
is parsed lazily and cached. This is not full eager classloader manifest
validation. Sealing, signing/JAR verification, package annotations, custom
loader namespaces and module package semantics remain unsupported.

Parsing executes unchanged OpenJDK 8 Manifest and Attributes bytecode through
the original MIT nspire.reflect.PackageInfo adapter. Valid mixed-case attribute
names and folded UTF-8 are supported. The duplicate-header warning path still
needs PlatformLogger; the complete java.util.jar API is not implemented.

The legacy String(byte[], highByte[, offset, count]) constructors used by the
manifest parser combine actual unsigned bytes with UTF-16 high bytes. Embedded
NUL, surrogate units, signed/overflowing high-byte arguments and bounds/null
exception precedence are compared with Java 8.

```sh
python3 tools/test-package-metadata.py --java /path/to/java8/bin/java
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-package-metadata.py --java /path/to/java8/bin/java --vm build/nspire-jvm-asan
```

PACKAGE-METADATA-RESULTS.txt records three host checks: two different class
definition orders across real fixture JARs, and high-byte string constructors.
They cover package identity, all six fields, package overrides, empty/missing
values, UTF-8 split across continuation lines, directory behavior, GC and absence
of unintended class initialization. All three pass on ordinary and ASan/UBSan
builds with leak detection. They do not establish calculator execution.

Unchanged Xinbot reads its JAR's Implementation-Version (2.4.3-RELEASE).
Record and integer parsing support now let Version.from complete, followed by
entry into Xinbot.main. LangManager initialization stops at Map.of. Complete
startup, networking and calculator execution remain unverified; see XINBOT-RUN.txt.
