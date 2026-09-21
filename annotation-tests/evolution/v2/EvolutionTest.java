import java.lang.annotation.*;
public class EvolutionTest {
    static void check(boolean x){if(!x)throw new AssertionError();}
    public static void main(String[] args) {
        Evolved a=Stored.class.getAnnotation(Evolved.class);
        check(a!=null&&a.okay().equals("ok")&&Stored.class.getAnnotations().length==1);
        check(LostAnnotations.class.getAnnotations().length==0);
        System.gc();
        try {a.changed();throw new AssertionError();}catch(AnnotationTypeMismatchException e){check(e.element().getName().equals("changed")&&e.element().getDeclaringClass()==Evolved.class);}
        try {a.array();throw new AssertionError();}catch(AnnotationTypeMismatchException e){check(e.element().getName().equals("array"));}
        try {a.phase();throw new AssertionError();}catch(EnumConstantNotPresentException e){check(e.enumType()==VersionEnum.class&&e.constantName().equals("OLD"));}
        try {a.absent();throw new AssertionError();}catch(TypeNotPresentException e){check(e.typeName().equals("Removed"));}
        try {a.types();throw new AssertionError();}catch(TypeNotPresentException e){check(e.typeName().equals("Removed"));}
        try {a.added();throw new AssertionError();}catch(IncompleteAnnotationException e){check(e.annotationType()==Evolved.class&&e.elementName().equals("added"));}
        System.gc();check(a.okay().equals("ok"));
        System.out.println("evolved annotation types, deferred errors and missing annotations passed");
    }
}
