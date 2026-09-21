# Nspire JVM 0.1（实验版）

面向已安装 Ndless 的 TI-Nspire CX II CAS 的 C 字节码解释器。

**目前不能启动完整 Xinbot，也不是 Java SE 17 兼容运行环境。**
本项目实现了可测试的 JVM 子集，目标是在计算器上执行 Java 字节码。
主机对照测试与计算器实机测试是两回事：目前没有完成计算器实机验证。

## 已实现

- 从多个 JAR/ZIP 或目录读取 `.class`，支持 `.jar.tns` 文件名和独立补充运行库。
- 基础 `Class` 对象：类字面量、getClass、类名、父类、组件类型、isAssignableFrom、isInstance、cast 和单参数 forName。
- 读取 class-file version 45–61；只执行本解释器已实现的指令。
- `int`、`long`、`float`、`double` 的主要运算、转换、分支、两类 switch、wide 局部变量。
- 静态方法、实例方法、递归、继承、接口方法分派、静态初始化、实例与静态字段。
- 基本类型数组、对象数组、多维数组、类型检查和 `System.arraycopy`。
- Java 异常表，支持显式抛出、跨方法捕获，以及常见运行时异常。
- 标记清扫 GC，根包括执行栈、局部变量、静态字段、字符串常量和本地临时引用。
- 实验性协作式线程、Thread/Runnable、join/sleep/interrupt、可重入 monitor、synchronized 和 wait/notify；GC 扫描挂起线程的根。
- ThreadLocal/InheritableThreadLocal 的隔离、初始值、构造时继承、移除和弱键清理。
- OpenJDK 8 集合补充库：已对照验证 HashMap、ConcurrentHashMap、ArrayList、HashSet、原子变量、ReentrantLock/Condition 和 LinkedBlockingQueue 的部分路径。
- 运行库所需的字段句柄、原子 CAS/更新、park/unpark 和系统属性；不提供任意本机地址访问。
- 很小的内建运行库：部分 Object、String、StringBuilder、System、PrintStream、Math 方法。
- 执行指令预算和 Ndless 下的 ESC 中断检查。

`tests/` 中的 Java 程序使用真实 `javac` 编译；测试工具逐项比较标准 Java 和本解释器的输出。
32 KiB Java 堆测试包含大量临时数组和保留链表，验证 GC 后活对象仍然可用。

## 计算器端使用

若发行包的 `dist/` 中包含 `nspire-jvm.tns`，把以下三个文件传入计算器文档区的**同一个文件夹**：

```text
nspire-jvm.tns
demo.jar.tns
jvm.cfg.tns
```

安装 Ndless 后打开 `nspire-jvm.tns`。默认示例会计算 Fibonacci、20!，测试数组并捕获除零异常。
执行过程中按 ESC 请求终止，结束后按任意键返回。没有实际设备运行记录，第一次上机仍是移植验证。

`jvm.cfg.tns` 是普通文本，不是 TI 文档，内容为两行：

```text
demo.jar.tns
Demo
```

第一行是相对于启动器的 JAR 路径，第二行是入口类名（可使用 `a.b.Main`）。
可选第三行指定补充运行库路径。多个路径用分号分隔，计算器与主机采用相同规则。
主机对应参数为 `-bootclasspath runtime.jar -cp 'app.jar;library.jar'`。
补充运行库先于应用路径查找，但内建的基础类仍由 VM 提供；这尚不是完整的 Java ClassLoader 模型。
JAR 的 Manifest `Main-Class` 尚未读取，需要显式指定入口类。
这里的 `.jar.tns` 只是方便传输的 ZIP/JAR；启动器负责读取它，并没有把 JAR 转成原生代码。

## 构建和测试

已有 Ndless SDK 的 Linux/WSL 环境：

```sh
make ndless
```

主机测试使用 Linux/WSL，需要 C99 编译器、POSIX ucontext、Python 3.11+ 和 JDK 17+：

```sh
make host
python3 tools/test.py --vm build/nspire-jvm
./build/nspire-jvm -cp build/tests/tests.jar CoreTest one two
```

测试脚本会生成 `dist/demo.jar.tns`、`dist/jvm.cfg.tns` 和 `TEST-RESULTS.txt`。
在 WSL 没有 Linux JDK 时，脚本也会查找 `/mnt/c/Program Files/Java/*/bin/` 下的 Windows JDK。
本次实际使用的工具链与兼容修改记录在 `BUILD-NOTES.md`。

重建 `dist/runtime.jar.tns` 需要额外准备一个 Java 8 JRE/JDK，其 `rt.jar` 仅用来提供编译接口：

```sh
python3 tools/build-runtime.py --java8-home /path/to/java8
python3 tools/test-runtime.py --vm build/nspire-jvm
```

源码、固定版本和授权位于 `runtime/openjdk8/`。本次有 38 项基础检查和 2 个运行库对照程序通过普通构建及 ASan/UBSan；这不等同于完整标准库兼容性测试。
计算器程序需要补充库时，把 `runtime.jar.tns` 也传入同一文件夹，并将其名称写入 `jvm.cfg.tns` 第三行。

制作自己的简单示例：

```sh
javac --release 8 -d classes MyMain.java
jar cf myapp.jar.tns -C classes .
```

使用 `--release 8` 可以避免现代 javac 默认生成的字符串拼接 `invokedynamic`，但它**不会**把任何 Java 程序自动变成可在本 VM 上运行的程序。

## 尚未实现的关键功能

- `invokedynamic`、MethodHandle、动态常量和完整反射（方法/字段反射、注解、泛型等）。
- 完整线程语义及并发库兼容性、NIO、socket、TLS、DNS、联网驱动。现有线程后端仅经过主机测试，ARM 切换代码尚未实机验证。
- 完整 Java 标准类库、ClassLoader 扩展、JNI、资源加载、插件 JAR 动态加载。
- 字节码安全验证器、Java SE/TCK 兼容性；本版只用于可信的自己编译的程序。
- JAR Manifest 自动入口、多版本 JAR 选择。
- 完整 Unicode/字符串 API 和 Java 浮点数的精确文本格式规则。
- 旧式 `jsr/ret` 指令及完整的类初始化错误语义。

不支持的功能会报错；不会用空线程、假的网络成功或跳过字节码来冒充兼容。
内建类只提供明确实现的方法，不能当作完整 JDK。

默认 Java 堆上限为 8 MiB，另有最多 16 MiB 的类元数据/解释器分配预算。
ZIP 中央目录等第三方分配不计入这两个预算。最多加载 512 个类，调用深度最多 128，
默认指令预算为 1 亿。这些是本版实现限制，不是计算器硬件规格。
当前线程实现最多同时保留 32 个活动线程，每个子线程分配 256 KiB 的 C 栈，计入解释器分配预算。

## Xinbot 的真实验证结果

审计对象是官方 `xinbot-2.4.3-RELEASE.jar`，不是自行简化的替代程序。
完整静态清单见 `XINBOT-AUDIT.json`，其中 SHA-256 用于标识本次文件。

尝试命令：

```sh
./build/nspire-jvm -bootclasspath dist/runtime.jar.tns -cp xinbot-2.4.3-RELEASE.jar xin.bbtt.mcbot.Xinbot
```

当前实际结果：

```text
VM error: runtime method not implemented: java/lang/Class.getClassLoader()Ljava/lang/ClassLoader;
  at org/slf4j/LoggerFactory.findServiceProviders()Ljava/util/List; pc=10
  at org/slf4j/LoggerFactory.bind()V pc=0
  at org/slf4j/LoggerFactory.performInitialization()V pc=0
  at org/slf4j/LoggerFactory.getProvider()Lorg/slf4j/spi/SLF4JServiceProvider; pc=21
  at org/slf4j/LoggerFactory.getILoggerFactory()Lorg/slf4j/ILoggerFactory; pc=0
  at org/slf4j/LoggerFactory.getLogger(Ljava/lang/String;)Lorg/slf4j/Logger; pc=0
  at xin/bbtt/mcbot/Xinbot.<clinit>()V pc=5
```

即使补齐这一项，仍需实现上列的运行库、动态调用、完整线程语义和网络支持。
JAR 中含 10,719 个基础类、5,507 个 InvokeDynamic 常量池条目，另含部分可选的 Java 22 FFM 类。
这不意味着每次启动都会加载所有类，也不意味着仅凭这些可选类就能断定最低 Java 版本是 22。

继续研发应以真实 Xinbot 的下一处加载/执行失败为依据扩展功能，并在模拟器或实机上验证内存和系统接口。
要连接 Minecraft 服务器，还需要确定并实现计算器的实际联网通道；本项目当前没有提供联网能力。

## 源码

`src/vm.c` 是解释器、类加载器、对象堆和最小运行库；`src/main.c` 是主机/Ndless 入口。
`src/threads.inc` 实现协作式调度与 monitor；`src/context.*`、`src/context_arm.S` 提供主机及 ARM 栈切换。
`src/unsafe.inc` 为 OpenJDK 提供经过对象边界检查的字段访问和原子操作。
`vendor/miniz.*` 仅用于读取压缩 JAR。授权和来源见 `LICENSE`、`THIRD-PARTY.md`。
