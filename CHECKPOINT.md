# 本机链接异常与临时目录检查点

目标依然是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
完整启动、联网及实机执行尚未完成。

未绑定的 native 方法现在抛出原始 OpenJDK UnsatisfiedLinkError，普通调用、
反射和 lambda 共用异常路径。System.mapLibraryName 支持库名映射与边界检查；
System/Runtime.load 系列报告实际的链接失败，动态 JNI 加载仍未实现。
新增 java.io.tmpdir、主机 --tmpdir、计算器配置第五行，以及整数二/八/十六进制转换。

原始 Xinbot 已进入 Jansi 的库文件提取流程，File.toPath 当前缺少
java.nio.file.FileSystems，尚未执行 Files.copy，也未进入 Xinbot.main。
普通及 ASan/UBSan/泄漏检测构建均复现该停止点，无 sanitizer 错误。
临时目录使用独立测试目录；完整实际堆栈见 XINBOT-RUN.txt。

现有主机回归套件及新增 6 项本机链接/临时目录/整数转换对照检查通过普通
构建和 ASan/UBSan/泄漏检测。文件生命周期检查包含共享描述符、GC、异常退出
和同进程多次 VM 运行。命令与边界见 NATIVE-SUPPORT.md 和 FILE-SUPPORT.md。
这些测试不能等同于完整 Java SE 兼容性或计算器实机验证。

dist/ 中的计算器程序和运行库已同步重建，运行库包含 379 份未修改的上游
Java 源文件。TARGET-RESULTS.txt 记录 ARM/ELF/Zehn 结构检查；
DIST-MANIFEST.json 记录发行文件大小和 SHA-256。

下一步继续实现实际 NIO 文件路径和复制接口，推进 Jansi 与原始应用执行。
JNI、更多标准库接口、网络传输和实机验证仍未完成。
