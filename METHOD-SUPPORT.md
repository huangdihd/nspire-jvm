# Method reflection and package lookup

Class.getMethods/getDeclaredMethods and getMethod/getDeclaredMethod now use
real classfile declarations. Enumeration accounts for public inheritance,
interface defaults, static interface methods, unrelated interfaces, covariant
returns, bridges, arrays, primitives and void. Results have no promised order;
tests sort their signatures before comparing with Java. Selection follows the
[Class API rules](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/lang/Class.html#getMethods()).

Each returned Method mirror has separate accessible state. Method APIs include
name, declaring class, modifiers, parameter/return/checked-exception types,
parameter count, bridge/synthetic/varargs/default flags, equality/hashCode,
setAccessible/isAccessible and the existing runtime annotation operations for
bytecode-defined methods. Type arrays are defensive copies. The parser preserves
the classfile Exceptions attribute for checked-exception metadata.

Method.invoke performs access and receiver checks, argument count/type checks,
primitive unboxing and permitted widening. It invokes the actual target,
including virtual overrides, interface defaults, private methods, synchronized
bytecode and static methods. Static initialization precedes argument conversion,
so an initializer can change the caller's argument array as on the reference VM.
Target Java exceptions are wrapped in InvocationTargetException. Void returns
null; primitive results use the corresponding wrappers. Converted arguments and
receiver survive GC and native calls obey the VM's depth/root limits.

Byte, Short, Character and Float now provide primitive constructors/valueOf,
the needed value accessors, equality/hash and required small-value caches.
This completes the primitive wrappers used by reflective invocation; parsing,
comparison and formatting coverage of these classes remains partial. Float
object text formatting fails explicitly, as Double formatting already does.

Intrinsic classes get reflection declarations from
`vendor/openjdk8-api/methods.inc`: 1,604 method records for 92 names referenced by
the intrinsic registry and its parent mappings. Names, descriptors, modifiers
and checked exceptions come from the pinned Temurin 8u504-b01 rt.jar. The table
does not copy implementation bytecode or add implementations for those APIs.
Reflectively invoking an unavailable intrinsic still fails explicitly. This
metadata remains separate from bytecode dispatch so it cannot accidentally
replace native String, boxed-value or Class behavior with Object's methods.

Reproduce the table with the pinned Java 8 input:

```sh
python3 tools/generate-builtin-methods.py --rt-jar /path/to/java8/lib/rt.jar
```

The generator verifies the input and output SHA-256. `--update` explicitly
refreshes the output record after a reviewed extractor/registry change. Original
upstream notices and GPLv2 + Classpath-exception terms accompany the table; the
adapter and extraction script are original MIT project code.

Class.getPackage and Package.getName derive the package from the loaded class
name. Package objects are cached and shared within the VM's bootstrap/application
loader scopes and remain GC roots. Primitive and array classes return null;
ordinary classes in the unnamed package get an empty-name package, matching the
Java 17 reference. Manifest/version/sealing metadata and package annotations are
not provided; their APIs do not return invented values.

```sh
python3 tools/test-methods.py --vm build/nspire-jvm
python3 tools/test-methods.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
```

With the original Xinbot JAR, nine checks cover seven Java comparisons, the
unchanged Logback bean-discovery component and an explicit unsupported-intrinsic
failure. The conversion matrix exercises all eight primitive source and target
types, rejecting narrowing and boolean/numeric mixing; float checks include
NaN payloads, signed zero, infinities and integer conversion boundaries. Other
tests cover public/declared discovery, unrelated and overriding interfaces,
bridges, varargs, private/protected/package access, independent mirrors, actual
virtual/private/static calls, synchronization, initialization ordering, exception
wrapping, method annotations and repeated GC on a 256 KiB Java heap.
All nine checks and the existing full host regression suite pass on ordinary
and ASan/UBSan builds with leak detection. Direct original-Xinbot runs on both
builds reach the same missing Charset class without sanitizer errors.

The Logback check runs its original BeanDescriptionCache/Factory against the
actual JLineConsoleAppender, compares all getter/setter/adder mappings, then
invokes real setters for withJansi and name and reads the changed values through
their getters. This is a component test, not a replacement application entry.

Remaining gaps include generic signatures, Parameter/type-use metadata,
Method.toString/toGenericString/getDefaultValue, full nestmate/module access
rules, caller-sensitive intrinsic protocols and custom loader namespaces.
Intrinsic-method annotations are not imported and fail explicitly. Signature
resolution for missing classes reports the existing VM diagnostic rather than
the complete Java linkage-error chain. Some native methods have declarations
but still lack implementations. Reflection observes the VM's existing class,
metadata, depth, instruction and heap limits; it does not make this a verifier
or an untrusted-code security boundary.

Actual Xinbot now completes appender bean discovery and package lookup. Its
property converter next needs java.nio.charset.Charset, before Xinbot.main.
No calculator or firmware-emulator execution is established by these host tests.
