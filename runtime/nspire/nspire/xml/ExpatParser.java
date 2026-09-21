/* Nspire JVM Expat adapter. MIT license; see the repository LICENSE. */
package nspire.xml;

import javax.xml.parsers.SAXParser;
import org.xml.sax.*;

final class ExpatParser extends SAXParser {
    private final ExpatReader reader;
    ExpatParser(boolean namespaces, boolean disallowDoctype) {
        reader = new ExpatReader(namespaces, disallowDoctype);
    }
    public XMLReader getXMLReader() { return reader; }
    public Parser getParser() throws SAXException { throw new SAXNotSupportedException("SAX1 Parser is not supported"); }
    public boolean isNamespaceAware() { return reader.namespaces; }
    public boolean isValidating() { return false; }
    public void setProperty(String name, Object value) throws SAXNotRecognizedException, SAXNotSupportedException { reader.setProperty(name, value); }
    public Object getProperty(String name) throws SAXNotRecognizedException, SAXNotSupportedException { return reader.getProperty(name); }
}
