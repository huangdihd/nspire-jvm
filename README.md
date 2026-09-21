# Nspire JVM 0.1（实验版）

> **退出钩子与文件清理检查点。** 已实现原始 OpenJDK 退出钩子、deleteOnExit 和两个 String.join 重载。原始 Xinbot 经过 Jansi 库提取及链接失败处理，当前停在 `Method.getParameters`，仍未进入 main。动态 JNI 加载和计算器实机验证尚未完成。范围见 [CHECKPOINT.md](CHECKPOINT.md)、[SHUTDOWN-SUPPORT.md](SHUTDOWN-SUPPORT.md)。

面向已安装 Ndless 的 TI-Nspire CX II CAS 的 C 字节码解释器。

**目前不能启动完整 Xinbot，也不是 Java SE 17 兼容运行环境。**
本项目实现了可测试的 JVM 子集，目标是在计算器上执行 Java 字节码。
主机对照测试与计算器实机测试是两回事：目前没有完成计算器实机验证。

## 已实现

- 原始 OpenJDK 退出钩子和 deleteOnExit，System/Runtime.exit、Runtime.halt 与嵌入式 VM 返回；并发钩子、逆序删除及错误路径已做主机对照，详见 `SHUTDOWN-SUPPORT.md`。
- String.join 数组与 Iterable 重载接入原始 StringJoiner，空值、UTF-16、自定义字符序列和 GC 已与 Java 8 对照。

- 从多个 JAR/ZIP 或目录读取 `.class`，支持 `.jar.tns` 文件名和独立补充运行库。
- 基础 `Class` 对象：类字面量、getClass、类名、父类、组件类型、直接接口列表、isAssignableFrom、isInstance、cast 和 forName；枚举常量、规范类名和声明类信息。
- 内建 bootstrap/application 类加载器、线程 context loader、JAR/目录资源读取及 ServiceLoader 服务发现。
- 字节码类的构造器查找与调用、参数类型查询、访问检查、Integer/Boolean/Long/Double 拆箱及合法拓宽转换、InvocationTargetException 包装。
- 方法反射：公开继承方法和声明方法查询、类型与异常元数据、访问检查、实际虚调用、八种基本类型的拆箱及合法拓宽、返回值装箱；支持范围见 `METHOD-SUPPORT.md`。
- Byte/Short/Character/Float 的基本装箱与数值接口，以及 Class.getPackage/Package.getName 包查询。
- 原始 OpenJDK 字符集编码器：六种标准编码、字符集查找与服务发现、String 字节转换、分段编码和错误处理；原始堆缓冲区及字节序视图、供编码缓存使用的无队列弱引用。范围与版本差异见 `CHARSET-SUPPORT.md`。
- 运行时注解读取：实际成员和默认值、继承与重复注解、数组复制、相等比较与哈希、成员访问时的类型演化异常；支持范围见 `ANNOTATION-SUPPORT.md`。
- 原始 OpenJDK 时间与时区规则、TZDB 数据、数字日期格式化和已验证的 ISO 日期路径；详见 `TIME-SUPPORT.md`。
- 原始 OpenJDK File 路径处理、目录与文件属性接口；真实 FileInputStream/FileOutputStream、追加/截断、关闭与句柄回收。Ndless 仍缺部分系统操作，详见 `FILE-SUPPORT.md`。新增原始 FileDescriptor/getFD、共享关闭及异常回调、System.in/out/err 描述符连接与 InputStream 默认方法。
- 原始 NIO FileSystems/Paths/Files、Unix 路径算法适配、真实流复制与可寻址文件通道、目录及基本属性；主机独占创建、符号链接和句柄生命周期已对照。Ndless 独占创建等能力尚缺，详见 `NIO-FILE-SUPPORT.md`。
- 部分输入流与 UTF-8 Reader、资源 URL、Integer/Long 装箱缓存、Boolean 单例、Double 数值对象、数组和 Cloneable 对象浅复制。
- 类路径 JAR/文件的只读 URLConnection：连接设置、内容长度、资源流关闭和无缓存 JAR 流的关闭联动。
- Expat 2.8.4 驱动的 SAX XML 解析：命名空间、属性、UTF-8/UTF-16、内部实体、回调异常、错误定位和输入流关闭；真实 Logback 配置的事件结果已对照标准 Java。
- `StringConcatFactory` 字符串拼接：支持引用、整数、long、char、boolean 和配方常量；对象转换调用实际的 toString。
- 编译后的 `LambdaMetafactory` 调用：捕获变量、普通方法/构造器引用、参数转换、标记接口及桥接方法；生成的对象与方法参与现有 GC、异常和线程机制。可序列化 lambda 的捕获、writeReplace 和原始 readResolve 已支持；通用对象流序列化仍缺失。
- 接口默认方法分派及初始化，包含 lambda 的继承默认方法；Java 17 私有方法引用保持直接调用原声明方法。
- String 的字符数组构造、Comparable/CharSequence、前后缀匹配；String.format 支持 `%s`、`%%`、`%n`、参数索引、宽度和字符串精度。
- 原始 OpenJDK Pattern/Matcher 正则引擎，支持已验证的捕获、回溯引用、断言、替换和分割路径；String.matches/replaceAll/replaceFirst/split 调用真实引擎。规范等价及脚本/区块属性仍缺依赖，见 `REGEX-SUPPORT.md`。
- 原始 OpenJDK 输出流、过滤流、缓冲流和内存流；PrintStream 适配层提供实际字节输出、UTF-8、刷新、关闭、错误状态与 System.out/err 重定向。支持原始 Logback 控制台包装类，范围见 `OUTPUT-SUPPORT.md`。
- Unicode 13 大小写转换、Locale 默认值、土耳其语/立陶宛语与希腊 sigma 上下文规则，以及大小写不敏感比较；范围与数据来源见 `CASE-SUPPORT.md`。
- 字符串 contains/indexOf/lastIndexOf 子串查找：UTF-16 下标、空串及实际 CharSequence.toString 调用。
- UTF-16 代理项、码点遍历与 Unicode 13 字符分类/数字属性；原始 StringBuffer 及字符串数组复制接口。
- Double/Float 字符串解析、Double.valueOf(String)，以及查询实际进程环境的 System.getenv(String)。
- 读取 class-file version 45–61；只执行本解释器已实现的指令。
- `int`、`long`、`float`、`double` 的主要运算、转换、分支、两类 switch、wide 局部变量。
- 静态方法、实例方法、递归、继承、接口方法分派、静态初始化、实例与静态字段。
- 基本类型数组、对象数组、多维数组、类型检查和 `System.arraycopy`。
- 本机库名映射、未绑定 native 方法的 UnsatisfiedLinkError（含反射和 lambda 调用）、可配置的 java.io.tmpdir，以及整数二/八/十六进制转换；动态 JNI 加载尚未实现，详见 `NATIVE-SUPPORT.md`。
- Java 异常表，支持显式抛出、跨方法捕获，以及常见运行时异常。
- 标记清扫 GC，根包括执行栈、局部变量、静态字段、字符串常量和本地临时引用。
- 实验性协作式线程、Thread/Runnable、优先级继承及加权轮转、join/sleep/interrupt、可重入 monitor、synchronized 和 wait/notify；GC 扫描挂起线程的根。
- ThreadLocal/InheritableThreadLocal 的隔离、初始值、构造时继承、移除和弱键清理。
- OpenJDK 8 集合补充库：已对照验证 HashMap、ConcurrentHashMap、ArrayList、HashSet、CopyOnWriteArrayList、原子变量、ReentrantLock/Condition 和 LinkedBlockingQueue 的部分路径。
- 原始 AtomicBoolean，包含 CAS 保护共享更新的双线程测试。
- OpenJDK Properties 文件读取、默认值和 Hashtable；对象稳定排序与部分基本类型排序，包含 TimSort 和旧版合并排序路径。
- 原始 OpenJDK Stack、Vector 和 EmptyStackException，包含扩容、迭代、克隆及同步方法。
- 原始 OpenJDK 顺序 Stream 管道：对象和 int/long/double 流、短路匹配、筛选、排序、去重、归约和部分收集器；支持边界见 `STREAM-SUPPORT.md`。
- EnumMap/EnumSet、共享枚举常量缓存和 Enum.valueOf；StringBuilder 的 CharSequence 追加、charAt 和 setLength。
- 运行库所需的字段句柄、原子 CAS/更新、park/unpark 和系统属性；不提供任意本机地址访问。
- 很小的内建运行库：部分 Object、String、StringBuilder、System、PrintStream 方法；本开发分支已改为导入原始 Math/StrictMath，部分本机数学函数尚缺。
- 执行指令预算和 Ndless 下的 ESC 中断检查。

`tests/` 中的 Java 程序使用真实 `javac` 编译；测试工具逐项比较标准 Java 和本解释器的输出。
32 KiB Java 堆测试包含大量临时数组和保留链表，验证 GC 后活对象仍然可用。

## 计算器端使用

若发行包的 `dist/` 中包含 `nspire-jvm.tns`，把以下四个文件传入计算器文档区的**同一个文件夹**：

```text
nspire-jvm.tns
demo.jar.tns
jvm.cfg.tns
runtime.jar.tns
```

安装 Ndless 后打开 `nspire-jvm.tns`。默认示例会计算 Fibonacci、20!，测试数组并捕获除零异常。
执行过程中按 ESC 请求终止，结束后按任意键返回。没有实际设备运行记录，第一次上机仍是移植验证。

`jvm.cfg.tns` 是普通文本，不是 TI 文档，当前示例内容为三行：

```text
demo.jar.tns
Demo
runtime.jar.tns
```

第一行是相对于启动器的 JAR 路径，第二行是入口类名（可使用 `a.b.Main`）。
第三行指定运行库路径；基础控制台现在也需要其中的原始 Java 输出流类。
多个路径用分号分隔，计算器与主机采用相同规则。
主机对应参数为 `-bootclasspath runtime.jar -cp 'app.jar;library.jar'`。
第四行可选填时区，如 `Asia/Hong_Kong`；计算器默认 UTC。主机对应 `--timezone Asia/Hong_Kong`，详见 `TIME-SUPPORT.md`。
第五行可选填已存在的临时目录；第四行不用时保留空行。主机对应 `--tmpdir directory`。默认值为主机 `/tmp`、计算器启动目录；VM 不自动创建或清空目录。
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
./build/nspire-jvm -bootclasspath dist/runtime.jar.tns -cp build/tests/tests.jar CoreTest one two
```

测试脚本会生成 `dist/demo.jar.tns`、`dist/jvm.cfg.tns` 和 `TEST-RESULTS.txt`。
在 WSL 没有 Linux JDK 时，脚本也会查找 `/mnt/c/Program Files/Java/*/bin/` 下的 Windows JDK。
本次实际使用的工具链与兼容修改记录在 `BUILD-NOTES.md`。

重建本分支的 `dist/runtime.jar.tns` 需要 Java 8 JDK 编译器，其 `rt.jar` 仅用来提供编译接口：

```sh
python3 tools/build-runtime.py --java8-home /path/to/java8-jdk --javac /path/to/java8-jdk/bin/javac
python3 tools/test-runtime.py --vm build/nspire-jvm
python3 tools/test-loader.py --vm build/nspire-jvm
python3 tools/test-xml.py --vm build/nspire-jvm
python3 tools/test-lambda.py --vm build/nspire-jvm
python3 tools/test-time.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-native.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-nio-files.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-case.py --vm build/nspire-jvm
python3 tools/test-stream.py --vm build/nspire-jvm
python3 tools/test-annotations.py --vm build/nspire-jvm
python3 tools/test-regex.py --vm build/nspire-jvm
python3 tools/test-output.py --vm build/nspire-jvm --java /path/to/java8/bin/java
python3 tools/test-methods.py --vm build/nspire-jvm
make build/file-lifetime
python3 tools/test-files.py --vm build/nspire-jvm --java /path/to/java8/bin/java --lifetime build/file-lifetime
python3 tools/test-charset.py --vm build/nspire-jvm --java /path/to/java8/bin/java
# 可选：使用自己下载的真实 Xinbot 发布包测试其中的 Logback XML 组件
python3 tools/test-logback-xml.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
python3 tools/test-annotations.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
python3 tools/test-regex.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
python3 tools/test-output.py --vm build/nspire-jvm --java /path/to/java8/bin/java --xinbot /path/to/xinbot.jar
python3 tools/test-methods.py --vm build/nspire-jvm --xinbot /path/to/xinbot.jar
python3 tools/test-charset.py --vm build/nspire-jvm --java /path/to/java8/bin/java --xinbot /path/to/xinbot.jar
```

源码、固定版本和授权位于 `runtime/openjdk8/`，共 443 个未修改的上游源文件；`vendor/openjdk8-nio/` 另保存生成模板、工具和 80 个生成源码。`vendor/openjdk8-time/` 保存时区数据和两份原始加载器，修改后的加载器位于 `runtime/nspire/`，仍保留上游许可证。
当前有 48 项基础检查、6 项运行库对照运行、11 项资源/连接/服务/反射/字符串测试、6 项 lambda/异常传播对照运行、3 项大小写检查、6 项流/枚举/装箱检查、6 项注解检查（含真实 Logback 阶段）、8 项正则及配套运行库检查（含真实 Logback Duration）、7 项输出流检查（含真实 Logback ConsoleTarget）、10 项方法反射及包查询检查（含真实 Logback 属性发现与调用）、8 项字符集/缓冲区/弱引用检查（含真实 Logback 正文与日志头编码）、11 项文件系统/描述符/句柄生命周期检查、3 项 SAX 测试和 1 项真实 Logback XML 组件测试通过普通构建及 ASan/UBSan/泄漏检查；具体结果见对应 RESULTS 文件。另有 8 项时间与日期组件检查、6 项本机链接/临时目录/整数转换检查和 6 项 NIO 路径/复制/通道/句柄生命周期检查，普通构建及 ASan/UBSan/泄漏检查均通过；这不等同于完整标准库兼容性测试。
计算器程序需要补充库时，把 `runtime.jar.tns` 也传入同一文件夹，并将其名称写入 `jvm.cfg.tns` 第三行。

制作自己的简单示例：

```sh
javac --release 8 -d classes MyMain.java
jar cf myapp.jar.tns -C classes .
```

使用 `--release 8` 可以避免现代 javac 默认生成的字符串拼接 `invokedynamic`，但它**不会**把任何 Java 程序自动变成可在本 VM 上运行的程序。

## 尚未实现的关键功能

- 通用 `invokedynamic`、对象流序列化、MethodHandle API、动态常量和完整反射（字段值访问、参数/类型使用位置的注解、泛型等）。方法反射和 lambda 的支持边界分别见 `METHOD-SUPPORT.md`、`LAMBDA-SUPPORT.md`；字符串拼接暂不支持 float/double 的 Java 格式化。
- 完整 Formatter：数字、日期、Locale、Formattable 和格式错误对应的 Java 异常仍未实现，遇到这些路径会给出 VM 诊断。构造器反射尚缺多数内建类构造器、其他装箱类型和 nestmate 访问规则。
- 完整线程语义及并发库兼容性、完整 NIO 文件/通道接口、选择器及直接内存、socket、TLS、DNS、联网驱动。现有线程后端仅经过主机测试，ARM 切换代码尚未实机验证。
- 完整 Java 标准类库、自定义 ClassLoader 命名空间和 defineClass、JNI、插件 JAR 动态加载。现有资源 API 只读启动时指定的类路径，单个资源最多 8 MiB。
- 资源流会一次性读入内存，类路径 JAR 在一次运行期间须保持不变；URLConnection 尚不支持 HTTP、任意 URL 构造、完整元数据和 JAR 热替换。
- 完整 JAXP/XML：当前仅有 SAX 解析子集，外部实体保持禁用；DTD/XSD 验证、DOM、XSLT、SAX1、词法回调、仅凭 URI 打开 XML 等尚未实现。详细范围见 `XML-SUPPORT.md`。
- 字节码安全验证器、Java SE/TCK 兼容性；本版只用于可信的自己编译的程序。
- JAR Manifest 自动入口、多版本 JAR 选择。
- 完整 Unicode/字符串 API 和 Java 浮点数的精确文本格式规则。
- 并行 Stream 的 CountedCompleter/ForkJoinPool 后端，以及 Java 8 之后新增的流接口；Double 对象的 Java 精确文本转换。已补入基本 suppressed-exception 列表，但完整 Throwable 构造器、堆栈与序列化接口仍不完整。
- Locale 的服务提供者、语言标签；泰语字典词边界（影响该 Locale 下的希腊词尾 sigma）。分类默认值已加入本开发分支。
- 旧式 `jsr/ret` 指令及完整的类初始化错误语义。

不支持的功能会报错；不会用空线程、假的网络成功或跳过字节码来冒充兼容。
内建类只提供明确实现的方法，不能当作完整 JDK。

默认 Java 堆上限为 8 MiB，另有最多 16 MiB 的类元数据/解释器分配预算。
ZIP 中央目录等第三方分配不计入这两个预算。最多加载 2,048 个类，调用深度最多 128，
默认指令预算为 1 亿。这些是本版实现限制，不是计算器硬件规格。
当前线程实现最多同时保留 32 个活动线程，每个子线程分配 256 KiB 的 C 栈，计入解释器分配预算。
Expat 另有每个 VM 共计 8 MiB 的本机分配上限；每次 XML 解析的输入字节数最多 8 MiB。

## Xinbot 的真实验证结果

审计对象是官方 `xinbot-2.4.3-RELEASE.jar`，不是自行简化的替代程序。
完整静态清单见 `XINBOT-AUDIT.json`，其中 SHA-256 用于标识本次文件。

尝试命令：

```sh
mkdir -p build/xinbot-tmp
./build/nspire-jvm --tmpdir build/xinbot-tmp -bootclasspath dist/runtime.jar.tns -cp xinbot-2.4.3-RELEASE.jar xin.bbtt.mcbot.Xinbot
```

当前实际结果：

```text
VM error: runtime method not implemented: java/lang/reflect/Method.getParameters()[Ljava/lang/reflect/Parameter;
  at ch/qos/logback/core/ConsoleAppender.lambda$wrapWithJansi$1(Ljava/lang/reflect/Method;)Z pc=1
```

真实 SLF4J 服务发现已找到 Logback 提供者，读取版本属性、生成状态消息，并反射创建配置器。
当前已创建并使用配置事件的 lambda，解析原始 XML，并使用真实 Stream.noneMatch 完成路径匹配。实际注解已用于选择配置处理阶段，Pattern 编译、环境变量替换和 AtomicBoolean 初始化已通过。现在成功创建 Xinbot 自身的 JLineConsoleAppender，BeanDescriptionFactory 已完成继承方法发现和类型查询。时间库、TZDB 和可序列化 lambda 协议现已让原始日期转换器完成初始化。实际 ConsoleAppender 随后按原始配置启用 Jansi，已创建描述符输出流并设置清理线程优先级，库名映射、临时目录和 NIO 复制已让它实际提取出 18,976 字节的库文件，其内容与原始 JAR 一致；现在 File.deleteOnExit 已登记退出清理，Jansi 捕获真实的 JNI 加载失败，String.join 已构造错误消息；Logback 随后停在 Method.getParameters，尚未进入 Xinbot.main。完整堆栈见 `XINBOT-RUN.txt`。
另行直接调用同一 Xinbot JAR 中未修改的 Logback SaxEventRecorder，已从原始 `logback.xml` 得到与标准 Java 一致的 27 个事件；见 `LOGBACK-XML-RESULTS.txt`。这是一项组件测试，完整启动仍未通过。
真实 Logback 的 9 个相关类的注解阶段读取、Duration 时长解析及 ConsoleTarget 输出包装也与标准 Java 一致，见 `LOGBACK-ANNOTATION-RESULTS.txt`、`LOGBACK-DURATION-RESULTS.txt` 和 `LOGBACK-CONSOLE-RESULTS.txt`。原始属性发现与 setter/getter 调用见 `LOGBACK-BEAN-RESULTS.txt`，字符集属性转换和日志正文编码见 `LOGBACK-CHARSET-RESULTS.txt`。日志头编码也已通过对照。原始 CachingDateFormatter 的组件对照见 `LOGBACK-DATE-RESULTS.txt`。仍需补齐 Jansi 本机库加载与平台接口、剩余时间/时区接口、设备文件系统缺口、更多动态调用路径、完整线程语义和网络支持。
JAR 中含 10,719 个基础类、5,507 个 InvokeDynamic 常量池条目，另含部分可选的 Java 22 FFM 类。
这不意味着每次启动都会加载所有类，也不意味着仅凭这些可选类就能断定最低 Java 版本是 22。

继续研发应以真实 Xinbot 的下一处加载/执行失败为依据扩展功能，并在模拟器或实机上验证内存和系统接口。
要连接 Minecraft 服务器，还需要确定并实现计算器的实际联网通道；本项目当前没有提供联网能力。

## 源码

`src/vm.c` 是解释器、类加载器、对象堆和最小运行库；`src/main.c` 是主机/Ndless 入口。
`src/threads.inc` 实现协作式调度与 monitor；`src/context.*`、`src/context_arm.S` 提供主机及 ARM 栈切换。
`src/unsafe.inc` 为 OpenJDK 提供经过对象边界检查的字段访问和原子操作。
`src/loader.inc` 实现类加载器与资源 API，`src/indy.inc` 实现字符串拼接 bootstrap。
`src/lambda.inc` 生成 lambda 捕获对象和字节码桥接方法；`src/split.inc` 实现单字符分割路径。
`src/case.inc` 和固定数据表实现 Unicode 大小写及词边界，`src/search.inc` 实现子串查找。
`src/enums.inc` 提供枚举常量和名字缓存；`src/builder.inc` 支持流收集器使用的 CharSequence 操作。
`src/reflection.inc` 实现构造器反射，`src/format.inc` 实现上述字符串格式化子集。
`src/methods.inc` 实现方法发现与调用，`src/boxing.inc` 补充基本类型包装类；内建类的反射声明数据位于 `vendor/openjdk8-api/`，声明不代表已实现该 API。
`src/annotations.inc` 读取运行时注解、默认值，并提供注解成员访问器。
`src/regex.inc` 把 String 正则入口接到实际类库；`src/character.inc` 提供字符属性与码点接口，`src/parse_number.inc` 和 `src/environment.inc` 提供上述数值解析与环境查询。
`src/output.inc` 把 PrintStream 接到 Java 输出流或主机/Ndless 控制台，保留回调、监视器与 GC 根。
`src/filesystem.inc` 提供原始 UnixFileSystem 的本机绑定，`src/descriptors.inc` 连接原始 Java 文件描述符与平台句柄；`src/canonical.c` 编译原始 OpenJDK 路径规范化算法。
`src/nio_files.inc` 提供 NIO 的平台文件操作；`runtime/nspire/sun/nio/fs/` 包含默认提供者、路径适配和通道。
`src/charset.inc` 把 String 字节转换交给 `nspire.charset.StringCoding` 和真实 Java 编码器；`tools/generate-nio.py` 复现原始 NIO 生成源码。
`src/xml.inc` 与 `runtime/nspire/` 把 Expat 解析事件交给真实 Java SAX 回调。
`src/identifiers.inc` 是由 `tools/GenerateIdentifiers.java` 生成的 Java 标识符字符范围表。
`vendor/miniz.*` 仅用于读取压缩 JAR，`vendor/expat/` 提供 XML 解析。授权和来源见 `LICENSE`、`THIRD-PARTY.md`。
