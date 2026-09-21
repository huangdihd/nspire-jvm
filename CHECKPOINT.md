# 时间和时区开发快照

本分支保存尚未完成的研发成果，基于 `23f3c7043dafc780e7a38b3dc08a1cb01e999d98`。
完整 Xinbot 尚不能启动，也没有计算器实机验证或网络连接验证。

## 此次保存的内容

- 导入原始 OpenJDK 8 的 java.time、日历、Math/StrictMath、BigInteger/BigDecimal 及相关输入接口。
- 打包 TZDB 2026b；两份上游时区加载器改从 JAR 资源读取数据库，解码和时区规则仍使用原实现。
- 加入 Locale 分类默认值、TimeZone/CRC32 本机绑定，以及主机 `--timezone` 参数。
- Ndless 配置计划使用第四行指定时区，未配置时默认 UTC；该路径尚未在设备上验证。
- 保存时间基础测试、日期规则测试及原始 Logback 日期格式器的组件测试代码。

## 实际检查结果

- 补充运行库使用 Java 8 javac 构建成功，当前 `dist/runtime.jar.tns` 为 1,992,713 字节。
- `TimeSupportTest` 与标准 Java 8 对照通过：整数溢出检查、Locale 分类默认值、CRC32 和 DataInputStream 的选定路径。
- `TimeTest` 未通过，实际停在：

```text
VM error: unbound native method: java/lang/StrictMath.log(D)D
  at java/lang/Math.log(D)D pc=1
  at java/math/BigInteger.<clinit>()V pc=300
  at java/time/Duration.<clinit>()V pc=15
```

该失败发生在 `ZoneId.systemDefault()` 加载时区规则的初始化路径上。
后续日期格式化、夏令时测试和 Logback 日期组件测试尚未通过。
StrictMath.sqrt 暂时接到 C 数学库，尚未验证 Java StrictMath 要求的精确一致性。
Locale 服务提供者、部分非 ISO 日历资源及其他运行库依赖仍不完整。

现有 `*-RESULTS.txt`、`XINBOT-RUN.txt`、`TARGET-RESULTS.txt` 和 `BUILD-NOTES.md`
中的通过记录来自此前的检查点，不表示本次时间改动已经通过完整回归、ASan/UBSan 或 ARM 构建。
完整 Xinbot 最近一次运行仍是 `23f3c70` 的记录，停在缺少 `java/time/ZoneId`，尚未进入 `Xinbot.main`。

## 复现当前时间测试

在 Linux/WSL 中准备 C 编译器、Python 3.11+、用于测试编译的 JDK 17 和用于补充库构建/对照的 JDK 8：

```sh
make host
python3 tools/build-runtime.py \
  --java8-home /path/to/jdk8 \
  --javac /path/to/jdk8/bin/javac
python3 tools/test-time.py --java /path/to/jdk8/bin/java
```

最后一条命令目前预期在 `TimeTest` 报出上述错误。
JDK 17 javac 不能直接编译全部保留的原始 Java 8 时间源码，需显式指定 JDK 8 编译器。

## 预构建文件

本分支的 `dist/nspire-jvm.tns` 仍是 `23f3c70` 的产物，运行库已更新。
**不要将这组混合版本文件视为可安装发行包。** `DIST-MANIFEST.json` 仅记录现有文件的大小和哈希。
需要此前配套的实验性文件时，请使用 `23f3c70` 中的整个 `dist/`；该版本同样没有实机验证，也不能启动完整 Xinbot。

本次仅保存开发进度，不发布新的 Release。
