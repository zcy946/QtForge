# XlcLogger

基于 [spdlog](https://github.com/gabime/spdlog) 封装的 C++17 日志库：控制台 + 可选轮转文件、可选异步输出、带源位置的 `LOG_*` 宏、可选 Qt 类型（`QString`、`QUrl` 等）的 `fmt` 格式化支持。以**动态库**形式提供，供 Qt / 非 Qt 应用链接使用（当前实现依赖 **Qt5::Core** 以使用 `QT_DEBUG` 等构建宏；头文件中 Qt 相关部分在检测到 `QT_CORE_LIB` 等或 `QString` 可用时启用）。

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
| **Qt5 Core** | 链接 `Qt5::Core`；头文件内 Qt 类型为可选 |

---

## 作为子工程接入 CMake

在**已**能 `find_package(spdlog)`、`find_package(Qt5 Core)` 的工程根 `CMakeLists.txt` 中：

```cmake
# 建议在 find_package(spdlog) 与 Qt5 之后
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogger")

add_executable(my_app main.cpp ...)
target_link_libraries(my_app PRIVATE
    XlcLogger
    # 其他库…
)
```

链接 `XlcLogger` 后：

- 自动获得 **头文件搜索路径**（`#include "XlcLogger.hpp"`）。
- 自动继承 **PUBLIC** 依赖：`spdlog::spdlog`、`Qt5::Core`。
- 自动继承 **`SPDLOG_ACTIVE_LEVEL`** 编译定义（与本库 `CMakeLists.txt` 中 Debug/Release 策略一致），用于 `LOG_*` 宏的编译期裁剪。

若仅静态链接 spdlog、不希望再暴露 Qt，需自行改本库 CMake（例如将 `Qt5::Core` 改为 `PRIVATE` 并自行处理 `defaultForCurrentBuild()` 中的宏），属于进阶定制。

---

## 运行时常驻文件与返回值

- 默认日志文件路径：`logs/log.log`（可通过 `XlcLoggerOptions::logFilePath` 修改）。
- 若设置 `createParentDirs == true`（默认），会尝试创建日志文件所在父目录。
- **`XlcLogger::init()` / `init(opts)` 的返回值**：`true` 表示**文件 sink 已成功**；`false` 表示仅控制台（路径无效、创建失败等）。控制台 logger 仍会注册，应用可正常打日志。

---

## 使用步骤概览

1. 在程序入口**尽早**调用 `XlcLogger::init()` 或 `init(XlcLoggerOptions{...})`（仅第一次有效）。
2. 在业务代码中使用 `LOG_INFO("value={}", x)` 等宏。
3. 进程退出前建议调用 `spdlog::shutdown()`（例如在 `QCoreApplication::aboutToQuit`），确保缓冲写入。

---

## 示例一：最小初始化（默认选项）

```cpp
#include "XlcLogger.hpp"

int main(int argc, char *argv[])
{
    (void)XlcLogger::init();  // 使用 defaultForCurrentBuild()：Debug 多为 trace，Release 多为 warn

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

## 示例五（推荐🚩）：Qt 应用与退出时 flush

```cpp
#include <QCoreApplication>
#include <QApplication>
#include <spdlog/spdlog.h>
#include "XlcLogger.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    (void)XlcLogger::init();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        spdlog::shutdown();
    });

    // ... UI ...
    return app.exec();
}
```

---

## 日志级别与宏裁剪

- **运行时**：`loggerLevel` 为总闸；`consoleLevel`、`fileLevel` 分别限制控制台与文件。
- **编译期**：`LOG_TRACE`～`LOG_CRITICAL` 受 **`SPDLOG_ACTIVE_LEVEL`** 约束（由本库 CMake 按配置传入）。低于该级别的宏可展开为 `(void)0`，避免格式参数求值。

若需自定义裁剪策略，可在**链接 XlcLogger 的目标**上覆盖 `SPDLOG_ACTIVE_LEVEL`（注意与 spdlog 级别数值一致）。

---

## Windows 说明

- 生成 **`XlcLogger.dll`**（或 MinGW 下同名动态库）。请保证可执行文件运行时能加载该 DLL（同目录或 `PATH`）。
- **Release 子系统为 GUI** 且无控制台时，标准输出 sink 可能不可见，请依赖文件日志或附加控制台。
- 初始化时会调用 `SetConsoleOutputCP` / `SetConsoleCP`（`CP_UTF8`），使控制台按 UTF-8 显示中文；**源码与工程需使用 UTF-8**（MSVC 建议使用 `/utf-8`，本库 `CMakeLists.txt` 已加）。若日志文件用记事本打开仍乱码，请在编辑器中选择 **UTF-8** 编码查看。

---

## 可选宏

| 宏 | 作用 |
|----|------|
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
