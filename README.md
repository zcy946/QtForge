# QtForge

QtForge 是一个基于 **CMake + Qt5 Widgets + C++17** 的桌面应用脚手架，适合作为新的 Qt Widgets 项目起点。它提供了一个可直接运行的示例程序，并集成了基于 `spdlog` 的日志库和日志查看控件，方便快速搭建带日志能力的 Qt 桌面程序。

## 功能概览

- 使用 CMake 管理 Qt Widgets 应用。
- 自动开启 `AUTOMOC`、`AUTOUIC`、`AUTORCC`。
- 使用 `FindMyQt.cmake` 根据 Qt 版本、编译器和系统位数选择 Qt 安装路径。
- 内置 `XlcLogger` 动态库，提供 `LOG_TRACE`、`LOG_DEBUG`、`LOG_INFO` 等日志宏。
- 内置 `XlcLogWidget` 静态库，可在界面中实时显示 `spdlog` 日志。
- 约定 QWidget 子类使用统一的初始化流程，避免额外抽象基类限制继承层级。
- MSVC 下启用 `/utf-8`，便于中文源码和日志显示。

## 目录结构

```text
.
├── CMakeLists.txt                 # 顶层 CMake 配置，管理公共配置、内置库和示例开关
├── cmake/
│   └── FindMyQt.cmake             # 本地 Qt 路径选择辅助函数
├── examples/
│   └── log_widget_demo/           # 日志控件示例程序
│       ├── CMakeLists.txt
│       ├── main.cpp               # 程序入口与日志格式化示例
│       ├── MainWindow.h           # 示例主窗口
│       └── MainWindow.cpp         # 示例界面与日志按钮
├── libs/
│   ├── XlcLogger/                 # spdlog 封装库，动态库
│   └── XlcLogWidget/              # Qt 日志查看控件，静态库
├── CODING_GUIDELINES.md           # 编码规范
├── LICENSE                        # MIT 许可证
└── README.md
```

## 环境要求

| 依赖 | 说明 |
|------|------|
| CMake >= 3.16 | 顶层和子库均要求 3.16 或更高版本 |
| C++17 编译器 | MSVC 或 MinGW 均可按需配置 |
| Qt5 | 当前顶层工程调用 `find_my_qt(5)` 并查找 `Qt5::Core`、`Qt5::Widgets`、`Qt5::Gui` |
| spdlog | 通过 `find_package(spdlog CONFIG REQUIRED)` 查找，建议使用 vcpkg、Conan 或本机包管理方式安装 |

## Qt 路径配置

`cmake/FindMyQt.cmake` 会根据目标 Qt 版本、编译器和系统位数拼出变量名，例如：

- `ROOT_QT_5_MSVC_X64`
- `ROOT_QT_5_MSVC_X32`
- `ROOT_QT_5_MINGW_X64`
- `ROOT_QT_6_MSVC_X64`

当前顶层工程使用的是 Qt5：

```cmake
find_my_qt(5)
find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)
```

使用前请按自己的机器环境修改 `cmake/FindMyQt.cmake` 中的默认路径，或在 CMake Configure 时通过命令行 / CMake Preset / VSCode Profile 传入对应变量：

```powershell
cmake -S . -B build -DROOT_QT_5_MSVC_X64="D:/Qt/5.15.2/msvc2019_64"
```

## 构建

示例命令：

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

默认会构建示例程序。如只需要构建内置库，可关闭示例：

```powershell
cmake -S . -B build -DQTFORGE_BUILD_EXAMPLES=OFF
cmake --build build --config Debug
```

如果你使用多配置生成器（例如 Visual Studio），运行产物通常位于：

```text
build/Debug/
build/Release/
```

切换 Qt 路径、编译器、生成器或重要 CMake 配置后，建议删除 `build` 目录后重新配置和编译。

## 示例程序

示例程序位于 `examples/log_widget_demo`，可执行目标名为：

```cmake
QtForgeLogWidgetExample
```

顶层 CMake 项目名为 `QtForge`。使用 QtForge 创建自己的项目时，可以保留 `libs/` 作为基础组件，也可以按需关闭或删除 `examples/`。

程序启动时会：

1. 初始化 `XlcLogger`。
2. 输出多种 Qt 类型的日志格式化示例，包括 `QString`、`QUrl`、`QJsonObject`、`QVariant`、`QRect` 等。
3. 显示一个 800x600 的 Qt Widgets 窗口。
4. 在窗口中提供 Trace、Debug、Info、Warn、Error、Critical 按钮，可将输入框内容写入不同级别的日志。
5. 通过 `XlcLogWidget` 在界面内实时展示日志。
6. 在应用正常退出路径中显式调用一次 `XlcLogger::shutdown()`（例如通过 `QCoreApplication::aboutToQuit`），确保日志缓冲刷新。

默认日志文件由 `XlcLogger` 管理，通常写入：

```text
logs/log.log
```

## 内置库

### XlcLogger

路径：`libs/XlcLogger`

`XlcLogger` 是基于 `spdlog` 的日志封装库，当前以动态库形式构建。它提供：

- 默认 logger 初始化。
- 控制台与可选轮转文件输出。
- 可选异步文件日志。
- 带源位置的 `LOG_*` 宏。
- Qt 常用类型的 `fmt` 格式化支持。

更详细的 API 和示例见 `libs/XlcLogger/README.md`。

### XlcLogWidget

路径：`libs/XlcLogWidget`

`XlcLogWidget` 是基于 Qt Widgets 的日志查看控件，当前以静态库形式构建。它提供：

- 附加到 `spdlog::default_logger` 的 sink。
- 日志实时追加显示。
- 关键字搜索。
- 按日志级别筛选。
- 按级别着色。
- 多选复制日志行。

更详细的 API 和示例见 `libs/XlcLogWidget/README.md`。

## QWidget 初始化约定

QWidget 子类建议在构造函数中显式调用本类私有的 `setupUi()`，并在 `setupUi()` 中保持统一初始化顺序：

```text
initItems() -> initLayout() -> initConnections() -> initWidget()
```

其中：

- `initItems()`：创建子控件、成员对象，并设置不依赖布局的基础属性。
- `initLayout()`：创建布局并将控件加入布局。
- `initConnections()`：集中建立信号与槽连接。
- `initWidget()`：设置窗口属性、默认状态或初始数据；没有需要时可以省略。

该约定只作为编码规范，不要求继承额外基类。不同窗口可按需继承 `QWidget`、`QMainWindow`、`QDialog` 或其他合适的 Qt 类型。

## 作为项目模板使用

复制 QtForge 后，通常需要修改：

- 顶层 `CMakeLists.txt` 中的 `project(...)` 名称和版本。
- `cmake/FindMyQt.cmake` 中的本机 Qt 安装路径。
- `examples/log_widget_demo/main.cpp` 中的启动逻辑和示例日志。
- `examples/log_widget_demo/MainWindow.h`、`examples/log_widget_demo/MainWindow.cpp` 中的示例界面。
- 按需保留、替换或删除 `libs/XlcLogger` 与 `libs/XlcLogWidget`。
- 按需保留、替换、关闭或删除 `examples/`。

建议先确保模板原样能配置、编译、运行，再逐步替换业务代码。

## 注意事项

- 本项目源码建议统一保存为 UTF-8。
- Windows Release 配置会自动隐藏控制台：`WIN32_EXECUTABLE` 仅在 Release 为 true。
- `XlcLogger` 是动态库，运行时需确保生成的 DLL 能被可执行程序找到。
- 如果 `find_package(spdlog CONFIG REQUIRED)` 失败，请先检查包管理器安装情况和 `CMAKE_PREFIX_PATH`。
- 切换配置文件、编译器或 Qt 安装路径后，建议删除 `build` 目录重新配置。

## 许可证

QtForge 采用 MIT 许可证，详见 [LICENSE](LICENSE)。

本项目依赖的 Qt、spdlog 等第三方库遵循各自的许可证。使用或分发基于 QtForge 构建的程序时，请同时遵守这些第三方依赖的许可要求。
