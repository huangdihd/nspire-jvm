# Lambda and method-reference support

The interpreter recognizes the compiled `LambdaMetafactory.metafactory`
bootstrap and the marker/bridge portions of `altMetafactory`. It generates a
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
  VM's wrapper boxing/unboxing methods. Integer/Boolean and Long/Double numeric
  wrappers are supported; Double object text formatting and other wrapper APIs
  remain incomplete and still fail explicitly when reached.
- `altMetafactory` marker interfaces and additional bridge descriptors.
  Serializable-lambda flags are rejected; no serialized representation is faked.
- Interface default methods are selected by specificity. Interfaces declaring
  default methods are initialized when their implementing class is initialized.
  Newer javac's private-method references do not dispatch to unrelated subclass
  methods with the same name.

Current limits:

- No general MethodHandle/MethodType/CallSite APIs, application-defined bootstrap
  methods, dynamic constants or direct Java calls to LambdaMetafactory.
- Generated classes use internal VM class metadata; full hidden-class reflection,
  nestmate metadata, class unloading and lambda serialization are not implemented.
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

The original unsupported-lambda tests now use a serializable lambda to preserve
their main-stack and child-stack fatal-error checks. Ordinary lambda execution
is covered by the new positive tests. Real Xinbot now passes its initial lambda
bootstrap, XML parsing, sequential stream matching and annotation-based phase
selection. It also compiles the real Duration regex and passes property
substitution and creates the application's JLineConsoleAppender. The next
failure is Class.getMethods in BeanDescriptionFactory during property setup.
