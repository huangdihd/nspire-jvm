# SAX XML support

The supplemental runtime contains unchanged OpenJDK 8 SAX/JAXP API sources.
The MIT-licensed Java adapter in `runtime/nspire/` drives the vendored Expat
2.8.4 C parser through `src/xml.inc`. It does not depend on a desktop JVM or
an external XML service. Expat is compiled for both the host and Ndless ARM.

Implemented paths:

- `SAXParserFactory.newInstance()`, system-property and ServiceLoader provider
  selection, explicit provider construction and the default Nspire provider.
- InputSource byte streams and character streams, with character streams taking
  precedence. Byte streams use Expat's supported encoding detection; Reader
  input is delivered as UTF-16, independently of the InputSource encoding label.
- Document, element, attribute, text, namespace, processing-instruction and
  skipped-entity callbacks. Handler references remain Java objects, and callbacks
  can allocate, trigger GC, throw Java exceptions or parse on another reader.
- Internal entities and default/normalized DTD attributes. External general and
  parameter entities remain disabled. Enabling them throws SAXNotSupportedException.
- SAXParseException with location and InputSource identifiers; ErrorHandler
  fatalError callbacks. Java callback and I/O exceptions propagate to the caller.
- Stream closing after ordinary parse success or failure. Native parser state is
  also freed on a fatal VM abort; such an abort does not run Java finally blocks.
- Nested readers, rejection of recursive use of the same reader, and parser
  reuse after errors. A SAXParser instance is not intended for concurrent use.

Known limits:

- No DTD/XSD validation, DOM, XSLT, XInclude, SAX1 Parser or lexical/declaration
  extension properties. The namespace-prefixes setting follows the namespace
  mode; reporting xmlns attributes together with expanded names is unsupported.
- URI-only inputs cannot yet be opened. Supply an InputStream or Reader.
  jaxp.properties provider configuration and complete JAXP provider error
  semantics are not implemented. The default provider is `nspire.xml.ParserFactory`.
- DTD notation/entity declaration system identifiers are currently reported
  as declared; relative identifiers are not resolved against the document URI.
- Expat and Xerces may split character callbacks differently. Prefix mapping
  end-order is not specified by SAX. Locator positions follow Expat and do not
  promise the same columns as Xerces for every encoding or markup layout.
- Each parse accepts at most 8 MiB of input. All active Expat parsers in one VM
  share an additional 8 MiB native-allocation payload budget (tracking headers
  are extra). Java heap and VM metadata
  retain their separate limits. Parsing-limit failures are reported, not ignored.
- Ndless currently uses Expat's low-entropy hash-salt fallback because a verified
  entropy source is unavailable. Host builds use /dev/urandom. This does not
  provide a security sandbox for untrusted programs or XML on the calculator.

Verification:

`tools/test-xml.py` compares events and failures with standard Java, including
Unicode surrogate pairs, UTF-16 input, namespace attributes, malformed input,
internal/external entities and stream lifetime. It also tests nested/threaded
callbacks with a 64 KiB Java heap and a fatal VM abort during a callback.

`tools/test-logback-xml.py --xinbot /path/to/xinbot.jar` directly runs the actual
Logback SaxEventRecorder bundled inside that JAR against its original logback.xml.
The tested Xinbot 2.4.3 release produces the same 27 events on Java and this VM.
This is a component test: actual Xinbot startup still encounters a missing lambda
bootstrap before it reaches the parser, and calculator execution is not verified.
