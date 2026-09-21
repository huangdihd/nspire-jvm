import java.io.*;
public class FileGcTest {
    static void abandoned()throws Exception {
        FileInputStream input=new FileInputStream("data");
        FileInputStream inputAlias=new FileInputStream(input.getFD()); input=null;
        System.gc(); if(inputAlias.read()!='i')throw new AssertionError();
        FileOutputStream output=new FileOutputStream("gc-output",true);
        FileOutputStream outputAlias=new FileOutputStream(output.getFD()); output=null;
        System.gc(); outputAlias.write(7);
        // Both alias groups become unreachable here; each shared handle closes once.
    }
    public static void main(String[] args)throws Exception {
        for(int i=0;i<200;i++){abandoned();System.gc();}
        System.out.println(new File("gc-output").length());
        // Leave these live at VM return: vm_run must close them as well.
        new FileInputStream("data");new FileOutputStream("exit-output").write(11);
        if(args.length>0)String.format("%f",1.0); // explicit fatal path after opening
    }
}
