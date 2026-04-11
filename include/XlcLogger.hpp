/**
 * @file XlcLogger.hpp
 * @author xialichen (xlc946@qq.com)
 * @brief 日志模块
 * @version 0.1
 * @date 2026-04-11
 * 
 * @copyright Copyright (c) 2026  夏黎辰
 * 
 */

#ifndef XLCLOGGER_H
#define XLCLOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>
#include <string_view>
#include <fmt/format.h> 
/* NOTE vcpkg安装的spdlog用上面这个，自行clone的用下面这个 */
// #include <spdlog/fmt/bundled/format.h>
#include <mutex>

// -----------------------------------------------------------------------------
// Qt 自动检测逻辑
// -----------------------------------------------------------------------------
// 如果定义了 QT_CORE_LIB (通常由 CMake/qmake 定义) 或者检测到 QString 头文件
#if defined(QT_CORE_LIB)
#define XLC_HAS_QT
#elif defined(__has_include)
#if __has_include(<QString>)
#define XLC_HAS_QT
#endif
#endif

#ifdef XLC_HAS_QT
#include <QString>
#endif
// -----------------------------------------------------------------------------

// 存放日志路径&名称
constexpr const char *LOG_FILE_NAME = "logs/log.log";

#define LOG_TRACE(...) XlcLogger::trace({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_DEBUG(...) XlcLogger::debug({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_INFO(...) XlcLogger::info({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_WARN(...) XlcLogger::warn({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_ERROR(...) XlcLogger::error({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)
#define LOG_CRITICAL(...) XlcLogger::critical({__FILE__, __LINE__, SPDLOG_FUNCTION}, __VA_ARGS__)

// -----------------------------------------------------------------------------
// 仅在有 Qt 环境时启用 QString 的 fmt 特化
// -----------------------------------------------------------------------------
#ifdef XLC_HAS_QT
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
        // 1. 将 QString 转换为 UTF-8 的 QByteArray
        const QByteArray byteArray = q.toUtf8();
        // 2. 创建 string_view 以避免额外的 string 拷贝
        std::string_view view(byteArray.constData(), static_cast<size_t>(byteArray.size()));
        // 3. 格式化
        return fmt::format_to(ctx.out(), "{}", view);
    }
};
#endif

class XlcLogger
{
public:
    static void init()
    {
        static std::once_flag init_flag;
        std::call_once(init_flag,
                       []()
                       {
                           /* -------------------- // 控制台输出 -------------------- */
                           auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                           console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [T-%t] [%s:%#] [%!] %v%$");
                           console_sink->set_level(spdlog::level::trace);

                           /* -------------------- 文件输出（可旋转） -------------------- */
                           auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(LOG_FILE_NAME, 1024 * 1024 * 50, 62);
                           rotating_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [T-%t] [%s:%#] [%!] %v");

            /* -------------------- 设置日志级别 -------------------- */
            // 支持 Qt 调试宏以及标准 C++ 调试宏 _DEBUG 是 MSVC 标准，NDEBUG 是标准 C++ Release 模式定义的
#if defined(QT_DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
                           rotating_sink->set_level(spdlog::level::trace);
#else
                           rotating_sink->set_level(spdlog::level::warn);
#endif

                           /* -------------------- 注册全局 Logger -------------------- */
                           auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, rotating_sink});
                           logger->set_level(spdlog::level::trace);
                           spdlog::register_logger(logger);
                           spdlog::set_default_logger(logger);

                           /* -------------------- 设置刷新策略（遇到 Error 时立即刷新磁盘，或每隔 3 秒自动刷新） -------------------- */
                           spdlog::flush_on(spdlog::level::err);
                           spdlog::flush_every(std::chrono::seconds(3));
                       });
    }

    // 通用转换函数：
    // 如果有 Qt，fmt 特化会处理 QString。
    // 如果没有 Qt，代码中也不会出现 QString 类型，此函数透传普通类型。
    template <typename T>
    static auto convert(const T &value) -> decltype(value)
    {
        return value;
    }

    // 自动格式化 + 源位置日志调用封装
    template <typename... Args>
    static void trace(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::trace, fmt, convert(std::forward<Args>(args))...);
    }

    template <typename... Args>
    static void debug(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::debug, fmt, convert(std::forward<Args>(args))...);
    }

    template <typename... Args>
    static void info(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::info, fmt, convert(std::forward<Args>(args))...);
    }

    template <typename... Args>
    static void warn(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::warn, fmt, convert(std::forward<Args>(args))...);
    }

    template <typename... Args>
    static void error(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::err, fmt, convert(std::forward<Args>(args))...);
    }

    template <typename... Args>
    static void critical(spdlog::source_loc loc, const char *fmt, Args &&...args)
    {
        spdlog::log(loc, spdlog::level::critical, fmt, convert(std::forward<Args>(args))...);
    }
};

#endif // XLCLOGGER_H
