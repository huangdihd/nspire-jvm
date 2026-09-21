# 参数反射与包元数据检查点

目标仍是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
当前完整启动、联网及实机执行均未完成。

新增真实 Method/Constructor 参数反射：读取 MethodParameters 中的名称和标志，
保留声明方法、类型、可变参数和参数注解；Parameter 对象执行原始 OpenJDK 字节码。
数组复制、缓存、GC 和错误元数据路径均有 Java 8 对照。泛型 Signature 已保留，
但泛型解析、重复参数注解和类型使用位置的注解仍未完成，遇到相关路径明确报错。

包版本等六项元数据现从实际 JAR Manifest 读取，使用原始 Manifest/Attributes；
覆盖包专属字段、主字段回退、UTF-8 折行、缺失值及首次类定义顺序。
同时补入 Manifest 使用的旧式 high-byte String 构造器。
运行库包含 447 份未修改的 OpenJDK Java 源文件及 80 份生成源码。

普通及 ASan/UBSan/泄漏检测构建均通过 12 项参数检查、3 项包元数据/字符串检查，
以及既有完整回归。命令、结果和边界见 PARAMETER-SUPPORT.md、
PACKAGE-METADATA-SUPPORT.md 及对应 RESULTS 文件。
原有退出钩子、文件清理与 String.join 功能保留，
实际 JansiLoader 正常返回/System.exit 清理另有组件检查。

原始 Xinbot 已通过 Logback 的参数反射查询并读取包实现版本。其静态初始化器
调用 Version.from 时，在 pc=31 因缺少 java.lang.Record 失败，尚未进入 main。
普通及 ASan/UBSan/泄漏检测构建均复现此结果。Jansi 真实提取的 18,976 字节
Linux 库与 JAR 内容完全一致，但动态 JNI 加载仍未实现；Jansi 自身处理该失败。
解释器致命错误跳过 Java 退出钩子，测试驱动清理独立临时目录。
完整命令、SHA-256 和堆栈见 XINBOT-RUN.txt。

ARM 产物已重建，ELF/Zehn 记录见 TARGET-RESULTS.txt；这不证明实机运行。
下一步需要 record 类与 ObjectMethods bootstrap、后续真实调用路径、网络及
设备验证。当前产物是开发检查点，不是已经能够完整运行 Xinbot 的发行版。
