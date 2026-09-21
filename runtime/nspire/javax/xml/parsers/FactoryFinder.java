/* Nspire JVM provider selection. MIT license; see the repository LICENSE. */
package javax.xml.parsers;

import java.util.Iterator;
import java.util.ServiceLoader;

final class FactoryFinder {
    static <T> T find(Class<T> type, String fallback) {
        String name = System.getProperty(type.getName());
        if (name != null) return newInstance(type, name, null, true);
        Iterator<T> providers = ServiceLoader.load(type).iterator();
        if (providers.hasNext()) return providers.next();
        if (type == SAXParserFactory.class) return type.cast(new nspire.xml.ParserFactory());
        throw new FactoryConfigurationError("No provider for " + type.getName());
    }

    static <T> T newInstance(Class<T> type, String name, ClassLoader loader, boolean fallback) {
        if (name == null) throw new FactoryConfigurationError("Null provider name");
        if (loader == null) loader = Thread.currentThread().getContextClassLoader();
        try {
            return type.cast(Class.forName(name, true, loader).getConstructor().newInstance());
        } catch (Exception e) {
            throw new FactoryConfigurationError(e, "Cannot create " + name);
        }
    }
}
