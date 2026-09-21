import java.lang.reflect.*;
import reflect.Fixture;
public class ReflectionTest {
    public static void main(String[] args)throws Exception {
        Constructor<Fixture.Target> ctor=Fixture.Target.class.getConstructor(Object.class,long.class,boolean.class,double.class);
        System.out.println(Fixture.initializations);
        System.out.println(ctor.getDeclaringClass()==Fixture.Target.class);System.out.println(ctor.getName());
        System.out.println(ctor.getParameterCount());System.out.println(ctor.getModifiers());
        Class<?>[] types=ctor.getParameterTypes();System.out.println(types[1]==long.class);types[1]=int.class;
        System.out.println(ctor.getParameterTypes()[1]==long.class);
        System.out.println(ctor.equals(Fixture.Target.class.getConstructor(Object.class,long.class,boolean.class,double.class)));
        System.out.println(ctor.hashCode()==Fixture.Target.class.getName().hashCode());
        Object[] values={new StringBuilder().append("kept").toString(),Integer.valueOf(42),Boolean.TRUE,Integer.valueOf(-7)};
        Fixture.callerArguments=values;Fixture.Target made=ctor.newInstance(values);
        System.out.println(Fixture.initializations);System.out.println(values[0]==null);
        System.out.println(made.payload);System.out.println(made.number);System.out.println(made.flag);System.out.println((int)made.real);
        try{ctor.newInstance();}catch(IllegalArgumentException e){System.out.println("arity rejected");}
        try{ctor.newInstance("x",null,true,3);}catch(IllegalArgumentException e){System.out.println("null primitive rejected");}
        try{ctor.newInstance("x",true,true,3);}catch(IllegalArgumentException e){System.out.println("wrong primitive rejected");}
        try{Fixture.Small.class.getConstructor(byte.class).newInstance(Integer.valueOf(12));}catch(IllegalArgumentException e){System.out.println("narrowing rejected");}
        try{Fixture.Target.class.getConstructor();}catch(NoSuchMethodException e){System.out.println("missing constructor");}
        Constructor<Fixture.Hidden> hidden=Fixture.Hidden.class.getDeclaredConstructor();
        try{hidden.newInstance();}catch(IllegalAccessException e){System.out.println("private denied");}
        System.out.println(hidden.isAccessible());hidden.setAccessible(true);System.out.println(hidden.newInstance().getClass()==Fixture.Hidden.class);
        System.out.println(Fixture.Hidden.class.getDeclaredConstructor().isAccessible());hidden.setAccessible(false);
        try{hidden.newInstance();}catch(IllegalAccessException e){System.out.println("private denied again");}
        try{Fixture.Broken.class.getConstructor().newInstance();}catch(InvocationTargetException e){System.out.println(e.getCause()==e.getTargetException());System.out.println(e.getCause().getMessage());
            try{e.initCause(null);throw new AssertionError();}catch(IllegalStateException expected){System.out.println("wrapper cause locked");}}
        try{Fixture.Abstract.class.getConstructor().newInstance();}catch(InstantiationException e){System.out.println("abstract rejected");}
        Constructor<Fixture.Varargs> varargs=Fixture.Varargs.class.getConstructor(String[].class);
        System.out.println(varargs.isVarArgs());System.out.println(varargs.newInstance((Object)new String[]{"a","b"}).size);
        System.out.println(Object.class.getConstructor((Class<?>[])null).newInstance((Object[])null).getClass()==Object.class);
        try{Fixture.BadArity.class.getConstructor(String.class).newInstance();}catch(IllegalArgumentException e){System.out.println("bad arity initialization="+Fixture.initializations);}
        Object[] snapshot={new StringBuilder().append("retained").toString()};Fixture.callerArguments=snapshot;
        System.out.println(Fixture.Snapshot.class.getConstructor(Object.class).newInstance(snapshot).payload);
    }
}
