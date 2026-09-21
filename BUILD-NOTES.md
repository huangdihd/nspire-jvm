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
- 内存检查：当前 45 项基础检查、5 项运行库对照运行和 8 项资源/服务/反射测试均通过 AddressSanitizer、UndefinedBehaviorSanitizer 与泄漏检测；预期失败的测试也检查 sanitizer 输出。
- 目标构建：ARM ELF 链接成功、`genzehn` 生成 `.tns` 并检查其结构。
- **未完成：计算器或带合法系统镜像的模拟器运行测试。**

`dist/` 中的示例用于第一次实机验证；即使它通过，也不能据此声称完整 Xinbot 已经兼容。

## 补充运行库

固定 OpenJDK 8 提交及逐文件校验记录见 `runtime/openjdk8/SOURCES.json`。
使用 JDK 17 javac 的 `-source 8 -target 8`，并以 Java 8 `rt.jar` 为 bootclasspath。
本次编译接口来自 Temurin 8u504-b01 的 Linux x64 JRE 压缩包，SHA-256：
`52dcd578baca1d3e449ea86768a9129c0ee04d7b22565695498353cc66940c61`。
该 JRE 仅是构建依赖；计算器执行的是本项目解释器和重新编译的补充类库。

当前 Ndless SDK 的 `_gettimeofday` 实现仅读取 RTC 秒数，微秒部分恒为零。
计算器端定时等待和 `nanoTime` 的精度、单调性仍需要更换计时后端并做实机验证；
主机定时测试使用 CLOCK_MONOTONIC，不能证明计算器定时行为。
