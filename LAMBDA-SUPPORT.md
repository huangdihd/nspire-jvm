# Lambda and method-reference support

The interpreter recognizes the compiled `LambdaMetafactory.metafactory`
bootstrap and the marker/bridge/serializable portions of `altMetafactory`. It generates a
class for each linked invokedynamic constant-pool entry. Captured values become
ordinary instance fields; each SAM/bridge method is executable JVM bytecode.
Calls therefore use the same GC roots, exception handling, instruction budget,
thread scheduling and synchronization as application methods.

Implemented paths:

- Captured references and primitive values, including long/double values.
  Repeated captures share the generated class but can allocate distinct objects;
  no promise is made about identity caching of stateless lambda objects.
- Static, bound/unbound virtual, interface and special method references, plus
  constructor references. A bound instance receiver must be non-null at capture.
  Actual target class initialization occurs when the generated invocation needs it.
- Reference casts, primitive widening, return-value discarding and calls to the
  VM's wrapper boxing/unboxing methods. All eight primitive wrappers now provide
  these basic operations; Float/Double object text formatting and other wrapper APIs
  remain incomplete and still fail explicitly when reached.
- `altMetafactory` marker interfaces and additional bridge descriptors.
- Serializable lambda classes implement Serializable and generate a private
  writeReplace method with actual capture-site/implementation metadata and
  boxed captured values. It constructs the preserved OpenJDK SerializedLambda.
  That class's original readResolve invokes javac's real $deserializeLambda$
  method, which validates and reconstructs the lambda. This protocol works
  independently of the still-missing generic object-stream implementation.
- The no-context PrivilegedExceptionAction overload executes the action and
  wraps checked exceptions in the original PrivilegedActionException. Runtime
  exceptions and Errors propagate unchanged. There is no SecurityManager or
  protection-domain enforcement.
- Interface default methods are selected by specificity. Interfaces declaring
  default methods are initialized when their implementing class is initialized.
  Newer javac's private-method references do not dispatch to unrelated subclass
  methods with the same name.

Current limits:

- No general MethodHandle/MethodType/CallSite APIs, application-defined bootstrap
  methods, dynamic constants or direct Java calls to LambdaMetafactory.
- Generated classes use internal VM class metadata; full hidden-class reflection,
  nestmate metadata and class unloading are not implemented. Generic stream
  serialization/deserialization is missing; no serialized byte-stream round
  trip or arbitrary object graph persistence is claimed.
- Generated classes count toward the existing 2,048-class and 16 MiB metadata
  limits. The current global-per-name class loader model still applies.
- Linkage checks and failure reporting are a subset of JVM bootstrap verification;
  malformed or unsupported bootstraps can produce a VM diagnostic instead of a
  fully specified BootstrapMethodError/LambdaConversionException chain.

`tools/test-lambda.py` compiles real source twice with `javac --release 8` and
`--release 17`, then compares execution with standard Java. It covers changing
captures, primitive/wide values, class reuse, 64 KiB GC pressure, constructor and
array references, synchronized targets, generic casts, boxing/unboxing, exception
propagation, null receivers, marker interfaces, bridges, interface initialization,
private/super references, default-method composition and threaded execution.

There are also standard-Java comparisons for writeReplace metadata, primitive
and reference captures, GC pressure, original readResolve, method/constructor
references and invalid metadata rejection. These run for Java 8 and Java 17
bytecode with a 64 KiB VM heap. Privileged actions have separate checked,
unchecked, Error, null-action and exception-cause checks.
Invalid metadata must reach javac's IllegalArgumentException. The check compares
that root cause: Java 17 adds an InvalidObjectException wrapper around readResolve
failures, while this runtime preserves the Java 8 implementation and its wrappers.

Unbound application native methods now throw catchable UnsatisfiedLinkError,
including through lambda bridges; see NATIVE-SUPPORT.md. Fatal cleanup tests
use unsupported numeric String.format on main/child stacks. Real Xinbot passes
its time formatter's internal serializable lambdas; the current whole-startup
failure is missing java.lang.Record after actual Jansi extraction,
delete-on-exit registration, native load-failure handling, parameter reflection
and package version lookup. Record ObjectMethods bootstrap is not implemented.
