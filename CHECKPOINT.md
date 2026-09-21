# 退出钩子与文件清理检查点

目标仍是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
当前完整启动、联网及实机执行均未完成。

运行库保留原始 OpenJDK Shutdown、ApplicationShutdownHooks、DeleteOnExitHook、
IdentityHashMap 和 ThreadDeath，现有 443 份原始 Java 源文件及 80 份生成源码。
System/Runtime.exit 执行真正的钩子线程并等待，deleteOnExit 随后逆序删除文件；
Runtime.halt 直接终止当前 VM。所有路径均返回嵌入式调用方并清理本机资源。
自然退出另建线程身份，原始 main 在钩子执行时保持终止状态。两个 String.join
重载也已接入原始 StringJoiner。

普通及 ASan/UBSan/泄漏检测构建通过 14 项退出检查：10 个 Java 8 返回码及
磁盘对照、3 个 VM 致命错误路径和同进程 20 次启动的资源/返回码检查。
String.join 的数组、迭代、自定义字符序列、UTF-16 和 GC 对照通过。
另有两项直接调用原始 JansiLoader 的组件检查，确认真实提取字节一致，
正常返回与 System.exit(17) 后库文件和锁文件均由 Java 退出清理删除。
既有完整回归和 NIO 检查也已执行；结果见各 RESULTS 文件。

原始 Xinbot 实际提取出的 18,976 字节 Jansi 库仍与 JAR 原文件完全一致。
现在已登记退出删除，并由 Jansi 捕获真实 JNI 链接失败；随后 Logback 停在
Method.getParameters。普通及内存检查构建均复现，未进入 Xinbot.main。
由于停止原因是解释器致命错误，该尝试不执行 Java 退出钩子，临时文件由
测试驱动的独立目录清理。完整命令、SHA-256 和堆栈见 XINBOT-RUN.txt。

ARM 产物已重建，ELF/Zehn 记录见 TARGET-RESULTS.txt；这不证明实机运行。
动态 JNI、设备文件系统缺口、信号退出、ThreadGroup 异常处理、finalizer、
后续 JDK 动态路径、网络和设备验证仍未完成。下一步补真实反射 Parameter
元数据并继续原始应用执行。具体边界见 SHUTDOWN-SUPPORT.md。
