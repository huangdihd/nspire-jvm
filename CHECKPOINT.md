# 控制台接口开发快照

当前源码是未完成的 FileDescriptor/标准输入输出移植，保存于
`wip/timezone-checkpoint` 分支。它导入原始 OpenJDK FileDescriptor、
FileInputStream、FileOutputStream、SyncFailedException 和
JavaIOFileDescriptorAccess，并将描述符读写、关闭及标准流接到 C 平台层。
共享描述符关闭逻辑来自原始 Java 代码；此路径尚未完成对照验证。

运行库编译和主机 C 构建通过。文件测试中 FilePathTest、FileMutationTest
通过，FileStreamTest 因缺少 `java/io/InputStream.markSupported()Z` 停止；
后续文件测试未执行。本次未重新验证完整 Xinbot、ASan 或 ARM 构建。
完整记录与复现命令见 DESCRIPTOR-CHECKPOINT.txt。

`dist/` 与 DIST-MANIFEST.json 保持提交 `4e097f3` 的配套二进制；
既有 RESULTS 文件、TARGET-RESULTS.txt 和 XINBOT-RUN.txt 也属于该检查点。
当前源码新增了标准流对原始 FileDescriptor 类的依赖，测试当前源码前须
按 README 重建 runtime.jar.tns，不能与旧运行库混用。
本开发快照仍不能认定已启动完整 Xinbot，尚无实机运行或联网成功记录。

## 上一次通过验证的时间库检查点：4e097f3

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
