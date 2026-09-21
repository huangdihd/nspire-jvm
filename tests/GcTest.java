class Node { Node next; int value; Node(Node n,int x){next=n;value=x;} }
public class GcTest {
    static Node saved;
    public static void main(String[] args) {
        Node head=null;
        for(int i=0;i<60;i++)head=new Node(head,i);
        saved=head;
        for(int i=0;i<10000;i++) {
            int[] garbage=new int[80]; garbage[0]=i;
            if(i%1000==0)System.gc();
        }
        int total=0;while(head!=null){total+=head.value;head=head.next;}
        System.out.println(total);System.out.println(saved.next.value);
        String text="live string";
        for(int i=0;i<300;i++){String s="count="+i; if(i==299)System.out.println(s);}
        System.gc();System.out.println(text);
    }
}
