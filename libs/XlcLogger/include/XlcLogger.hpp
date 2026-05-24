/**
 * @file XlcLogger.hpp
 * @author xialichen (xlc946@qq.com)
 * @brief 基于 spdlog 的日志封装（配置、初始化、宏、可选 Qt 类型 fmt 特化）。
 * @version 0.2
 * @date 2026-04-11
 *
 * @copyright Copyright (c) 2026  夏黎辰
 *
 * @par 级别语义（XlcLoggerOptions）
 * - **loggerLevel**：logger 总闸，高于此级别的消息不会进入各 sink。
 * - **consoleLevel / fileLevel**：各 sink 再收紧；应与 loggerLevel 配合使用。
 *
 * @note Windows Release 若使用 `WIN32_EXECUTABLE` 且无控制台，标准输出 sink 可能不可见，此时主要依赖文件 sink。
 */

#ifndef XLCLOGGER_H
#define XLCLOGGER_H

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <fmt/format.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

/* NOTE vcpkg 安装的 spdlog 用上面 fmt；自行 clone 的 spdlog 可改用 spdlog/fmt/bundled/format.h */

// -----------------------------------------------------------------------------
// Qt：优先使用构建系统注入的模块宏，其次 __has_include 兜底
// -----------------------------------------------------------------------------
#if defined(QT_CORE_LIB) || defined(QT_GUI_LIB) || defined(QT_WIDGETS_LIB)
#define XLC_HAS_QT
#elif defined(__has_include)
#if __has_include(<QString>)
#define XLC_HAS_QT
#endif
#endif

#ifdef XLC_HAS_QT
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLine>
#include <QLineF>
#include <QMargins>
#include <QPoint>
#include <QPointF>
#include <QRegularExpression>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVersionNumber>
#include <QtGlobal>
#endif

// -----------------------------------------------------------------------------

/**
 * @brief 日志初始化与输出的可配置项。
 */
struct XlcLoggerOptions
{
    /// @brief 日志文件路径（含文件名），默认 `logs/log.log`。
    std::string logFilePath{"logs/log.log"};
    /// @brief 单文件最大字节数，超过后轮转。
    std::size_t rotatingMaxBytes{50 * 1024 * 1024};
    /// @brief 保留的轮转文件个数。
    std::size_t rotatingMaxFiles{62};
    /// @brief 控制台 sink 的 spdlog pattern。
    std::string consolePattern{"%^[%Y-%m-%d %H:%M:%S.%e] [%l] [T-%t] [%s:%#] [%!] %v%$"};
    /// @brief 文件 sink 的 spdlog pattern。
    std::string filePattern{"[%Y-%m-%d %H:%M:%S.%e] [%l] [T-%t] [%s:%#] [%!] %v"};
    /// @brief 控制台 sink 级别。
    spdlog::level::level_enum consoleLevel{spdlog::level::trace};
    /// @brief 文件 sink 级别。
    spdlog::level::level_enum fileLevel{spdlog::level::trace};
    /// @brief Logger 总闸；通常为 trace，与 Debug/Release 差异主要由 consoleLevel / fileLevel 体现。
    spdlog::level::level_enum loggerLevel{spdlog::level::trace};
    /// @brief 为 `true` 时使用异步 logger（线程池 + async_logger），减轻调用线程 I/O 阻塞；崩溃时可能丢失队列尾部日志。
    bool useAsyncFile{false};
    /// @brief 异步队列容量（`useAsyncFile` 时有效）。
    std::size_t asyncQueueSize{8192};
    /// @brief 异步线程数（`useAsyncFile` 时有效）。
    std::size_t asyncThreadCount{1};
    /// @brief 周期性刷盘间隔。
    std::chrono::seconds flushEvery{3};
    /// @brief 不低于该级别时立即 flush。
    spdlog::level::level_enum flushOnLevel{spdlog::level::err};
    /// @brief 是否在创建文件前自动创建父目录。
    bool createParentDirs{true};

    /**
     * @brief 按当前构建类型（如 Qt Debug / `_DEBUG` / `NDEBUG`）填充默认级别。
     * @return 填充后的选项（Debug 多为 trace，Release 多为 warn）。
     */
    static XlcLoggerOptions defaultForCurrentBuild();
};

/**
 * @brief 日志入口：初始化、级别、按模块名获取 logger，以及供宏调用的分级输出。
 */
class XlcLogger
{
public:
    /**
     * @brief 使用 `defaultForCurrentBuild()` 作为配置并初始化。
     * @return `true` 表示文件 sink 已成功启用；`false` 表示仅控制台（文件路径不可用或创建失败等）。
     * @note 仅首次调用生效，与 `init(const XlcLoggerOptions&)` 共享一次初始化。
     */
    static bool init();

    /**
     * @brief 使用给定选项初始化 spdlog 默认 logger（控制台 + 可选文件，可选异步）。
     * @param[in] opts 输出路径、级别、pattern、flush 策略等。
     * @return `true` 表示文件 sink 已成功启用；`false` 表示仅控制台。
     * @note 仅首次调用生效；重复调用被忽略，返回值仍为首次初始化时的结果。
     */
    static bool init(const XlcLoggerOptions &opts);

    /**
     * @brief 关闭并刷新 spdlog，确保操作发生在 XlcLogger DLL 内部。
     */
    static void shutdown();

    /**
     * @brief 当前是否已成功注册文件 sink（与 `init` 返回值一致）。
     * @return `true` 文件可用；`false` 仅控制台。
     */
    static bool fileSinkEnabled();

    /**
     * @brief 将默认 logger 及其所有 sink 设为同一级别。
     * @param[in] lvl spdlog 级别枚举。
     */
    static void setLevel(spdlog::level::level_enum lvl);

    /**
     * @brief 仅调整控制台 sink 级别（若已初始化且该 sink 仍存在）。
     * @param[in] lvl spdlog 级别枚举。
     */
    static void setConsoleLevel(spdlog::level::level_enum lvl);

    /**
     * @brief 仅调整文件 sink 级别（若已初始化且文件 sink 已成功创建）。
     * @param[in] lvl spdlog 级别枚举。
     */
    static void setFileLevel(spdlog::level::level_enum lvl);

    /**
     * @brief 按名称获取或创建模块 logger（克隆默认 logger 的 sink 列表）。
     * @param[in] name 模块名，对应 spdlog 注册名。
     * @param[in] lev  新创建时的 logger 级别；若已存在同名 logger 则**不修改**其级别。
     * @return 已注册的 logger；若尚无默认 logger 则返回 `nullptr`。
     */
    static std::shared_ptr<spdlog::logger> moduleLogger(std::string_view name,
                                                        spdlog::level::level_enum lev = spdlog::level::info);

    /**
     * @brief 占位透传，便于对自定义类型扩展 `convert` 特化。
     * @tparam T 值类型。
     * @param[in] value 待写入日志的值。
     * @return 原样返回 `value`。
     */
    template <typename T>
    static auto convert(const T &value) -> decltype(value)
    {
        return value;
    }

    /**
     * @brief 带源位置的通用分级输出（内部由各级别函数与宏转发）。
     * @tparam Args 格式化参数类型。
     * @param[in] lvl 日志级别。
     * @param[in] loc 源位置（文件、行、函数）。
     * @param[in] fmt `fmt` 风格格式串。
     * @param[in] args 格式参数。
     */
    template <typename... Args>
    static void log(spdlog::level::level_enum lvl, spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        logText(lvl, loc, fmt::vformat(fmt, fmt::make_format_args(convert(std::forward<Args>(args))...)));
    }

    /**
     * @brief 输出已格式化好的日志文本；实现在 DLL 内，避免 header-only spdlog 在调用方生成独立 registry。
     */
    static void logText(spdlog::level::level_enum lvl, spdlog::source_loc loc, std::string_view message);

    /**
     * @brief Trace 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void trace(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::trace, loc, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Debug 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void debug(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::debug, loc, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Info 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void info(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::info, loc, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Warn 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void warn(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::warn, loc, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Error 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void error(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::err, loc, fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Critical 级别，带源位置。
     * @tparam Args 格式化参数类型。
     */
    template <typename... Args>
    static void critical(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        log(spdlog::level::critical, loc, fmt, std::forward<Args>(args)...);
    }
};

// -----------------------------------------------------------------------------
/** @name 日志宏 `LOG_*`
 *  @brief 带源文件、行号、函数名的快捷调用；内部转发到 `XlcLogger::trace` 等。
 *  @note 与 `SPDLOG_ACTIVE_LEVEL`（可由 CMake 定义）一致时，低于该级别的宏在编译期展开为 `(void)0`，不计算参数。
 */
/// @{
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE(...) XlcLogger::trace({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_TRACE(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG(...) XlcLogger::debug({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO(...) XlcLogger::info({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARN(...) XlcLogger::warn({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR(...) XlcLogger::error({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL(...) XlcLogger::critical({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#else
#define LOG_CRITICAL(...) (void)0
#endif
/// @}

#ifdef XLC_LOGGER_USE_PREFIXED_MACROS
/** @name 可选别名宏 `XLC_LOG_*`（需定义 `XLC_LOGGER_USE_PREFIXED_MACROS`） */
/// @{
#define XLC_LOG_TRACE LOG_TRACE
#define XLC_LOG_DEBUG LOG_DEBUG
#define XLC_LOG_INFO LOG_INFO
#define XLC_LOG_WARN LOG_WARN
#define XLC_LOG_ERROR LOG_ERROR
#define XLC_LOG_CRITICAL LOG_CRITICAL
/// @}
#endif

// -----------------------------------------------------------------------------
/** @name 模块日志宏 `LOG_M_*`
 *  @brief 第一个参数为模块 `std::shared_ptr<spdlog::logger>`，其余为格式串与参数。
 *  @note 编译期裁剪规则与 `LOG_*` 相同。
 */
/// @{
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_M_TRACE(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::trace, __VA_ARGS__)
#else
#define LOG_M_TRACE(lg, ...) (void)0
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_M_DEBUG(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#else
#define LOG_M_DEBUG(lg, ...) (void)0
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_M_INFO(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#else
#define LOG_M_INFO(lg, ...) (void)0
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_M_WARN(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#else
#define LOG_M_WARN(lg, ...) (void)0
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_M_ERROR(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::err, __VA_ARGS__)
#else
#define LOG_M_ERROR(lg, ...) (void)0
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_M_CRITICAL(lg, ...) (lg)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, spdlog::level::critical, __VA_ARGS__)
#else
#define LOG_M_CRITICAL(lg, ...) (void)0
#endif
/// @}

namespace xlc
{
/**
 * @brief 非宏 API：自动填充源位置，语义与 `XlcLogger` 静态方法一致。
 * @note 未按 `SPDLOG_ACTIVE_LEVEL` 逐函数裁剪；需要编译期裁剪时请优先使用 `LOG_*` 宏。
 */
inline namespace log
{
/** @brief Trace：等价于 `XlcLogger::trace` + 当前位置。 */
template <typename... Args>
void trace(const char *fmt, Args &&...args)
{
    XlcLogger::trace({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
/** @brief Debug：等价于 `XlcLogger::debug` + 当前位置。 */
template <typename... Args>
void debug(const char *fmt, Args &&...args)
{
    XlcLogger::debug({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
/** @brief Info：等价于 `XlcLogger::info` + 当前位置。 */
template <typename... Args>
void info(const char *fmt, Args &&...args)
{
    XlcLogger::info({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
/** @brief Warn：等价于 `XlcLogger::warn` + 当前位置。 */
template <typename... Args>
void warn(const char *fmt, Args &&...args)
{
    XlcLogger::warn({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
/** @brief Error：等价于 `XlcLogger::error` + 当前位置。 */
template <typename... Args>
void error(const char *fmt, Args &&...args)
{
    XlcLogger::error({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
/** @brief Critical：等价于 `XlcLogger::critical` + 当前位置。 */
template <typename... Args>
void critical(const char *fmt, Args &&...args)
{
    XlcLogger::critical({__FILE__, __LINE__, SPDLOG_FUNCTION}, fmt, std::forward<Args>(args)...);
}
} // namespace log
} // namespace xlc

// -----------------------------------------------------------------------------
/** @name Qt 类型的 `fmt::formatter` 特化（需 `XLC_HAS_QT`） */
/// @{
#ifdef XLC_HAS_QT
namespace xlc::logger_detail
{
inline std::string toStdString(const QString &value)
{
    const QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<size_t>(bytes.size()));
}

inline std::string toStdString(const QByteArray &value)
{
    return std::string(value.constData(), static_cast<size_t>(value.size()));
}

inline std::string toCompactJson(const QJsonDocument &doc)
{
    return toStdString(doc.toJson(QJsonDocument::Compact));
}

inline std::string toCompactJson(const QJsonValue &value)
{
    if (value.isUndefined())
    {
        return "undefined";
    }

    if (value.isObject())
    {
        return toCompactJson(QJsonDocument(value.toObject()));
    }

    if (value.isArray())
    {
        return toCompactJson(QJsonDocument(value.toArray()));
    }

    const std::string wrapped = toCompactJson(QJsonDocument(QJsonArray{value}));
    return wrapped.size() >= 2 ? wrapped.substr(1, wrapped.size() - 2) : wrapped;
}

inline std::string toCompactJson(const QVariant &value)
{
    if (!value.isValid())
    {
        return "invalid";
    }

    const QJsonValue jsonValue = QJsonValue::fromVariant(value);
    if (!jsonValue.isUndefined())
    {
        return toCompactJson(jsonValue);
    }

    const QString text = value.toString();
    if (!text.isEmpty())
    {
        return toStdString(text);
    }

    return value.typeName() ? value.typeName() : "unknown";
}
} // namespace xlc::logger_detail

/**
 * @brief `QString`：按 UTF-8 输出。
 * @note 极高频路径可在外部先 `toUtf8()` 再传 `string_view`/`std::string`，避免重复分配。
 */
template <>
struct fmt::formatter<QString>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QString &q, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(q));
    }
};

/** @brief `QByteArray`：按原始字节输出，常用于 UTF-8 文本缓冲。 */
template <>
struct fmt::formatter<QByteArray>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QByteArray &bytes, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(bytes));
    }
};

/** @brief `QStringList`：格式为 `["a", "b"]`。 */
template <>
struct fmt::formatter<QStringList>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QStringList &list, FormatContext &ctx) const -> decltype(ctx.out())
    {
        QStringList quoted;
        quoted.reserve(list.size());
        for (const QString &item : list)
        {
            quoted.append(QStringLiteral("\"") + item + QStringLiteral("\""));
        }
        return fmt::format_to(ctx.out(), "[{}]", xlc::logger_detail::toStdString(quoted.join(QStringLiteral(", "))));
    }
};

/** @brief `QUrl`：默认 `QUrl::PrettyDecoded` 字符串形式（UTF-8）。 */
template <>
struct fmt::formatter<QUrl>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QUrl &u, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(u.toString(QUrl::PrettyDecoded)));
    }
};

/** @brief `QDate`：使用 `Qt::ISODate`。 */
template <>
struct fmt::formatter<QDate>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QDate &date, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(date.toString(Qt::ISODate)));
    }
};

/** @brief `QTime`：Qt 5.8+ 使用 `ISODateWithMs`，否则 `ISODate`。 */
template <>
struct fmt::formatter<QTime>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QTime &time, FormatContext &ctx) const -> decltype(ctx.out())
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
        const QString isoString = time.toString(Qt::ISODateWithMs);
#else
        const QString isoString = time.toString(Qt::ISODate);
#endif
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(isoString));
    }
};

/** @brief `QDateTime`：Qt 5.8+ 使用 `ISODateWithMs`，否则 `ISODate`。 */
template <>
struct fmt::formatter<QDateTime>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QDateTime &dt, FormatContext &ctx) const -> decltype(ctx.out())
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
        const QString isoString = dt.toString(Qt::ISODateWithMs);
#else
        const QString isoString = dt.toString(Qt::ISODate);
#endif
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(isoString));
    }
};

/** @brief `QJsonDocument`：紧凑 JSON。 */
template <>
struct fmt::formatter<QJsonDocument>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QJsonDocument &doc, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(doc));
    }
};

/** @brief `QJsonObject`：紧凑 JSON 对象。 */
template <>
struct fmt::formatter<QJsonObject>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QJsonObject &object, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(QJsonDocument(object)));
    }
};

/** @brief `QJsonArray`：紧凑 JSON 数组。 */
template <>
struct fmt::formatter<QJsonArray>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QJsonArray &array, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(QJsonDocument(array)));
    }
};

/** @brief `QJsonValue`：紧凑 JSON 值。 */
template <>
struct fmt::formatter<QJsonValue>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QJsonValue &value, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(value));
    }
};

/** @brief `QVariant`：优先按 JSON 语义输出，无法转换时回退到 `toString()`。 */
template <>
struct fmt::formatter<QVariant>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QVariant &value, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(value));
    }
};

/** @brief `QVariantList`：紧凑 JSON 数组。 */
template <>
struct fmt::formatter<QVariantList>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QVariantList &list, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(QJsonDocument::fromVariant(list)));
    }
};

/** @brief `QVariantMap`：紧凑 JSON 对象。 */
template <>
struct fmt::formatter<QVariantMap>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QVariantMap &map, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toCompactJson(QJsonDocument::fromVariant(map)));
    }
};

/** @brief `QUuid`：无大括号字符串形式。 */
template <>
struct fmt::formatter<QUuid>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QUuid &uuid, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(uuid.toString(QUuid::WithoutBraces)));
    }
};

/** @brief `QVersionNumber`：标准点分版本号。 */
template <>
struct fmt::formatter<QVersionNumber>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QVersionNumber &version, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(version.toString()));
    }
};

/** @brief `QRegularExpression`：输出 pattern。 */
template <>
struct fmt::formatter<QRegularExpression>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QRegularExpression &re, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", xlc::logger_detail::toStdString(re.pattern()));
    }
};

/** @brief `QSize`：格式为 `wxh`。 */
template <>
struct fmt::formatter<QSize>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QSize &s, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}x{}", s.width(), s.height());
    }
};

/** @brief `QSizeF`：格式为 `wxh`。 */
template <>
struct fmt::formatter<QSizeF>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QSizeF &s, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}x{}", s.width(), s.height());
    }
};

/** @brief `QPoint`：格式为 `x,y`。 */
template <>
struct fmt::formatter<QPoint>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QPoint &p, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{}", p.x(), p.y());
    }
};

/** @brief `QPointF`：格式为 `x,y`。 */
template <>
struct fmt::formatter<QPointF>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QPointF &p, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{}", p.x(), p.y());
    }
};

/** @brief `QRect`：格式为 `x,y wxh`。 */
template <>
struct fmt::formatter<QRect>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QRect &r, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{} {}x{}", r.x(), r.y(), r.width(), r.height());
    }
};

/** @brief `QRectF`：格式为 `x,y wxh`。 */
template <>
struct fmt::formatter<QRectF>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QRectF &r, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{} {}x{}", r.x(), r.y(), r.width(), r.height());
    }
};

/** @brief `QLine`：格式为 `x1,y1 -> x2,y2`。 */
template <>
struct fmt::formatter<QLine>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QLine &line, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{} -> {},{}", line.x1(), line.y1(), line.x2(), line.y2());
    }
};

/** @brief `QLineF`：格式为 `x1,y1 -> x2,y2`。 */
template <>
struct fmt::formatter<QLineF>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QLineF &line, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{} -> {},{}", line.x1(), line.y1(), line.x2(), line.y2());
    }
};

/** @brief `QMargins`：格式为 `left,top,right,bottom`。 */
template <>
struct fmt::formatter<QMargins>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QMargins &margins, FormatContext &ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{},{},{},{}",
                              margins.left(),
                              margins.top(),
                              margins.right(),
                              margins.bottom());
    }
};
#endif
/// @}

#endif // XLCLOGGER_H
