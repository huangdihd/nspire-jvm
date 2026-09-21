import java.lang.reflect.*;
import java.util.*;
import ch.qos.logback.core.joran.util.beans.*;
import xin.bbtt.mcbot.jLine.JLineConsoleAppender;
public class LogbackBeanTest {
    static void print(String prefix,Map<String,Method> methods){
        String[] names=methods.keySet().toArray(new String[0]);Arrays.sort(names);
        for(String name:names){Method m=methods.get(name);System.out.println(prefix+name+"="+m.getDeclaringClass().getName()+"."+m.getName()+"/"+m.getParameterCount()+"/"+m.getReturnType().getName());}
    }
    public static void main(String[] args)throws Exception {
        BeanDescriptionCache cache=new BeanDescriptionCache(null);
        BeanDescription bean=cache.getBeanDescription(JLineConsoleAppender.class);
        print("getter:",bean.getPropertyNameToGetter());print("setter:",bean.getPropertyNameToSetter());print("adder:",bean.getPropertyNameToAdder());
        JLineConsoleAppender appender=new JLineConsoleAppender();
        bean.getSetter("withJansi").invoke(appender,true);System.out.println(bean.getGetter("withJansi").invoke(appender));
        bean.getSetter("name").invoke(appender,"reflection-console");System.out.println(bean.getGetter("name").invoke(appender));
        System.out.println(cache.getBeanDescription(JLineConsoleAppender.class)==bean);
    }
}
