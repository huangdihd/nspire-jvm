import java.io.*;
public class FileGcTest {
    static void abandoned()throws Exception{new FileInputStream("data");new FileOutputStream("gc-output",true).write(7);}
    public static void main(String[] args)throws Exception {
        for(int i=0;i<200;i++){abandoned();System.gc();}
        System.out.println(new File("gc-output").length());
        // Leave these live at VM return: vm_run must close them as well.
        new FileInputStream("data");new FileOutputStream("exit-output").write(11);
        if(args.length>0)String.format("%f",1.0); // explicit fatal path after opening
    }
}
