# Runtime annotation support

`src/annotations.inc` reads actual `RuntimeVisibleAnnotations` and
`AnnotationDefault` attributes. Annotation interfaces and their members come
from the application or preserved OpenJDK sources. Generated concrete classes
implement the annotation interface; their native accessors read GC-traced
member values and return defensive copies of arrays.

Implemented paths:

- `Class.isAnnotation`, `getAnnotation`, `getDeclaredAnnotation`,
  `isAnnotationPresent`, `getAnnotations`, `getDeclaredAnnotations`,
  `getAnnotationsByType` and `getDeclaredAnnotationsByType`.
- The same annotation queries on existing Field and Constructor mirrors.
  Class implements the AnnotatedElement/GenericDeclaration interfaces, so
  calls through those interfaces reach the same metadata.
- Runtime retention, superclass-only `@Inherited` lookup, declared overrides,
  repeated-annotation container expansion and classfile ordering. An empty
  repeatable container permits inherited annotations to be found.
- All primitive member types, strings, class literals (including primitives
  and arrays), actual enum constants, nested annotations and member arrays.
  Enum constants initialize their actual declaring enum; inspecting annotations
  does not initialize the annotated application class.
- Compiled annotation defaults, independent array results, `annotationType`,
  `equals` and the annotation hash algorithm, including canonical NaNs and
  distinct positive/negative floating-point zero values. Equality calls actual
  member implementations on handwritten annotation objects.
- Deferred member-access errors for missing class literals, removed enum
  constants, mismatched member types and newly added members without defaults.
  The exceptions use unchanged OpenJDK implementations. Missing top-level
  annotation types and annotations whose current retention is not RUNTIME are
  omitted, as in the standard Java comparison.

Four differential tests exercise these paths with 128/256 KiB Java heaps,
explicit GC, replaced class definitions and intentionally missing classfiles.
A fifth check compares the real PhaseIndicator values on nine classes from the
original Xinbot release with standard Java. It observes SECOND and
DEPENDENCY_ANALYSIS where declared; it does not substitute FIRST everywhere.
A sixth check verifies explicit failure for unsupported floating-point text.
All six checks pass on ordinary and ASan/UBSan builds with leak detection.
The full existing host regression suite also passes on both builds.

```sh
python3 tools/test-annotations.py --vm build/nspire-jvm
# Optional original application component check:
python3 tools/test-annotations.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
```

Limits:

- Method lookup/invocation reflection, `Method.getDefaultValue`, parameter
  annotations and type-use annotations are not implemented by this change.
  Method mirrors currently expose names and declaring classes when returned
  by AnnotationTypeMismatchException; this is not general Method reflection.
- Annotation serialization, java.lang.reflect.Proxy APIs and custom class
  loader namespaces are unavailable. Serializable marking does not provide an
  object serialization implementation.
- Text output is an implementation-specific annotation description; member
  values containing float/double deliberately fail until Java-compatible
  number formatting is provided. Arrays and nested annotations retain this
  restriction. Annotation text containing malformed members is not a complete
  implementation of OpenJDK's diagnostic formatting.
- This is not a classfile verifier. Some malformed/unsupported metadata reports
  a VM diagnostic instead of the complete Java-specified linkage/error chain.
  Annotation nesting is limited to 64. Generated implementation classes count
  toward the 2,048-class limit and the unchanged 16 MiB metadata budget.

Actual Xinbot now passes annotation-based configuration phase selection and
starts creating configuration handlers. Regex compilation and property
substitution also pass. It next fails loading `java.io.OutputStream` while
constructing Xinbot's JLineConsoleAppender. This remains
before Xinbot.main, without networking or calculator runtime verification.
