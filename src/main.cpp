#include <QApplication>
#include <QCoreApplication>
#include <spdlog/spdlog.h>

#include "MainWindow.h"
#include "XlcLogger.hpp"

int logger = []() -> int
{
    (void)XlcLogger::init();
    return 0;
}();

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     []()
                     {
                         spdlog::shutdown();
                     });

    MainWindow w;
    w.resize(800, 600);
    w.show();

    LOG_TRACE("LOG_TRACE 日志");
    LOG_DEBUG("DELOG_DEBUG 日志");
    LOG_WARN("LOG_WARN 日志");
    LOG_INFO("LOG_INFO 日志");
    LOG_ERROR("LOG_ERROR 日志");
    LOG_CRITICAL("LOG_CRITICAL 日志");

    app.exec();
    return 0;
}