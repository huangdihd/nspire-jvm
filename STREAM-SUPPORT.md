# Stream and enum support

The supplemental runtime contains the original OpenJDK 8 stream implementation,
functional interfaces, optional values, summary statistics, collectors,
comparators and enum collections. Sources remain unchanged at the revision in
`runtime/openjdk8/SOURCES.json`; they run as Java bytecode on this interpreter.

The VM supplies enum constants by invoking the actual compiled `values()`
method after class initialization. Each class roots its shared enum universe
and name directory. Public `Class.getEnumConstants()` returns a defensive array
copy, and EnumMap/EnumSet use the internal shared universe. Enum.valueOf and
canonical/declaring-class names are supported for the tested top-level, member,
local, anonymous and array cases. General method/field reflection APIs remain
incomplete.

Additional native support required by these libraries includes Integer/Long
comparison, sum and bit counts; Long min/max; Double NaN/infinity checks;
StringBuilder CharSequence append, charAt and setLength; and Long/Double
boxing/unboxing, comparison and hashing. Long values in -128..127 are cached
and GC-rooted. Double comparison/hash/equality distinguish signed zero and
canonicalize NaNs. Constructor reflection accepts the supported wrappers with
Java widening conversions and rejects narrowing.

## Verified sequential paths

`tools/test-stream.py` compares with standard Java using a 256 KiB Java heap:

- Enum reflection, initialization, defensive copies, valueOf errors, EnumMap,
  ordinary EnumSet and the >64-constant JumboEnumSet representation.
- noneMatch/anyMatch/allMatch and empty-stream behavior; noneMatch stops at the
  first matching value. Streams defer source traversal and reject reuse.
- map/filter/distinct/sorted/skip/limit, flatMap, reduction, collection to lists
  and joined strings, typed arrays, findFirst and min.
- IntStream/LongStream ranges, primitive mapping/filtering, sums, averages and
  double summary statistics, including NaN/infinity sums and boxed distinct/sort.
- Normal close-handler composition, repeated close, flatMap inner-stream cleanup,
  callback exceptions, captured lambdas and allocation/GC during traversal.
- StringBuilder calls custom CharSequence length/charAt methods, propagates
  their exceptions and preserves UTF-16 units when truncating/extending.
- Long/Double cache/value/NaN/bounds behavior and reflective numeric conversion.

OpenJDK 8 and later JDKs may legitimately differ in implementation details such
as whether count can skip a map stage on a sized source. Tests require traversal
when checking callback effects; they do not require a version-specific optimization.

## Current limits

Parallel streams reach the original parallel implementation and fail because
CountedCompleter/ForkJoinPool support is not yet present. Serializable-lambda
bootstraps are still rejected; this also affects some Comparator factory methods.
This is an OpenJDK 8 library, so newer Stream APIs such as toList are not supplied.

Float/Byte/Short/Character wrapper APIs remain incomplete. Double object text
formatting is explicitly rejected pending a Java-compatible conversion routine;
the numeric object operations above do not imply formatting support. Numeric
parsing, broad reflection and serialization also remain incomplete. Basic
Throwable suppressed-exception lists now support the original FilterOutputStream
close path (OUTPUT-SUPPORT.md). The stream suite covers ordinary close composition
and callback exceptions, not multiple stream-close failures.

The existing class-count, metadata, stack and Java-heap budgets apply. Host
success does not verify the ARM context switch, device timing or calculator
memory use. Full Xinbot startup passes annotation phase selection and still
fails later at Class.getMethods after creating the application's console appender;
see XINBOT-RUN.txt.
