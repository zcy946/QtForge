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
    
    app.exec();
    return 0;
}