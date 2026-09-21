# 实际构建记录

目标：TI-Nspire CX II CAS / Ndless / ARM926EJ-S、ARM 模式、软件浮点。
构建平台：Windows 上的 Ubuntu WSL。

实际工具版本：

- ARM GCC：Ubuntu `15:13.2.rel1-2`，编译器输出 `13.2.1 20231009`。
- binutils-arm-none-eabi：Ubuntu `2.42-1ubuntu1+23`。
- newlib：Ubuntu `4.4.0.20231231-2`。
- Ndless：`9484d8da7c7a4dde9766138c2e42e1d1e3acfcd4`。
- nspire-io：`d084a245286c073d161e1ed602788946aff79aee`。

编译器、newlib、Boost 包通过 `apt-get download` 下载并用 `dpkg-deb -x` 解包到任务目录。
没有运行系统软件安装，也没有修改用户的 Ndless 配置目录。
实际构建使用 Ndless 的库、链接脚本、启动汇编和 `genzehn`，不是把其他平台的 ELF 重命名成 `.tns`。

## 任务内 SDK 修改

随包提供 `vendor/ndless-sources.zip`，包含本次使用的 Ndless 和 nspire-io 对应源码、原有许可证，及以下修改：

1. SDK shell 脚本转换成 LF，解决 Windows 检出后的 shebang 问题。
2. `tools/genzehn/zehn.h` 的 Git 软链接在 Windows 被检出为文本路径；改为复制 `include/zehn.h` 内容。
3. `libsyscalls/stdlib.cpp` 增加 `#ifndef PATH_MAX / #define PATH_MAX 1024 / #endif`。
4. `bin/nspire-gcc` 和 `bin/arm-none-eabi-ld.gold` 不创建 `$HOME/.ndless` 子目录。
5. 链接包装脚本省去 `-lstdc++`：本应用为 C 代码，链接到的 SDK/nspire-io 代码不需要 C++ 标准库。
   仍保留真实的未定义符号检查，没有加入假的 C++ 库。

应用的 `src/ndless_compat.c` 提供一个弱 `_fini`：通用裸机 newlib 引用这个 ELF hook，
而 Ndless 使用自己的 `__cpp_init/__cpp_fini` 序列。本 C 程序没有额外 `.fini` 工作。

`.tns` 直接采用现代 Ndless 的 Zehn 格式，不附加旧版 Ndless 的兼容加载器。
本程序面向 CX II CAS 上的现代 Ndless，不承诺兼容旧型号或古老 Ndless 版本。

## 复现

已有正常 Ndless SDK：`make ndless`。

使用 Ubuntu 24.04 的本地包构建方案：

```sh
bash tools/build-local-sdk.sh
```

该脚本需要网络访问 Ubuntu 软件源，需要已有 `gcc/g++/make/python3/dpkg-deb/apt-get/zlib1g-dev`。
它解包在 `build/toolchain/`，不需要 Docker 或 sudo。软件源未来可能更新，版本以命令输出为准。
随包的 SDK 对应源码允许重新编译与替换所链接的库。

## 验证范围

- 主机运行：标准 Java 对照测试，包括目录和压缩 JAR；见 `TEST-RESULTS.txt`。
- 内存检查：当前 45 项基础检查、6 项运行库对照运行、11 项资源/连接/服务/反射/字符串测试、2 项 lambda 对照运行、3 项大小写检查和 6 项流/枚举/装箱检查均通过 AddressSanitizer、UndefinedBehaviorSanitizer 与泄漏检测；预期失败的测试也检查 sanitizer 输出。
- XML 检查：3 项 SAX 测试与 1 项真实 Logback XML 组件测试也通过上述检查；包含回调异常、嵌套解析、线程切换、GC 与解析中 VM 中止的资源清理。
- 注解检查：4 项标准 Java 对照、1 项真实 Logback 阶段对照和 1 项不支持文本格式的明确失败检查，均通过普通构建和 ASan/UBSan/泄漏检测；记录见 `ANNOTATION-RESULTS.txt`。
- 正则及配套运行库：8 项检查涵盖真实 OpenJDK 正则、UTF-16、全部 Unicode 码点属性、浮点解析、环境变量、真实 Logback Duration 和明确不支持的路径，均通过普通构建与 ASan/UBSan/泄漏检测；见 `REGEX-RESULTS.txt`。
- 输出流检查：5 项 Java 8 对照、1 项真实 Logback 控制台包装类对照和 1 项未实现文件构造器检查，均通过普通构建与 ASan/UBSan/泄漏检测。比较实际 stdout/stderr 字节；并发检查仅过滤 ASan 关于 ucontext 的那条固定提示，仍检查所有内存错误，见 `OUTPUT-SUPPORT.md`。
- 目标构建：ARM ELF 链接成功、`genzehn` 生成 `.tns` 并检查其结构。
- 方法反射与包查询：9 项检查包含标准 Java 对照、原始 Logback 属性发现和真实 setter/getter 调用，以及未实现内建方法的明确失败；全部通过普通构建与 ASan/UBSan/泄漏检测，见 `METHOD-SUPPORT.md`。
- 字符集：6 项 Java 8 对照、1 项原始 Logback 正文编码的 Java 17 对照、1 项原始日志头缺少 File 的明确失败检查，均通过普通构建与 ASan/UBSan/泄漏检测；见 `CHARSET-SUPPORT.md`。
- **未完成：计算器或带合法系统镜像的模拟器运行测试。**

`dist/` 中的示例用于第一次实机验证；即使它通过，也不能据此声称完整 Xinbot 已经兼容。

## 补充运行库

固定 OpenJDK 8 提交及逐文件校验记录见 `runtime/openjdk8/SOURCES.json`。
使用 JDK 17 javac 的 `-source 8 -target 8`，并以 Java 8 `rt.jar` 为 bootclasspath。
本次编译接口来自 Temurin 8u504-b01 的 Linux x64 JRE 压缩包，SHA-256：
`52dcd578baca1d3e449ea86768a9129c0ee04d7b22565695498353cc66940c61`。
该 JRE 仅是构建依赖；计算器执行的是本项目解释器和重新编译的补充类库。
输出流测试也用该 JRE 作为 Java 8 行为对照，保留原始 FilterOutputStream 的
双异常关闭顺序；原始 Xinbot/Logback 组件仍使用 Java 17 对照。基础示例现在也
依赖运行库中的输出流父类，部署时需要 `dist/runtime.jar.tns` 和三行配置文件。

内建类的反射声明表也来自上述固定 Java 8 `rt.jar`，包含名称、描述符、访问标志和
声明异常，不含可执行字节码。用 `python3 tools/generate-builtin-methods.py --rt-jar /path/to/java8/lib/rt.jar`
复现；生成器校验输入和输出哈希，授权及来源见 `vendor/openjdk8-api/`。
这些声明不代表所有内建方法均可执行；缺失实现仍明确报错。

`vendor/openjdk8-nio/` 保留同一固定 OpenJDK 8 版本的生成工具、模板、别名数据与
55 个生成源码。`tools/generate-nio.py --java /path/to/java8/bin/java` 在 Linux/WSL
调用原始 Spp、make 规则和异常类生成脚本，校验输入及输出哈希；本次复现通过。
运行库构建会校验这些文件并嵌入其授权和清单。字符集与缓冲区按 Java 8 行为对照，
包含它与 Java 17 在部分 UTF-16 非法输入上的差异；不据此声称 Java 17 完整兼容。

当前 Ndless SDK 的 `_gettimeofday` 实现仅读取 RTC 秒数，微秒部分恒为零。
计算器端定时等待和 `nanoTime` 的精度、单调性仍需要更换计时后端并做实机验证；
主机定时测试使用 CLOCK_MONOTONIC，不能证明计算器定时行为。

## Expat XML 解析后端

使用官方 Expat 2.8.4 发行源码，原文件位于 `vendor/expat/`，版本与哈希见其 `SOURCES.json`。
应用使用 `XML_UNICODE` 接收 UTF-16 SAX 数据，并启用命名空间和内部实体解析。
外部实体在 Java 适配层保持关闭；解析器不读取它们指向的文件或网络资源。
主机另编译 Expat 的 `/dev/urandom` 模块，Ndless 使用上游低熵后备实现。
本机 XML 分配经过独立的每 VM 8 MiB 上限检查；完整限制见 `XML-SUPPORT.md`。

## Unicode 数据

大小写映射、组合属性、ROOT 词边界表及字符属性表由开源 Temurin 17.0.20.1+1 生成，
发行包与生成文件的哈希记录在 `vendor/openjdk17-casing/SOURCES.json`。
普通构建使用已检入的数据，不需要在计算器上部署该 JRE。
数据与适配的边界算法保留上游授权；范围和复现命令见 `CASE-SUPPORT.md`。
字符属性表的复现命令见 `REGEX-SUPPORT.md`；生成器验证固定 JRE 版本和输出 SHA-256。

浮点字符串先按 Java 语法检查，再由 libc 转换。Linux 主机的边界值与随机十进制
输入通过标准 Java 位模式对照；Ndless 使用 newlib，其实际舍入结果仍需实机验证。
