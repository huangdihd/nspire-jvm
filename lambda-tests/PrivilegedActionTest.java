import java.security.*;
public class PrivilegedActionTest {
    static class Localized extends Exception {
        public String getLocalizedMessage(){System.gc();return "localized";}
    }
    public static void main(String[] args)throws Exception {
        System.out.println(AccessController.doPrivileged((PrivilegedExceptionAction<String>)()->"ran"));
        Exception checked=new Exception("checked");
        try{AccessController.doPrivileged((PrivilegedExceptionAction<Void>)()->{throw checked;});throw new AssertionError();}
        catch(PrivilegedActionException error){System.out.println(error.getException()==checked);System.out.println(error.getCause()==checked);}
        IllegalStateException unchecked=new IllegalStateException("runtime");
        try{AccessController.doPrivileged((PrivilegedExceptionAction<Void>)()->{throw unchecked;});throw new AssertionError();}
        catch(IllegalStateException error){System.out.println(error==unchecked);}
        Error fatal=new Error("error");
        try{AccessController.doPrivileged((PrivilegedExceptionAction<Void>)()->{throw fatal;});throw new AssertionError();}
        catch(Error error){System.out.println(error==fatal);}
        try{AccessController.doPrivileged((PrivilegedExceptionAction<?>)null);throw new AssertionError();}
        catch(NullPointerException expected){System.out.println("null action");}
        System.out.println(new Exception(checked).getMessage());
        System.out.println(new Exception(new Localized()).getMessage());
        Exception explicit=new Exception((Throwable)null);System.out.println(explicit.getMessage()==null);
        try{explicit.initCause(checked);throw new AssertionError();}catch(IllegalStateException expected){System.out.println("explicit null cause locked");}
        Exception later=new Exception();System.out.println(later.initCause(checked)==later);System.out.println(later.getCause()==checked);
        try{later.initCause(null);throw new AssertionError();}catch(IllegalStateException expected){System.out.println("cause assigned once");}
        Exception self=new Exception();try{self.initCause(self);throw new AssertionError();}catch(IllegalArgumentException expected){System.out.println("self cause rejected");}
        try{AccessController.doPrivileged((PrivilegedExceptionAction<Void>)()->{throw checked;});}
        catch(PrivilegedActionException error){try{error.initCause(null);throw new AssertionError();}catch(IllegalStateException expected){System.out.println("wrapper cause locked");}}
    }
}
