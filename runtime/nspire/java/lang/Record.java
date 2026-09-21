// Record base API for the interpreter. MIT; see repository LICENSE.
package java.lang;

public abstract class Record {
    protected Record() {}
    public abstract boolean equals(Object other);
    public abstract int hashCode();
    public abstract String toString();
}
