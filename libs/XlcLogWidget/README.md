# XlcLogWidget

基于 **Qt Widgets** 的日志查看控件：用 `QListView` + 源模型 + **过滤代理**展示文本，通过向 `spdlog::default_logger` **追加 sink** 实时接收日志。支持关键字搜索、按 spdlog 级别筛选、行内级别关键字着色与按级别行背景、多选后 **Ctrl+C** 复制多行。对外 API 在 **`XlcLogWidget.h`** 中以 **PIMPL** 形式暴露；实现细节在 **`XlcLogWidget.cpp`**。以**静态库**形式提供。

---

## 目录结构

```
libs/XlcLogWidget/
├── CMakeLists.txt          # 构建 XlcLogWidget STATIC 库
├── README.md               # 本说明
├── include/
│   └── XlcLogWidget.h      # 对外 API、信号槽声明（PIMPL）
└── src/
    └── XlcLogWidget.cpp    # 私用类、委托绘制、代理过滤、spdlog sink
```

---

## 依赖

| 依赖 | 说明 |
|------|------|
| **CMake** ≥ 3.16 | |
| **C++17** | |
| **spdlog** | `find_package(spdlog CONFIG REQUIRED)` |
| **Qt5** | **Core**、**Widgets**（`QListView`、`QStyledItemDelegate` 等） |

---

## 作为子工程接入 CMake

在已能 `find_package(spdlog)`、`find_package(Qt5 COMPONENTS Core Widgets)` 的根 `CMakeLists.txt` 中：

```cmake
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/XlcLogWidget")

add_executable(my_app
    main.cpp
    # ...
)
target_link_libraries(my_app PRIVATE
    XlcLogWidget
    Qt5::Widgets
    # 若主工程已链 Qt5::Widgets，可不再重复；XlcLogWidget 已 PUBLIC 传递 Qt5::Core / Qt5::Widgets / spdlog
)
```

链接 `XlcLogWidget` 后：

- 自动获得 **`#include "XlcLogWidget.h"`** 所需头文件搜索路径。
- 自动继承 **PUBLIC** 依赖：`Qt5::Core`、`Qt5::Widgets`、`spdlog::spdlog`。
- 目标需启用 **AUTOMOC**（主工程示例中通常已对可执行文件开启），以便 `Q_OBJECT` 生成 moc。

---

## 与 XlcLogger 的配合顺序

1. 应用入口调用 **`XlcLogger::init()`**（或带选项的 `init(opts)`），确保存在 **`spdlog::default_logger`**。
2. 创建 **`XlcLogWidget`** 实例并 **`attachToDefaultLogger()`**；可按需传入 spdlog **pattern** 字符串（与 `sink::set_pattern` 一致）。
3. 退出前建议 **`spdlog::shutdown()`**（例如在 `QCoreApplication::aboutToQuit`），与 XlcLogger 文档一致。

---

## 使用步骤概览

1. 将控件加入界面布局（如主窗口中央 `QVBoxLayout`）。
2. 在 logger 就绪后调用 **`attachToDefaultLogger()`**。
3. 需要时调用 **`setMaxLineCount`**、**`setLevelBackgroundColor` / `setLevelKeywordColor`**、**`resetLevelColors`**。
4. 可连接 **`sig_lineAppended`**、**`sig_filterRuleChanged`** 做侧车逻辑。
5. 析构 **`~XlcLogWidget`** 会自动 **`detachFromDefaultLogger()`**。

---

## 示例一：最小用法（附加到 default_logger）

```cpp
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <spdlog/spdlog.h>
#include "XlcLogger.hpp"
#include "XlcLogWidget.h"

void setupLogUi(QWidget *central)
{
    (void)XlcLogger::init();

    auto *logWidget = new XlcLogWidget(central);
    auto *lay = new QVBoxLayout(central);
    lay->addWidget(logWidget);

    logWidget->attachToDefaultLogger();  // 使用默认 pattern，见头文件声明
    spdlog::info("hello from spdlog");
}
```

---

## 示例二：配色与最大行数

```cpp
#include <QColor>
#include <spdlog/common.h>
#include "XlcLogWidget.h"

void tuneWidget(XlcLogWidget *w)
{
    w->setMaxLineCount(10000);

    using namespace spdlog::level;
    w->setLevelKeywordColor(static_cast<int>(warn), QColor(255, 152, 0));
    w->setLevelBackgroundColor(static_cast<int>(critical), QColor(255, 200, 200));
    // 某级别恢复为「跟随主题」行背景：
    w->setLevelBackgroundColor(static_cast<int>(info), QColor());

    w->resetLevelColors();  // 恢复内置默认配色策略
}
```

---

## 示例三：信号与槽命名（本模块约定）

- **信号**：`Q_SIGNALS:` 中声明，名称以 **`sig_`** 开头，例如 **`sig_lineAppended`**、**`sig_filterRuleChanged`**；触发处使用 **`Q_EMIT`**。
- **槽**：`public Q_SLOTS:` / `private Q_SLOTS:` 中声明；对外追加一行使用 **`slot_appendLine(const QString &)`**（由程序或测试直接 `QMetaObject::invokeMethod` 调用时注意槽名）。
- 与 **spdlog sink** 队列投递对应的私有槽为 **`slot_appendLineWithLevel`**（内部使用，字符串 **`"slot_appendLineWithLevel"`** 与 `QMetaObject::invokeMethod` 一致）。

```cpp
connect(logWidget, &XlcLogWidget::sig_lineAppended, [](const QString &line, int level) {
    Q_UNUSED(line);
    Q_UNUSED(level);
    // 例如：状态栏计数 +1
});
```

---

## 功能摘要（界面行为）

| 能力 | 说明 |
|------|------|
| **搜索** | 顶行编辑框：子串匹配（不区分大小写），由代理模型过滤。 |
| **级别模式** | 「显示全部 / 不低于所选级别 / 仅显示所选级别」+ 级别下拉，与 `spdlog` 级别序一致。 |
| **着色** | 委托绘制：级别关键字着色；可按级别设置行背景（无效色表示使用主题交替行）。 |
| **复制** | 列表获得焦点时 **Ctrl+C**：复制当前多选行的完整文本（换行拼接）。 |

---

## Windows 说明

- 本库 **`CMakeLists.txt`** 在 **MSVC** 下为源文件启用 **`/utf-8`**，与含中文的 UI 字符串、日志 UTF-8 一致；主工程也建议使用 UTF-8 源码编码。
- 静态库 **AUTOMOC**：请保证链接目标对 **`include/XlcLogWidget.h`** 参与 moc（本库已将该头文件列入库目标源）。

---

## 命名约定（本模块源码）

- **类名**：大驼峰，如 **`XlcLogWidget`**、**`XlcLogWidgetPrivate`**（实现细节，仅在 `.cpp`）。
- **对外成员函数**：小驼峰，如 **`attachToDefaultLogger`**、**`setMaxLineCount`**。
- **信号**：前缀 **`sig_`**，如 **`sig_lineAppended`**。
- **槽**：前缀 **`slot_`**，并与控件职责组合命名（如 **`slot_searchEdit_textChanged`**）。

---

## 版本与许可

- 库版本见 `libs/XlcLogWidget/CMakeLists.txt` 中 `project(... VERSION)`。
- 本库随 QtForge 采用 MIT 许可证，详见仓库根目录 `LICENSE`。
- 第三方依赖 `spdlog`、`fmt`、Qt 遵循各自的许可证。
