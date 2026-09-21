import java.util.function.*;

public class LambdaTest {
    interface IntFn { int apply(int x); default int twice(int x){return apply(apply(x));} }
    interface Wide { double call(long a, float b, double c); }
    interface Make { Box make(int x); }
    interface Marker {}
    interface Parent<T> { T get(); }
    interface Child extends Parent<String> { String get(); }
    static class Box {
        int n;Box(int n){this.n=n;}
        int add(int x){return n+x;}
        synchronized int bump(int x){return n+=x;}
        String label(){return "box"+n;}
    }
    static class Sub extends Box {
        Sub(){super(30);}
        String label(){return "sub";}
        Supplier<String> parent(){return super::label;}
        Supplier<String> privateRef(){return this::privateLabel;}
        private String privateLabel(){return "private";}
    }
    static class PrivateBase {
        private String secret(){return "base private";}
        Supplier<String> ref(){return this::secret;}
    }
    static class PrivateSub extends PrivateBase { public String secret(){return "sub unrelated";} }
    static int calls;
    static int add(int a,int b){calls++;return a+b;}
    static int length(String s){return s.length();}
    static int failed(int x){throw new IllegalArgumentException("target");}
    static <T> T identity(T x){return x;}
    static IntFn capture(final int[] data,final int offset){return x->data[0]+offset+x;}
    static class Late {
        static {System.out.println("target initialized");}
        static int value(){return 42;}
    }
    interface GetInt {int get();}
    static int seed(String name){System.out.println("init "+name);return 2;}
    interface Plain { int SEED=seed("plain");int get(); }
    interface Stateful extends Plain { int SEED=seed("default");default int twice(){return get()*2;} }
    public static void main(String[] args)throws Exception{
        final int[] state={1};Runnable r=()->{state[0]+=2;System.gc();};r.run();System.out.println(state[0]);
        IntFn f=capture(state,4);System.out.println(f.apply(5));
        IntFn other=capture(new int[]{10},2);System.out.println(f.getClass()==other.getClass());
        state[0]=20;System.gc();System.out.println(f.apply(1));System.out.println(other.apply(1));
        for(int i=0;i<2000;i++){IntFn temporary=capture(new int[]{i},1);if(temporary.apply(2)!=i+3)throw new AssertionError();}
        System.gc();System.out.println(f.apply(2));
        IntFn referenced=new Box(8)::add;System.out.println(referenced.apply(7));
        Function<Box,String> unbound=Box::label;System.out.println(unbound.apply(new Sub()));
        System.out.println(new Sub().parent().get());System.out.println(new Sub().privateRef().get());
        System.out.println(new PrivateSub().ref().get());
        Make constructor=Box::new;System.out.println(constructor.make(12).label());
        Function<char[],String> builtin=String::new;System.out.println(builtin.apply(new char[]{'o','k'}));
        IntFunction<String[]> arrayFactory=String[]::new;System.out.println(arrayFactory.apply(3).length);
        String[] original={"array"};Supplier<String[]> clone=original::clone;String[] copied=clone.get();System.out.println(copied!=original);System.out.println(copied[0]);
        IntFn synchronizedRef=new Box(1)::bump;System.out.println(synchronizedRef.apply(4));
        Function<String,Integer> boxed=LambdaTest::length;System.out.println(boxed.apply("four"));
        BiConsumer<Integer,Integer> ignored=LambdaTest::add;ignored.accept(3,7);System.out.println(calls);
        ToIntFunction<String> unbox=s->s.length();System.out.println(unbox.applyAsInt("abc"));
        IntFn generic=LambdaTest::identity;System.out.println(generic.apply(17));
        final long l=9000000000L+state[0];final double d=0.25+state[0];final float fl=0.5f+state[0];
        Wide wide=(a,b,c)->a+b+c+l+d+fl;System.out.println((long)wide.call(3L,0.5f,0.75));
        IntFn throwing=LambdaTest::failed;try{throwing.apply(1);}catch(IllegalArgumentException e){System.out.println(e.getMessage());}
        Function raw=boxed;try{raw.apply(new Box(1));}catch(ClassCastException e){System.out.println("cast before target");}
        try{unbound.apply(null);}catch(NullPointerException e){System.out.println("null invocation");}
        try{Box nullBox=null;IntFn fail=nullBox::add;}catch(NullPointerException e){System.out.println("null capture");}
        IntFn marked=(IntFn & Marker)(x->x+2);System.out.println(marked instanceof Marker);System.out.println(marked.apply(3));
        Child child=()->"bridge";Parent<String> parent=child;System.out.println(parent.get());System.out.println(child.get());
        System.out.println("before capture");GetInt late=Late::value;System.out.println("after capture");System.out.println(late.get());
        Thread thread=new Thread(()->{state[0]++;System.gc();});thread.start();thread.join();System.out.println(state[0]);
        IntFn increment=x->x+1;System.out.println(increment.twice(3));
        Consumer<Box> first=b->b.n++;Consumer<Box> second=b->b.n*=3;
        Box chained=new Box(5);first.andThen(second).accept(chained);System.out.println(chained.n);
        System.out.println(Function.<String>identity().apply("library lambda"));
        System.out.println("before default-interface capture");Stateful initialized=()->6;
        System.out.println("after default-interface capture");System.out.println(initialized.twice());System.out.println(Plain.SEED);
    }
}
