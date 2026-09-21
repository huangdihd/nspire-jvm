import java.lang.reflect.*;
import access.Parent;
import foreign.Child;
public class MethodAccessTest {
    interface Action {void run()throws Exception;}
    static void check(Action a){try{a.run();System.out.println("ok");}catch(Exception e){System.out.println(e.getClass().getName());}}
    public static void main(String[] args)throws Exception {
        Parent parent=new Parent();Child child=new Child();
        System.out.println(Parent.class.getMethod("visible").invoke(parent));
        for(String name:new String[]{"protectedCall","packageCall","privateCall"}){
            Method first=Parent.class.getDeclaredMethod(name),second=Parent.class.getDeclaredMethod(name);
            check(()->first.invoke(parent));first.setAccessible(true);System.out.println(first.invoke(parent));
            check(()->second.invoke(parent));first.setAccessible(false);check(()->first.invoke(parent));
        }
        System.out.println(Child.protectedInvoke(child));check(()->Child.protectedInvoke(parent));
        System.out.println(Child.protectedStaticInvoke());
    }
}
