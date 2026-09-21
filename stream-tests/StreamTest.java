import java.util.*;
import java.util.function.*;
import java.util.stream.*;

public class StreamTest {
    static void check(boolean condition) { if(!condition)throw new AssertionError(); }
    public static void main(String[] args) {
        int[] calls={0};
        check(!Arrays.asList("before","target","after").stream().noneMatch(s->{calls[0]++;return s.equals("target");}));
        System.out.println("noneMatch calls="+calls[0]);
        check(Stream.empty().noneMatch(s->{throw new AssertionError();}));
        check(Stream.empty().allMatch(s->{throw new AssertionError();}));
        check(!Stream.empty().anyMatch(s->{throw new AssertionError();}));
        List<String> source=new ArrayList<>(Arrays.asList(" a ","bbb"," cc ","a","bbb"));
        Stream<String> lazy=source.stream().map(s->{calls[0]++;return s.trim();});
        source.add(" d ");check(calls[0]==2);
        List<String> result=lazy.filter(s->s.length()<3).distinct().sorted().skip(1).limit(2).collect(Collectors.toList());
        System.out.println(result.toString());System.out.println("lazy calls="+calls[0]);
        try { lazy.count(); } catch(IllegalStateException expected){System.out.println("single use");}
        String joined=Stream.of("one","two","three").collect(Collectors.joining(",","[","]"));
        System.out.println(joined);
        System.out.println(Stream.of("a","b").flatMap(s->Stream.of(s,s)).reduce("",(a,b)->a+b));
        String[] array=Stream.of("abc","d").map(String::toUpperCase).toArray(String[]::new);
        System.out.println(array.length);System.out.println(array[0]);
        System.out.println(Stream.of("first","second").findFirst().get());
        System.out.println(Stream.<String>empty().findFirst().orElse("fallback"));
        System.out.println(Stream.of(3,1,2).min(Integer::compare).get());
        check(IntStream.range(1,101).sum()==5050);
        check(LongStream.rangeClosed(1,100).sum()==5050L);
        check(DoubleStream.of(1.5,2.25,-0.75).sum()==3.0);
        check(IntStream.of(2,3,4).map(x->x*x).filter(x->x>4).sum()==25);
        check(LongStream.of(2,3,4).map(x->x*x).filter(x->x>4).sum()==25L);
        check(DoubleStream.of(2,3,4).map(x->x*x).filter(x->x>4).sum()==25.0);
        check(IntStream.range(0,30).skip(3).limit(4).average().getAsDouble()==4.5);
        check(LongStream.empty().average().isPresent()==false);
        check(DoubleStream.of(1,2,3).summaryStatistics().getCount()==3);
        check(Double.isNaN(DoubleStream.of(Double.NaN,1).sum()));
        check(DoubleStream.of(Double.POSITIVE_INFINITY,1).sum()==Double.POSITIVE_INFINITY);
        check(LongStream.of(1,1,2,Long.MAX_VALUE).boxed().distinct().count()==3);
        Double[] order=DoubleStream.of(Double.NaN,-0.0,0.0,Double.NEGATIVE_INFINITY,Double.NaN,1.5)
            .boxed().distinct().sorted().toArray(Double[]::new);
        for(Double d:order)System.out.println(Double.doubleToLongBits(d.doubleValue()));
        System.out.println("primitive streams");
        int[] close={0};
        Stream<String> closed=Stream.of("x").onClose(()->close[0]++).onClose(()->close[0]+=2);
        closed.close();closed.close();check(close[0]==3);
        try { closed.count(); } catch(IllegalStateException expected){System.out.println("closed stream");}
        try { Stream.of("x").map(s->{throw new IllegalArgumentException("callback");}).forEach(x->{}); }
        catch(IllegalArgumentException expected){System.out.println(expected.getMessage());}
        int[] flatClosed={0};
        check(Stream.of("a","b").flatMap(s->Stream.of(s).onClose(()->flatClosed[0]++)).count()==2);
        check(flatClosed[0]==2);
        Supplier<String> supplier=()->{System.gc();return "kept";};
        check(Stream.generate(supplier).limit(200).allMatch(s->s.equals("kept")));
        System.out.println("closure, exceptions and GC");
    }
}
