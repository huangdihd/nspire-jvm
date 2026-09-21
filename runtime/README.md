# Supplemental class library

The sources in `openjdk8/` are preserved upstream OpenJDK sources, not a complete
runtime. The build deliberately packages only classes compiled from these files.
Missing classes, unimplemented natives and invokedynamic still fail explicitly.

Build with a host JDK 17+ and a Java 8 JDK/JRE supplying compatible API signatures:

```sh
python3 tools/build-runtime.py --java8-home /path/to/java8
python3 tools/test-runtime.py --vm build/nspire-jvm
```

Neither the build-time rt.jar nor desktop HotSpot runs on the calculator.
The resulting `dist/runtime.jar.tns` is interpreted by the Ndless C interpreter.
See `openjdk8/SOURCES.json` for the exact revision, paths and source checksums.
Licensing is described in `openjdk8/LICENSE` and the repository's THIRD-PARTY.md.

The differential programs cover map resizing and tree bins, equal string keys,
concurrent insertion and count updates, 32/64-bit and reference atomics, blocking
queue operations, timed polling, reentrant locks, conditions and parking.
They do not establish Java SE compatibility, serialization support, or support
for all methods present in the JAR (including streams, lambdas and fork/join).
