# Reflective parameters

Method and Constructor getParameters create preserved OpenJDK 8 Parameter
objects from real descriptors and MethodParameters attributes. Names, final,
synthetic and mandated flags come from the classfile; absent names use argN.
The declaring executable, parameter index, raw types and varargs state are
retained. Each executable caches its Parameter objects through GC and returns
a fresh array. Invalid name indices, names, flags and parameter counts throw
the original MalformedParametersException.

RuntimeVisibleParameterAnnotations is parsed through the existing annotation
implementation. Direct Parameter annotation lookup and executable annotation
arrays work, including the Java 8 handling of shortened constructor annotation
tables for synthetic arguments. Modifier is an original MIT adapter providing
the Java 8 masks and flag operations; Class.getTypeName handles arrays and
primitive types as well as reference classes.

Method Signature attributes are preserved. Generic parameter queries only
return raw Class values when no generic signature is present. Generic signatures
and unimported intrinsic generic metadata fail explicitly. Repeatable
Parameter.getAnnotationsByType, type-use annotations, getAnnotatedType and
complete generic reflection remain unsupported. Intrinsic parameter annotations
are not imported. The API-table generator checks that its pinned intrinsic
non-constructor declarations have no MethodParameters attributes, so synthesized
argN names do not discard available metadata.

Malformed structural attribute lengths still use the VM's fatal class-parser
diagnostic rather than a complete catchable ClassFormatError implementation.
The VM is not a bytecode verifier or a security boundary for untrusted classes.

```sh
python3 tools/test-parameters.py --java /path/to/java8/bin/java
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
  python3 tools/test-parameters.py --java /path/to/java8/bin/java --vm build/nspire-jvm-asan
```

PARAMETER-RESULTS.txt records comparisons with Java 8, with and without
javac -parameters: names and flags, inner/enum constructors, varargs, annotations,
independent arrays, executable identity, equality/hashCode, Modifier and GC.
Mutated classfiles check semantic metadata errors; a separate case verifies the
explicit generic-signature limitation. All 12 checks pass on ordinary and
ASan/UBSan builds with leak detection. These are host tests, not device tests.

Unmodified Parameter and MalformedParametersException sources and hashes are in
runtime/openjdk8. The C adapter is src/parameters.inc. See
[JVMS MethodParameters](https://docs.oracle.com/javase/specs/jvms/se8/html/jvms-4.html#jvms-4.7.24).
