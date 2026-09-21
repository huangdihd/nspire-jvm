# Third-party code and build references

`vendor/miniz.c` and `vendor/miniz.h` were copied, without source changes, from
digitalgust/miniJVM at commit `ac94e62781deda037875ff69d78f272a327a72bc`:
https://github.com/digitalgust/miniJVM/tree/ac94e62781deda037875ff69d78f272a327a72bc/minijvm/c/utils

Their original copyright/license notices are preserved in the files. Only the
ZIP reader/inflater is used. This project does **not** use miniJVM's interpreter
or claim miniJVM's Java compatibility. Our VM implementation is in `src/vm.c`.

Ndless SDK build reference:
https://github.com/ndless-nspire/Ndless/tree/9484d8da7c7a4dde9766138c2e42e1d1e3acfcd4

nspire-io dependency:
https://github.com/Vogtinator/nspire-io/tree/d084a245286c073d161e1ed602788946aff79aee

Ndless and nspire-io are external build dependencies with their own licenses.
The source archive does not relicense either project. See BUILD-NOTES.md for
the exact task-local build procedure and any compatibility changes.

Xinbot release audited (not redistributed in this package):
https://github.com/huangdihd/xinbot/releases/tag/2.4.3-RELEASE

The audit is a static class-pool inventory and a separate attempted host run.
Optional Java 22 classes in the release JAR do not, by themselves, establish
that Java 22 is required on every execution path.
