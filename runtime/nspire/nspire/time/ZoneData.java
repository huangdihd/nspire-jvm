/* Packaged runtime data access. Original project code, MIT license. */
package nspire.time;
import java.io.InputStream;
import java.io.IOException;
public final class ZoneData {
    private ZoneData() {}
    public static InputStream open() throws IOException {
        InputStream input=ZoneData.class.getResourceAsStream("/nspire/time/tzdb.dat");
        if(input==null)throw new IOException("Missing packaged TZDB database");
        return input;
    }
}
