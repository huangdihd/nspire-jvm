# 原始 Xinbot 已进入 main 的检查点

目标仍是在 TI-Nspire CX II CAS 上通过 Ndless 本地运行原始 Xinbot。
目前主机解释器已进入原始 Xinbot.main；完整启动、联网及实机执行均未完成。

已加入 Record 基类、真实 Record 属性检测，以及 javac 生成的 ObjectMethods
字段相等比较、哈希和文本生成。引用执行真实回调；数组保留引用相等语义，
浮点 NaN 与正负零按 Java 规则处理。浮点字段文本使用原始 FloatingDecimal
和 FDBigInteger 字节码，运行库现有 451 份原始 Java 源文件及 80 份生成源码。

Integer.parseInt/Long.parseLong 支持十进制与显式进制、Unicode 数字、正负号、
溢出及真实异常；新增按进制转字符串。原始 Version.from 及 record 的访问器、
比较、equals/hashCode 已有 Java 17 对照。八项检查覆盖 record 常规行为、
312 组浮点文本、整数转换、三个错误 bootstrap、移除 Record 属性，以及原始
Xinbot Version 组件。具体命令和限制见 RECORD-SUPPORT.md、RECORD-RESULTS.txt。
八项新增检查和既有完整回归均通过普通及 ASan/UBSan/泄漏检测构建。
Ndless ARM 构建与 ELF/Zehn 检查通过，记录见 TARGET-RESULTS.txt。

原始 Xinbot 通过日志和版本初始化后，在 main 的 pc=2 进入 LangManager，
后者在静态初始化 pc=35 调用 Map.of 时失败。错误当前显示 Object.of，
原始字节码确认实际目标是 java/util/Map.of；需要同时补齐不可变集合工厂
和缺失静态方法的准确查找。运行命令与哈希见 XINBOT-RUN.txt。

Jansi 实际提取的 18,976 字节 Linux 库仍与原始 JAR 完全一致；动态 JNI
尚未实现，Jansi 自身捕获该失败。致命解释器错误跳过 Java 退出钩子，
测试驱动清理独立临时目录。正常返回/System.exit 清理另有组件验证。

ARM 构建与 ELF/Zehn 检查不等于实机验证。RecordComponent、通用 MethodHandle、
完整 Formatter、更多 JDK 路径、设备文件系统和网络仍未完成。
本产物是研发检查点，不能当作已经能够完整运行 Xinbot 的发行版。
