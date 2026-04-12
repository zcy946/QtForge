# 项目编码约定与风格指南

本文档总结本仓库在命名、注释、Qt/spdlog 用法与工程结构上的习惯，供日常开发与 **AI 辅助编码（vibe coding）** 时对齐输出风格。若与具体文件冲突，以仓库内**既有实现**为准。

---

## 1. 沟通与输出

- **对用户说明**：默认使用 **简体中文**；技术内容表述清楚、完整句，避免电报式短语堆砌。
- **对代码与标识符**：类名、公开 API、仓库内已有标识符保持 **英文**（与现有代码一致）。
- **引用代码**：说明问题时优先使用编辑器可跳转的代码引用格式（行号 + 路径），避免只口述符号名。

---

## 2. 工程与工具链

| 项 | 约定 |
|----|------|
| **CMake** | ≥ 3.16；根 `CMakeLists.txt` 用分区注释（如「基础配置区」「查找 & 链接库」）。 |
| **C++** | **C++17**（`CMAKE_CXX_STANDARD 17`）。 |
| **Qt** | 当前为 **Qt5**（Core / Widgets / Gui 等按目标需要链接）。 |
| **日志** | **spdlog**（`find_package(spdlog CONFIG REQUIRED)`）。 |
| **字符编码** | MSVC 下源码与工程倾向 **UTF-8**（子库 CMake 中常见 `/utf-8`）；日志与 UI 字符串注意与之一致。 |

---

## 3. 命名约定

### 3.1 通用 C++

- **类 / 结构体**：`PascalCase`，模块相关类型可加前缀 **`Xlc`**（如 `XlcLogger`、`XlcLogWidget`、`XlcAbstractWidget`）。
- **函数 / 方法**：`camelCase`（小驼峰），公开 API 动词开头语义清晰（如 `attachToDefaultLogger`、`setMaxLineCount`）。
- **成员变量**：前缀 **`m_`** + `camelCase`（如 `m_listView`、`m_maxLineCount`）。
- **命名空间 / 文件**：与类名、模块名一致；匿名命名空间可用于翻译单元内部实现。
- **宏**：全大写或项目约定前缀（如日志宏 `LOG_*`）；与 Qt/spdlog 惯例冲突时优先跟随库习惯。

### 3.2 Qt 信号与槽

- **声明分区**：使用 Qt 宏 **`Q_SIGNALS:`**、**`public Q_SLOTS:`**、**`private Q_SLOTS:`**（而非旧式 `signals:` / `slots:` 亦可，但项目内以宏为准时保持一致）。
- **发射信号**：使用 **`Q_EMIT`**。
- **信号名**：前缀 **`sig_`** + 语义，如 `sig_lineAppended`、`sig_filterRuleChanged`。
- **槽名**：前缀 **`slot_`**；若槽响应**特定控件**上的行为，采用 **`slot_<对象语义>_<信号语义>`**，例如 `slot_searchEdit_textChanged`、`slot_modeCombo_currentIndexChanged`。
- **元对象字符串**：`QMetaObject::invokeMethod` 等使用的槽名字符串须与声明**完全一致**（改名时同步全局搜索）。

### 3.3 PIMPL 与友元（**用户明确要求用此模式时再写**）

- 对外头文件仅暴露 **公开 API** + `std::unique_ptr<XxxPrivate> d_ptr`（或等价指针）。
- 实现类 **`XxxPrivate`** 仅在 `.cpp` 中定义；若 Private 内需对**私有槽**取址做 `connect`，在公开类中声明 **`friend class XxxPrivate`**（并前向声明 `XxxPrivate`）。

---

## 4. 注释与文档（Doxygen）

- **头文件 / 公开 API**：使用 Doxygen 块注释，常用标签：
  - **`@brief`**：一句话说明职责或行为。
  - **`@param`** / **`@return`**：参数与返回值；无返回值可省略 `@return`。
  - **`@note`**：调用时机、线程、与别 API 的关系、破坏性注意点等。
- **符号引用**：在注释中用反引号包裹代码符号，如 `` `spdlog::default_logger` ``、`` `Qt::UserRole` ``。
- **文件头**：可保留 `@file`、`@author`、`@brief`、`@date`、`@copyright` 等与现有文件一致。
- **避免**：为显而易见的一行代码写长段注释；无请求的冗长 `README` 或文档文件（**用户明确要求时再写**）。

**长代码内的分段注释（与 Doxygen 互补）：**

- 当单文件或单函数内逻辑块**较长**、需要扫读结构时，可在**实现文件**（如较长的 `.cpp`）中用大写等号线段注释做**视觉分区**，与上文 Doxygen **不冲突**：Doxygen 仍用于声明处；分段注释用于**阅读导航**。
- **大段 / 一级分段**（名称居中、等号加粗边界感）：

```cpp
/* ==================== 初始化 default_logger ==================== */
```

- **子项 / 二级分段**（名称稍短、单线感）：

```cpp
/* ------------------ 装配 file sink ------------------ */
```

- 同一文件内两种线型**不要混用层级**（大段用 `===`，其下子块用 `---`）。

**示例（风格参考）：**

```cpp
/**
 * @brief 使用 `defaultForCurrentBuild()` 作为配置并初始化。
 * @return `true` 表示文件 sink 已成功启用；`false` 表示仅控制台（文件路径不可用或创建失败等）。
 * @note 仅首次调用生效，与 `init(const XlcLoggerOptions&)` 共享一次初始化。
 */
static bool init();
```

---

## 5. UI 与模块结构习惯

### 5.1 `XlcAbstractWidget` 模板方法

- 子类在构造函数**末尾**调用 **`setupUi()`**。
- 顺序：**`initItems()`**（只 new 与属性，不布局）→ **`initLayout()`** → **`initConnections()`** → **`initWidget()`**（窗口级属性与初值）。
- 无内容的步骤可不重写（基类空实现）。

### 5.2 独立控件库（如 `XlcLogWidget`）

- **静态/动态库**的 `CMakeLists.txt`：将含 **`Q_OBJECT`** 的头文件列入 `add_library` 源列表，并 **`AUTOMOC ON`**。
- 子目录 **`lib/<库名>/`** 内可放置 **`README.md`**：结构、依赖、接入方式、示例，风格对齐 **`lib/XlcLogger/README.md`**。

---

## 6. 修改代码的原则（与「顺手重构」的边界）

- **只改任务需要的部分**；不做无关文件的「大扫除」式重构。
- **风格跟随当前文件作者**：命名、缩进、注释密度、是否用 `auto` 等与周边一致。
- **不删除**与本次任务无关的注释或代码（除非用户要求清理）。
- **diff 可读**：每一行改动都应对应有明确目的；避免顺带改格式整文件。

---

## 7. 第三方与平台注意点

- **spdlog**：级别与 `spdlog::level::level_enum` 对齐时，在注释中写清数值含义（如 `0`…`5`）。
- **Qt5**：注意与 Qt6 的 API 差异（例如 `QVariant::toInt` 在 Qt5 无带默认值的便捷重载等），以当前工程实际编译版本为准。
- **Windows**：发布时注意 Qt 插件、运行库路径；日志 DLL/控制台与 UTF-8 说明可参考 `XlcLogger` README。

---

## 8. 文档文件

- **子库说明**：放在对应 **`lib/<模块>/README.md`**。
- **全项目风格（本文档）**：位于**仓库根目录**，便于 AI 与协作者统一查阅。
- 若无必要，**不**主动新增其他 Markdown；避免文档泛滥。

---

## 9. 修订

当引入新模块或团队约定变更时，更新本文档对应章节，并在提交说明中一句话带过即可。
