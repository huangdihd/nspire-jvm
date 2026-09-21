# 时间库研发检查点

本检查点让原始 Xinbot 越过 Logback 日期转换器，推进到 Jansi 控制台初始化。
当前实际失败为缺少 `java/io/FileDescriptor`，仍在 `Xinbot.main` 之前。
目标依然是在 TI-Nspire CX II CAS 上本地运行原始 Xinbot；完整启动、联网和实机执行尚未完成。

已加入原始 OpenJDK 时间/日历代码、TZDB 2026b、fdlibm log/sqrt、可序列化 lambda 的
writeReplace/readResolve 协议，以及实际触发的字符串、异常和整数接口。
日期、历史时区、夏令时、原始 Logback 格式器、4,585 组数学输入和 lambda 捕获/还原
已有标准 Java 对照。具体范围与缺口见 TIME-SUPPORT.md、LAMBDA-SUPPORT.md。

本次已重建配套的 dist/nspire-jvm.tns 和 dist/runtime.jar.tns。
主机检查结果见各 RESULTS 文件，ARM 结构信息见 TARGET-RESULTS.txt，
完整原始应用的失败堆栈见 XINBOT-RUN.txt。DIST-MANIFEST.json 记录当前文件大小及 SHA-256。
没有计算器或带合法系统镜像的模拟器运行记录；这些文件仍用于移植验证，不是可运行完整 Xinbot 的发行版。

后续需要实现控制台 FileDescriptor/流及 Jansi 平台依赖，再根据原始应用执行结果继续补齐。
还需实际设备测试、网络通道及相关 Java 网络接口，不能用主机组件测试代替最终目标验证。
