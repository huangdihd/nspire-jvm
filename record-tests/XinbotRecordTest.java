import xin.bbtt.mcbot.versions.Version;
public class XinbotRecordTest {
    static void check(boolean value){if(!value)throw new AssertionError();}
    public static void main(String[] args){
        Version a=Version.from("  2.4.3-release  ");
        Version b=Version.from("2.4.3-RELEASE");
        Version c=Version.from("2.4.4-RELEASE");
        check(Version.class.isRecord()); check(a.equals(b)&&a.hashCode()==b.hashCode()&&!a.equals(c));
        check(a.major()==2&&a.minor()==4&&a.patch()==3&&a.stage().name().equals("RELEASE"));
        check(a.hashCode()==31*(31*(31*a.major()+a.minor())+a.patch())+a.stage().hashCode());
        check(a.compareTo(b)==0&&a.compareTo(c)<0&&c.compareTo(a)>0);
        check(a.isOlderThan(c)&&c.isNewerThan(a)&&a.isAtLeast(b)&&a.isAtMost(b));
        try{Version.from("not-a-version");throw new AssertionError();}catch(IllegalArgumentException expected){System.out.println(expected.getMessage());}
        System.out.println("original Xinbot Version parsing, accessors, comparison, equals and hashCode passed");
    }
}
