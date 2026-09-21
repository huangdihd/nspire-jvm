import java.lang.annotation.*;
@Retention(RetentionPolicy.RUNTIME) public @interface Evolved {
    int changed(); int[] array(); VersionEnum phase(); Class<?> absent(); Class<?>[] types();
    String okay() default "ok";
}
enum VersionEnum { OLD, KEPT }
class Removed {}
@Retention(RetentionPolicy.RUNTIME) @interface Vanished {}
@Retention(RetentionPolicy.RUNTIME) @interface ToHidden {}
@Evolved(changed=9,array={3},phase=VersionEnum.OLD,absent=Removed.class,types={Removed.class}) class Stored {}
@Vanished @ToHidden class LostAnnotations {}
