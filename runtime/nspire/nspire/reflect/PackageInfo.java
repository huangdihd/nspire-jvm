// JVM package metadata adapter. MIT; see repository LICENSE.
package nspire.reflect;
import java.io.InputStream;
import java.io.IOException;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
public final class PackageInfo {
    private PackageInfo() {}
    public static String[] read(InputStream input, String section) throws IOException {
        Manifest manifest = new Manifest(input);
        Attributes main = manifest.getMainAttributes();
        Attributes specific = manifest.getAttributes(section);
        String[] keys = {"Specification-Title", "Specification-Version", "Specification-Vendor",
            "Implementation-Title", "Implementation-Version", "Implementation-Vendor"};
        String[] values = new String[keys.length];
        for(int i=0;i<keys.length;i++) {
            String value = specific == null ? null : specific.getValue(keys[i]);
            values[i] = value == null ? main.getValue(keys[i]) : value;
        }
        return values;
    }
}
