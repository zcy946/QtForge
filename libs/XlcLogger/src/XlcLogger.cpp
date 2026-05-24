/**
 * @file XlcLogger.cpp
 * @brief `XlcLogger` 与 `XlcLoggerOptions` 的实现：目录创建、sink 装配、异步可选、级别与模块 logger。
 */

#include "XlcLogger.hpp"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <mutex>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
#ifdef _WIN32
/// 将控制台输入/输出代码页设为 UTF-8，避免中文等字符在 spdlog 控制台 sink 中乱码。
static void ensureConsoleUtf8()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#endif
std::once_flag initOnceFlag;
std::atomic<bool> fileSinkOk{false};
std::weak_ptr<spdlog::sinks::sink> weakConsoleSink;
std::weak_ptr<spdlog::sinks::sink> weakFileSink;

/**
 * @brief 执行一次性 sink/logger 注册与 flush 策略。
 * @param[in] opts 用户选项。
 * @return `true` 文件 sink 已创建；`false` 仅控制台。
 */
bool doInit(const XlcLoggerOptions &opts)
{
#ifdef _WIN32
    ensureConsoleUtf8();
#endif
    fileSinkOk.store(false, std::memory_order_relaxed);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(opts.consolePattern);
    consoleSink->set_level(opts.consoleLevel);
    weakConsoleSink = consoleSink;

    spdlog::sink_ptr fileSink;
    try
    {
        if (opts.createParentDirs)
        {
            std::filesystem::path path(opts.logFilePath);
            if (path.has_parent_path())
            {
                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec)
                {
                    std::fprintf(stderr, "[XlcLogger] create_directories failed: %s\n", ec.message().c_str());
                }
            }
        }

        auto rotatingSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            opts.logFilePath, opts.rotatingMaxBytes, opts.rotatingMaxFiles);
        rotatingSink->set_pattern(opts.filePattern);
        rotatingSink->set_level(opts.fileLevel);
        fileSink = rotatingSink;
        fileSinkOk.store(true, std::memory_order_relaxed);
    }
    catch (const std::exception &ex)
    {
        std::fprintf(stderr, "[XlcLogger] file sink failed (%s), using console only.\n", ex.what());
        fileSink.reset();
    }
    catch (...)
    {
        std::fprintf(stderr, "[XlcLogger] file sink failed (unknown), using console only.\n");
        fileSink.reset();
    }

    if (fileSink)
    {
        weakFileSink = fileSink;
    }

    std::shared_ptr<spdlog::logger> spdlogLogger;
    if (opts.useAsyncFile && fileSink)
    {
        spdlog::init_thread_pool(opts.asyncQueueSize, opts.asyncThreadCount);
        auto threadPool = spdlog::thread_pool();
        spdlogLogger = std::make_shared<spdlog::async_logger>(
            "multi_sink",
            spdlog::sinks_init_list{consoleSink, fileSink},
            threadPool,
            spdlog::async_overflow_policy::block);
    }
    else
    {
        if (fileSink)
        {
            spdlogLogger = std::make_shared<spdlog::logger>("multi_sink",
                                                            spdlog::sinks_init_list{consoleSink, fileSink});
        }
        else
        {
            spdlogLogger = std::make_shared<spdlog::logger>("multi_sink", consoleSink);
        }
    }

    spdlogLogger->set_level(opts.loggerLevel);
    spdlog::register_logger(spdlogLogger);
    spdlog::set_default_logger(std::move(spdlogLogger));

    spdlog::flush_on(opts.flushOnLevel);
    spdlog::flush_every(opts.flushEvery);

    return fileSinkOk.load(std::memory_order_relaxed);
}
} // namespace

/**
 * @copydoc XlcLoggerOptions::defaultForCurrentBuild
 */
XlcLoggerOptions XlcLoggerOptions::defaultForCurrentBuild()
{
    XlcLoggerOptions options;
#if defined(QT_DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    options.consoleLevel = spdlog::level::trace;
    options.fileLevel = spdlog::level::trace;
#else
    options.consoleLevel = spdlog::level::warn;
    options.fileLevel = spdlog::level::warn;
#endif
    options.loggerLevel = spdlog::level::trace;
    return options;
}

/**
 * @copydoc XlcLogger::init()
 */
bool XlcLogger::init()
{
    return init(XlcLoggerOptions::defaultForCurrentBuild());
}

/**
 * @copydoc XlcLogger::init(const XlcLoggerOptions&)
 */
bool XlcLogger::init(const XlcLoggerOptions &opts)
{
    static bool fileEnabled = false;
    std::call_once(initOnceFlag, [&]() { fileEnabled = doInit(opts); });
    return fileEnabled;
}

/**
 * @copydoc XlcLogger::shutdown
 */
void XlcLogger::shutdown()
{
    spdlog::shutdown();
}

/**
 * @copydoc XlcLogger::logText
 */
void XlcLogger::logText(spdlog::level::level_enum lvl, spdlog::source_loc loc, std::string_view message)
{
    spdlog::log(loc, lvl, "{}", message);
}

/**
 * @copydoc XlcLogger::fileSinkEnabled
 */
bool XlcLogger::fileSinkEnabled()
{
    return fileSinkOk.load(std::memory_order_relaxed);
}

/**
 * @copydoc XlcLogger::setLevel
 */
void XlcLogger::setLevel(spdlog::level::level_enum lvl)
{
    if (auto logger = spdlog::default_logger())
    {
        logger->set_level(lvl);
        for (auto &sink : logger->sinks())
        {
            sink->set_level(lvl);
        }
    }
}

/**
 * @copydoc XlcLogger::setConsoleLevel
 */
void XlcLogger::setConsoleLevel(spdlog::level::level_enum lvl)
{
    if (auto sink = weakConsoleSink.lock())
    {
        sink->set_level(lvl);
    }
}

/**
 * @copydoc XlcLogger::setFileLevel
 */
void XlcLogger::setFileLevel(spdlog::level::level_enum lvl)
{
    if (auto sink = weakFileSink.lock())
    {
        sink->set_level(lvl);
    }
}

/**
 * @copydoc XlcLogger::moduleLogger
 */
std::shared_ptr<spdlog::logger> XlcLogger::moduleLogger(std::string_view name, spdlog::level::level_enum lev)
{
    const std::string nameStr(name);
    if (auto existingLogger = spdlog::get(nameStr))
    {
        return existingLogger;
    }
    auto defaultLogger = spdlog::default_logger();
    if (!defaultLogger)
    {
        return nullptr;
    }
    auto newLogger = defaultLogger->clone(nameStr);
    newLogger->set_level(lev);
    spdlog::register_logger(newLogger);
    return newLogger;
}
