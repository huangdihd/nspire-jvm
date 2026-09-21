import java.util.*;
import java.security.*;

public class SortingTest {
    static int calls;
    static class Item implements Comparable<Item> {
        final int key,order;
        Item(int key,int order){this.key=key;this.order=order;}
        public int compareTo(Item other){return key-other.key;}
    }
    static final Comparator<Item> ORDER=new Comparator<Item>() {
        public int compare(Item a,Item b){if((++calls&255)==0)System.gc();return a.key-b.key;}
    };
    static long check(Item[] a) {
        long hash=0;for(int i=0;i<a.length;i++) {
            if(i>0&&(a[i-1].key>a[i].key||(a[i-1].key==a[i].key&&a[i-1].order>a[i].order)))throw new AssertionError("unstable sort");
            hash=hash*31+a[i].key*1000+a[i].order;
        }return hash;
    }
    public static void main(String[] args) {
        System.setProperty("java.util.Arrays.useLegacyMergeSort",args.length>0?"true":"false");
        for(int n:new int[]{0,1,10,31,32,63,128,600}) {
            Item[] a=new Item[n];for(int i=0;i<n;i++)a[i]=new Item((i*197+i/7)%17,i);
            Item[] b=a.clone();Arrays.sort(a,ORDER);Arrays.sort(b);System.out.println(check(a)==check(b));System.out.println(check(a));
            ArrayList<Item> list=new ArrayList<Item>(Arrays.asList(b));list.sort(ORDER);System.out.println(list.size());
        }
        String[] words={"outside","z","a","d","a","outside"};Arrays.sort(words,1,5);System.out.println(Arrays.toString(words));
        int[] ints=new int[1000];for(int i=0;i<ints.length;i++)ints[i]=(i*719)%397-200;
        Arrays.sort(ints);int hash=0;for(int x:ints)hash=hash*31+x;System.out.println(hash);
        double[] numbers={Double.NaN,0.0,-0.0,3.0,-7.0,Double.POSITIVE_INFINITY,Double.NEGATIVE_INFINITY};Arrays.sort(numbers);
        for(double n:numbers){System.out.println((long)n);System.out.println(Double.isNaN(n));System.out.println(1.0/n==Double.NEGATIVE_INFINITY);}
        try{Arrays.sort(new Item[]{new Item(1,0),new Item(0,1)},new Comparator<Item>() {public int compare(Item a,Item b){throw new IllegalStateException("compare");}});}catch(IllegalStateException e){System.out.println("comparator exception caught");}
        final Object expected=new Object();
        Object result=AccessController.doPrivileged(new PrivilegedAction<Object>() {public Object run(){calls++;System.gc();return expected;}});
        System.out.println(result==expected);
        try{AccessController.doPrivileged(new PrivilegedAction<Object>() {public Object run(){throw new IllegalStateException("action");}});}catch(IllegalStateException e){System.out.println("action exception caught");}
        try{AccessController.doPrivileged((PrivilegedAction<Object>)null);}catch(NullPointerException e){System.out.println("null action rejected");}
        System.out.println(java.lang.reflect.Array.newInstance(int.class,3).getClass()==int[].class);
        System.out.println(java.lang.reflect.Array.newInstance(String[].class,2).getClass()==String[][].class);
        try{java.lang.reflect.Array.newInstance(void.class,0);}catch(IllegalArgumentException e){System.out.println("void array rejected");}
        try{java.lang.reflect.Array.newInstance(int.class,-1);}catch(NegativeArraySizeException e){System.out.println("negative array rejected");}
    }
}
