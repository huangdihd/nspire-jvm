import java.lang.annotation.*;
@Retention(RetentionPolicy.RUNTIME) public @interface Evolved {
    String changed(); String[] array(); VersionEnum phase(); Class<?> absent(); Class<?>[] types();
    String okay() default "ok"; int added();
}
enum VersionEnum { KEPT }
@Retention(RetentionPolicy.CLASS) @interface ToHidden {}
