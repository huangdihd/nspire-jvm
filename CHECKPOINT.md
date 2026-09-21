# 控制台与描述符检查点

目标依然是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
当前完整启动、联网及实机执行尚未完成；没有用修改应用或跳过日志配置替代目标。

原始 OpenJDK FileDescriptor、FileInputStream、FileOutputStream 和 InputStream
已接入真实平台句柄。文件流共享读写位置，显式关闭执行原始 Java 回调与异常聚合；
GC 和 VM 退出释放拥有的句柄。标准流使用共享描述符，支持实际输入输出与重定向。
Ndless 的行输入适配和存储行为仍待实机验证；目标端 sync 明确抛出 SyncFailedException。

原始 Xinbot 已越过 FileDescriptor 和 Thread.setPriority，当前停止在：
`System.mapLibraryName(String)`，调用方是 JansiLoader.loadJansiNativeLibrary。
这仍在 Xinbot.main 之前。完整实际执行堆栈见 XINBOT-RUN.txt。

线程优先级按创建线程继承，支持 1–10 的范围检查与运行期间更新。
协作式调度用优先级决定字节码时间片长度，并保留显式 yield 和阻塞时的让出。
ThreadGroup、安全管理器及完整 Java 线程语义仍未实现。

文件/控制台测试包括共享描述符、关闭回调与 suppressed 异常、真实磁盘内容、
标准输入管道和标准输出的逐字节对照，以及低句柄上限下的 GC、异常退出和多次 VM 运行。
验证命令、细节及限制见 FILE-SUPPORT.md 和 DESCRIPTOR-CHECKPOINT.txt。
其他接口的验证结果分别保存在对应 RESULTS 文件，不能等同于完整 Java SE 兼容性。

dist/ 中的计算器程序和运行库已同步重建。TARGET-RESULTS.txt 是 ARM/ELF/Zehn
结构检查；DIST-MANIFEST.json 记录文件大小和 SHA-256。它们不证明实机成功运行。
下一步继续补齐 Jansi 本机库加载及平台调用，并推进原始应用的真实执行路径。
