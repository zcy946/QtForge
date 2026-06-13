# XlcLogger

基于 [spdlog](https://github.com/gabime/spdlog) 封装的 C++17 日志库：控制台 + 可选轮转文件、可选异步输出、带源位置的 `LOG_*` 宏、可选 Qt 类型（`QString`、`QUrl` 等）的 `fmt` 格式化支持。以**动态库**形式提供，核心库可供 Qt / 非 Qt 应用链接使用；Qt 类型 formatter 通过独立扩展目标显式启用。

---

## 目录结构

```
libs/XlcLogger/
├── CMakeLists.txt      # 构建 XlcLogger SHARED 库
├── README.md           # 本说明
├── include/
│   └── XlcLogger.hpp   # 对外 API、宏、fmt 特化
└── src/
    └── XlcLogger.cpp   # 初始化与各 sink 装配
```

---

## 依赖

| 依赖 | 说明 |
|------|------|
| **CMake** ≥ 3.16 | |
| **C++17** | `std::filesystem`、模板等 |
| **spdlog** | `find_package(spdlog CONFIG REQUIRED)`，通常随 fmt |
| **Qt5 Core** | 仅使用 `XlcLoggerQtFormatter` 时需要 |

---

## 作为子工程接入 CMake

在**已**能 `find_package(spdlog)` 的工程根 `CMakeLists.txt` 中：

```cmake
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogger")

add_executable(my_app main.cpp ...)
target_link_libraries(my_app PRIVATE
    XlcLogger
    # 其他库…
)
```

链接 `XlcLogger` 后：

- 自动获得 **头文件搜索路径**（`#include "XlcLogger.hpp"`）。
- 自动继承 **PUBLIC** 依赖：`spdlog::spdlog_header_only`。
- 自动继承 **`SPDLOG_ACTIVE_LEVEL`** 编译定义（与本库 `CMakeLists.txt` 中 Debug/Release 策略一致），用于 `LOG_*` 宏的编译期裁剪。

若需要直接记录 Qt 类型（如 `QString`、`QDateTime`、`QVariant`），额外链接 `XlcLoggerQtFormatter`：

```cmake
find_package(Qt5 COMPONENTS Core REQUIRED)
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogger")

target_link_libraries(my_app PRIVATE
    XlcLogger
    XlcLoggerQtFormatter
)
```

`XlcLoggerQtFormatter` 是 `INTERFACE` 目标，会向调用方传递 `Qt5::Core` 和 `XLCLOGGER_ENABLE_QT_FORMATTER`。如果关闭 `XLCLOGGER_ENABLE_QT_FORMATTER` CMake 选项，或当前环境找不到 `Qt5::Core`，该扩展目标不会创建，核心 `XlcLogger` 仍可单独构建。

---

## 运行时常驻文件与返回值

- 默认日志文件路径：`logs/log.log`（可通过 `XlcLoggerOptions::logFilePath` 修改）。
- 若设置 `createParentDirs == true`（默认），会尝试创建日志文件所在父目录。
- **`XlcLogger::init()` / `init(opts)` 的返回值**：`true` 表示**文件 sink 已成功**；`false` 表示仅控制台（路径无效、创建失败等）。控制台 logger 仍会注册，应用可正常打日志。
- `init()` 与 `init(opts)` 共用一次性初始化逻辑，只有第一次调用会真正创建 sink 和 logger；后续调用返回首次初始化时的文件 sink 状态。
- `useAsyncFile == true` 且文件 sink 创建成功时，会使用 `spdlog::async_logger`；如果文件 sink 不可用，则退回仅控制台 logger。

---

## API 快览

| API | 说明 |
|-----|------|
| `XlcLoggerOptions::defaultForCurrentBuild()` | 根据当前预处理宏填充默认级别：Qt Debug、`_DEBUG` 或未定义 `NDEBUG` 时为 `trace`；否则为 `info`；`loggerLevel` 保持 `trace` 总闸。 |
| `XlcLogger::init()` / `init(opts)` | 初始化默认 logger，注册控制台 sink 和可选轮转文件 sink。 |
| `XlcLogger::shutdown()` | 关闭并刷新 spdlog；正常退出路径中应显式调用一次。 |
| `XlcLogger::fileSinkEnabled()` | 查询当前文件 sink 是否成功启用，与首次 `init` 返回值一致。 |
| `XlcLogger::setLevel(level)` | 同时调整默认 logger 和所有 sink 的运行时级别。 |
| `XlcLogger::setConsoleLevel(level)` | 仅调整控制台 sink 级别。 |
| `XlcLogger::setFileLevel(level)` | 仅调整文件 sink 级别。 |
| `XlcLogger::moduleLogger(name, level)` | 按名称获取或创建模块 logger；新建时克隆默认 logger 的 sink 列表。 |

---

## 使用步骤概览

1. 在程序入口**尽早**调用 `XlcLogger::init()` 或 `init(XlcLoggerOptions{...})`（仅第一次有效）。
2. 在业务代码中使用 `LOG_INFO("value={}", x)` 等宏。
3. 正常退出路径中显式调用一次 `XlcLogger::shutdown()`（例如在 `QCoreApplication::aboutToQuit`），确保缓冲写入；`aboutToQuit` 不是唯一写法，关键是正常退出时有一次明确收尾。

---

## 示例一：最小初始化（默认选项）

```cpp
#include "XlcLogger.hpp"

int main(int argc, char *argv[])
{
    (void)XlcLogger::init();  // 使用 defaultForCurrentBuild()：Debug 多为 trace，Release 多为 info

    LOG_INFO("application start");
    LOG_DEBUG("debug {}", 42);
    return 0;
}
```

---

## 示例二：自定义路径与异步文件

```cpp
#include "XlcLogger.hpp"

void setupLog()
{
    XlcLoggerOptions opt;
    opt.logFilePath = "var/my_app.log";
    opt.rotatingMaxBytes = 10 * 1024 * 1024;
    opt.rotatingMaxFiles = 10;
    opt.useAsyncFile = true;   // 使用 spdlog 线程池 + async_logger，减轻 UI 线程阻塞
    opt.asyncQueueSize = 16384;
    opt.asyncThreadCount = 1;

    if (!XlcLogger::init(opt))
    {
        LOG_WARN("file sink unavailable, console only");
    }
}
```

---

## 示例三：模块 logger 与 `LOG_M_*` 宏

```cpp
#include "XlcLogger.hpp"

void netModule()
{
    auto lg = XlcLogger::moduleLogger("net", spdlog::level::debug);
    if (!lg) return;

    LOG_M_INFO(lg, "connected to {}", "example.com");
}
```

说明：

- `moduleLogger()` 需要在 `XlcLogger::init()` 之后调用；若默认 logger 尚不存在，会返回 `nullptr`。
- 如果同名模块 logger 已存在，`moduleLogger(name, level)` 会直接返回已有 logger，**不会**修改它的级别。

---

## 示例四：非宏 API（`xlc::log`）

```cpp
#include "XlcLogger.hpp"

void f()
{
    xlc::log::info("hello {}", 2026);
}
```

说明：`xlc::log` 内函数**不**按 `SPDLOG_ACTIVE_LEVEL` 做逐函数编译期裁剪；需要裁剪时请用 `LOG_*` 宏。

---

## 示例五：Qt 应用与退出时 flush

Qt 程序可用 `QCoreApplication::aboutToQuit` 触发退出清理；也可以在 `app.exec()` 返回后、`QApplication` 析构前调用。两种方式都可以，关键是正常退出路径中显式调用一次 `XlcLogger::shutdown()`。

```cpp
#include <QCoreApplication>
#include <QApplication>
#include "XlcLogger.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    (void)XlcLogger::init();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        XlcLogger::shutdown();
    });

    // ... UI ...
    return app.exec();
}
```

---

## 日志级别与宏裁剪

- **运行时**：`loggerLevel` 为总闸；`consoleLevel`、`fileLevel` 分别限制控制台与文件。
- **默认运行时级别**：`defaultForCurrentBuild()` 在 Qt Debug、`_DEBUG` 或未定义 `NDEBUG` 时让控制台和文件输出 `trace` 及以上；否则输出 `info` 及以上。
- **运行时调级**：需要临时放宽或收紧输出时，可用 `setLevel()` 同时调整 logger 和 sink，或用 `setConsoleLevel()` / `setFileLevel()` 分别调整。
- **编译期**：`LOG_TRACE`～`LOG_CRITICAL` 受 **`SPDLOG_ACTIVE_LEVEL`** 约束（由本库 CMake 按配置传入：Debug / RelWithDebInfo 为 `0`，Release / MinSizeRel 为 `2`）。低于该级别的宏可展开为 `(void)0`，避免格式参数求值。

若需自定义裁剪策略，可在**链接 XlcLogger 的目标**上覆盖 `SPDLOG_ACTIVE_LEVEL`（注意与 spdlog 级别数值一致）。

---

## Qt formatter 支持类型

链接 `XlcLoggerQtFormatter` 后，可直接将常用 Qt Core 类型作为 `fmt` 参数传给 `LOG_*` / `xlc::log::*`：

| 类型 | 输出形式 |
|------|----------|
| `QString`、`QByteArray` | UTF-8 文本 |
| `QStringList` | `["a", "b"]` |
| `QUrl` | `QUrl::PrettyDecoded` 字符串 |
| `QDate`、`QTime`、`QDateTime` | ISO 日期 / 时间字符串 |
| `QJsonDocument`、`QJsonObject`、`QJsonArray`、`QJsonValue` | 紧凑 JSON |
| `QVariant`、`QVariantList`、`QVariantMap` | 优先按 JSON 语义输出，必要时回退为文本 |
| `QUuid`、`QVersionNumber`、`QRegularExpression` | UUID、版本号、正则 pattern |
| `QSize`、`QSizeF` | `widthxheight` |
| `QPoint`、`QPointF` | `x,y` |
| `QRect`、`QRectF` | `x,y widthxheight` |
| `QLine`、`QLineF` | `x1,y1 -> x2,y2` |
| `QMargins` | `left,top,right,bottom` |

```cpp
LOG_INFO("text={}, url={}, rect={}", QStringLiteral("你好"), QUrl("https://example.com"), QRect(0, 0, 320, 240));
```

---

## Windows 说明

- 生成 **`XlcLogger.dll`**（或 MinGW 下同名动态库）。请保证可执行文件运行时能加载该 DLL（同目录或 `PATH`）。
- **Release 子系统为 GUI** 且无控制台时，标准输出 sink 可能不可见，请依赖文件日志或附加控制台。
- 初始化时会调用 `SetConsoleOutputCP` / `SetConsoleCP`（`CP_UTF8`），使控制台按 UTF-8 显示中文；**源码与工程需使用 UTF-8**（MSVC 建议使用 `/utf-8`，本库 `CMakeLists.txt` 已加）。若日志文件用记事本打开仍乱码，请在编辑器中选择 **UTF-8** 编码查看。

---

## 可选宏

| 宏 | 作用 |
|----|------|
| `XLCLOGGER_ENABLE_QT_FORMATTER` | 启用 Qt 类型的 `fmt::formatter` 特化；通常由 `XlcLoggerQtFormatter` 自动传递 |
| `XLC_LOGGER_USE_PREFIXED_MACROS` | 定义 `XLC_LOG_TRACE` 等与 `LOG_*` 等价别名，减轻命名冲突顾虑 |

---

## 命名约定（本模块源码）

- **类名**：大驼峰（PascalCase），如 `XlcLogger`、`XlcLoggerOptions`。
- **公开成员函数**：小驼峰（camelCase），如 `fileSinkEnabled()`、`setLevel()`、`moduleLogger()`。
- **`XlcLoggerOptions` 字段**：小驼峰，如 `logFilePath`、`useAsyncFile`。
- **宏**：保持 `LOG_*`、`LOG_M_*` 等大写风格，与 spdlog 惯例一致。

---

## 版本与许可

- 库版本见 `libs/XlcLogger/CMakeLists.txt` 中 `project(... VERSION)`。
- 本库随 QtForge 采用 MIT 许可证，详见仓库根目录 `LICENSE`。
- 第三方依赖 `spdlog`、`fmt`、Qt 遵循各自的许可证。
