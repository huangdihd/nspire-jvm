# NIO 路径与文件复制检查点

目标仍是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
当前完整启动、联网及实机执行均未完成。

新增原始 OpenJDK FileSystems、Paths、Files、URI 和通道流适配，运行库包含
438 份原始 Java 源文件及 80 份按上游规则生成的源码。NspirePath 保留原始
Unix 路径算法，平台提供者接入实际文件描述符、目录和基本属性。
流复制、独占创建、覆盖失败、部分复制、寻址、追加、截断和关闭均有主机对照。

原始 Xinbot 实际提取出了 18,976 字节的 Jansi Linux x86_64 库，字节与
原始 JAR 完全一致。当前停止在 File.deleteOnExit 缺少 java.io.DeleteOnExitHook。
普通及 ASan/UBSan/泄漏检测构建均复现，未进入 Xinbot.main，也未加载 JNI 库。
命令、SHA-256 和堆栈见 XINBOT-RUN.txt。

完整既有回归及 6 项 NIO 检查通过普通和 ASan/UBSan/泄漏检测构建。
NIO 检查包含与 Linux Java 8 的输出和磁盘内容对照、64 句柄限制下的 GC
和异常退出，以及同进程多次 VM 运行的描述符计数。见 NIO-FILE-RESULTS.txt。
计算器产物已重建，ARM/ELF/Zehn 结构记录见 TARGET-RESULTS.txt；这不证明实机运行。

Ndless 独占创建、部分文件选项与属性操作尚缺；Path-to-Path 复制、move、
映射/锁/异步通道、watch、网络传输及动态 JNI 也未完成。详细边界见
NIO-FILE-SUPPORT.md。下一步继续退出清理与原始应用的实际执行路径。
