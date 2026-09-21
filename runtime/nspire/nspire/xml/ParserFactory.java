/* Nspire JVM Expat adapter. MIT license; see the repository LICENSE. */
package nspire.xml;

import javax.xml.parsers.*;
import org.xml.sax.*;

public final class ParserFactory extends SAXParserFactory {
    private boolean disallowDoctype;

    public SAXParser newSAXParser() throws ParserConfigurationException, SAXException {
        if (isValidating()) throw new ParserConfigurationException("DTD validation is not supported");
        return new ExpatParser(isNamespaceAware(), disallowDoctype);
    }

    public void setFeature(String name, boolean value) throws ParserConfigurationException, SAXNotRecognizedException, SAXNotSupportedException {
        if (ExpatReader.NAMESPACES.equals(name)) { setNamespaceAware(value); return; }
        if (ExpatReader.VALIDATION.equals(name)) { setValidating(value); return; }
        if (ExpatReader.DISALLOW_DOCTYPE.equals(name)) { disallowDoctype = value; return; }
        ExpatReader.checkFixedFeature(name, value);
    }

    public boolean getFeature(String name) throws ParserConfigurationException, SAXNotRecognizedException, SAXNotSupportedException {
        if (ExpatReader.NAMESPACES.equals(name)) return isNamespaceAware();
        if (ExpatReader.VALIDATION.equals(name)) return isValidating();
        if (ExpatReader.DISALLOW_DOCTYPE.equals(name)) return disallowDoctype;
        return ExpatReader.fixedFeature(name);
    }
}
