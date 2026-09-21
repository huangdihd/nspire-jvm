// Original platform adapter. MIT license; see the repository LICENSE.
package nspire.security;
import java.security.PrivilegedExceptionAction;
import java.security.PrivilegedActionException;
public final class Actions {
    private Actions() {}
    // This VM has no protection-domain policy; preserve actual execution and exceptions.
    public static Object run(PrivilegedExceptionAction<?> action)throws PrivilegedActionException {
        try { return action.run(); }
        catch(RuntimeException error) { throw error; }
        catch(Exception error) { throw new PrivilegedActionException(error); }
    }
}
