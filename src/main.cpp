#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QRect>
#include <QString>
#include <QUrl>
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

    const QString qtString = QStringLiteral("XlcLogger Qt formatter test");
    const QUrl qtUrl(QStringLiteral("https://example.com/path?q=Qt%E6%97%A5%E5%BF%97"));
    const QDateTime qtDateTime = QDateTime::currentDateTime();
    const QRect qtRect(10, 20, 300, 200);
    LOG_INFO("Qt compatible types: QString='{}', QUrl='{}', QDateTime='{}', QRect='{}'",
             qtString,
             qtUrl,
             qtDateTime,
             qtRect);
    LOG_ERROR("你好{} - {}", "hello", QStringLiteral("你好"));

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
