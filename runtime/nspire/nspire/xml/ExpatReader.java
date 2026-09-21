/* Nspire JVM Expat adapter. MIT license; see the repository LICENSE. */
package nspire.xml;

import java.io.IOException;
import org.xml.sax.*;
import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.helpers.LocatorImpl;

public final class ExpatReader implements XMLReader {
    static final String NAMESPACES = "http://xml.org/sax/features/namespaces";
    static final String VALIDATION = "http://xml.org/sax/features/validation";
    static final String PREFIXES = "http://xml.org/sax/features/namespace-prefixes";
    static final String DISALLOW_DOCTYPE = "http://apache.org/xml/features/disallow-doctype-decl";
    private static final String SECURE = "http://javax.xml.XMLConstants/feature/secure-processing";
    private static final String GENERAL = "http://xml.org/sax/features/external-general-entities";
    private static final String PARAMETER = "http://xml.org/sax/features/external-parameter-entities";
    boolean namespaces;
    private boolean disallowDoctype, parsing;
    private ContentHandler content;
    private DTDHandler dtd;
    private EntityResolver resolver;
    private ErrorHandler errors;
    private LocatorImpl locator;

    public ExpatReader() { this(true, false); }
    ExpatReader(boolean namespaces, boolean disallowDoctype) {
        this.namespaces = namespaces; this.disallowDoctype = disallowDoctype;
    }
    static boolean fixedFeature(String name) throws SAXNotRecognizedException {
        if (name == null) throw new NullPointerException();
        if (SECURE.equals(name)) return true;
        if (GENERAL.equals(name) || PARAMETER.equals(name)) return false;
        throw new SAXNotRecognizedException(name);
    }
    static void checkFixedFeature(String name, boolean value) throws SAXNotRecognizedException, SAXNotSupportedException {
        if (fixedFeature(name) != value) throw new SAXNotSupportedException(name);
    }
    public boolean getFeature(String name) throws SAXNotRecognizedException, SAXNotSupportedException {
        if (NAMESPACES.equals(name)) return namespaces;
        if (PREFIXES.equals(name)) return !namespaces;
        if (VALIDATION.equals(name)) return false;
        if (DISALLOW_DOCTYPE.equals(name)) return disallowDoctype;
        return fixedFeature(name);
    }
    public void setFeature(String name, boolean value) throws SAXNotRecognizedException, SAXNotSupportedException {
        if (parsing) throw new SAXNotSupportedException("Cannot change features during parsing");
        if (NAMESPACES.equals(name)) { namespaces = value; return; }
        if (PREFIXES.equals(name)) {
            if (value != !namespaces) throw new SAXNotSupportedException("Namespace declaration attributes are not supported with namespaces enabled");
            return;
        }
        if (VALIDATION.equals(name)) {
            if (value) throw new SAXNotSupportedException("DTD validation is not supported");
            return;
        }
        if (DISALLOW_DOCTYPE.equals(name)) { disallowDoctype = value; return; }
        checkFixedFeature(name, value);
    }
    public Object getProperty(String name) throws SAXNotRecognizedException, SAXNotSupportedException {
        if (name == null) throw new NullPointerException();
        throw new SAXNotRecognizedException(name);
    }
    public void setProperty(String name, Object value) throws SAXNotRecognizedException, SAXNotSupportedException { getProperty(name); }
    public void setContentHandler(ContentHandler handler) { content = handler; }
    public ContentHandler getContentHandler() { return content; }
    public void setDTDHandler(DTDHandler handler) { dtd = handler; }
    public DTDHandler getDTDHandler() { return dtd; }
    public void setEntityResolver(EntityResolver handler) { resolver = handler; }
    public EntityResolver getEntityResolver() { return resolver; }
    public void setErrorHandler(ErrorHandler handler) { errors = handler; }
    public ErrorHandler getErrorHandler() { return errors; }
    public void parse(String systemId) throws IOException, SAXException { parse(new InputSource(systemId)); }
    public void parse(InputSource source) throws IOException, SAXException {
        if (source == null) throw new IllegalArgumentException("Null InputSource");
        if (parsing) throw new SAXException("Recursive parse on the same reader");
        parsing = true;
        locator = new LocatorImpl();
        locator.setPublicId(source.getPublicId()); locator.setSystemId(source.getSystemId());
        try { parse0(source, namespaces, disallowDoctype); }
        finally { parsing = false; }
    }
    private native void parse0(InputSource source, boolean namespaces, boolean disallowDoctype) throws IOException, SAXException;

    // Called by the native parser. Java handler references remain ordinary GC roots.
    private void position(int line, int column) { locator.setLineNumber(line); locator.setColumnNumber(column); }
    private void documentStart() throws SAXException {
        if (content != null) { content.setDocumentLocator(locator); content.startDocument(); }
    }
    private void documentEnd() throws SAXException { if (content != null) content.endDocument(); }
    private void elementStart(String[] name, String[] values) throws SAXException {
        if (content == null) return;
        AttributesImpl attrs = new AttributesImpl();
        for (int i = 0; i < values.length; i += 5) attrs.addAttribute(values[i], values[i+1], values[i+2], values[i+3], values[i+4]);
        content.startElement(name[0], name[1], name[2], attrs);
    }
    private void elementEnd(String[] name) throws SAXException { if (content != null) content.endElement(name[0], name[1], name[2]); }
    private void text(char[] value) throws SAXException { if (content != null) content.characters(value, 0, value.length); }
    private void prefixStart(String prefix, String uri) throws SAXException { if (content != null) content.startPrefixMapping(prefix, uri); }
    private void prefixEnd(String prefix) throws SAXException { if (content != null) content.endPrefixMapping(prefix); }
    private void instruction(String target, String data) throws SAXException { if (content != null) content.processingInstruction(target, data); }
    private void skipped(String name) throws SAXException { if (content != null) content.skippedEntity(name); }
    private void notation(String name, String publicId, String systemId) throws SAXException { if (dtd != null) dtd.notationDecl(name, publicId, systemId); }
    private void unparsed(String name, String publicId, String systemId, String notation) throws SAXException { if (dtd != null) dtd.unparsedEntityDecl(name, publicId, systemId, notation); }
    private void fatal(String message) throws SAXException {
        SAXParseException exception = new SAXParseException(message, locator);
        if (errors != null) errors.fatalError(exception);
        throw exception;
    }
}
