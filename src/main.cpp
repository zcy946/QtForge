#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
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
    const QByteArray qtByteArray("utf8-byte-array");
    const QStringList qtStringList{QStringLiteral("alpha"), QStringLiteral("中文"), QStringLiteral("omega")};
    const QUrl qtUrl(QStringLiteral("https://example.com/path?q=Qt%E6%97%A5%E5%BF%97"));
    const QDate qtDate = QDate::currentDate();
    const QTime qtTime = QTime::currentTime();
    const QDateTime qtDateTime = QDateTime::currentDateTime();
    const QJsonObject qtJsonObject{{QStringLiteral("name"), QStringLiteral("xlc")},
                                   {QStringLiteral("enabled"), true},
                                   {QStringLiteral("count"), 3}};
    const QJsonArray qtJsonArray{QStringLiteral("first"), 2, true};
    const QJsonValue qtJsonValue(QStringLiteral("json-value"));
    const QJsonDocument qtJsonDocument(qtJsonObject);
    const QVariant qtVariant = QVariantMap{{QStringLiteral("variant"), QStringLiteral("value")}};
    const QVariantList qtVariantList{QStringLiteral("item"), 42, false};
    const QVariantMap qtVariantMap{{QStringLiteral("answer"), 42}, {QStringLiteral("ok"), true}};
    const QUuid qtUuid = QUuid::createUuid();
    const QVersionNumber qtVersion(1, 2, 3);
    const QRegularExpression qtRegularExpression(QStringLiteral(R"(\w+\d+)"));
    const QSize qtSize(640, 480);
    const QSizeF qtSizeF(640.5, 480.25);
    const QPoint qtPoint(12, 34);
    const QPointF qtPointF(12.5, 34.25);
    const QRect qtRect(10, 20, 300, 200);
    const QRectF qtRectF(10.5, 20.25, 300.75, 200.5);
    const QLine qtLine(0, 1, 2, 3);
    const QLineF qtLineF(0.5, 1.25, 2.5, 3.75);
    const QMargins qtMargins(1, 2, 3, 4);
    LOG_INFO("Qt Core text/url/date: QString='{}', QByteArray='{}', QStringList={}, QUrl='{}', QDate={}, QTime={}, QDateTime={}",
             qtString,
             qtByteArray,
             qtStringList,
             qtUrl,
             qtDate,
             qtTime,
             qtDateTime);
    LOG_INFO("Qt Core json/variant: QJsonDocument={}, QJsonObject={}, QJsonArray={}, QJsonValue={}, QVariant={}, QVariantList={}, QVariantMap={}",
             qtJsonDocument,
             qtJsonObject,
             qtJsonArray,
             qtJsonValue,
             qtVariant,
             qtVariantList,
             qtVariantMap);
    LOG_INFO("Qt Core misc/geometry: QUuid={}, QVersionNumber={}, QRegularExpression='{}', QSize={}, QSizeF={}, QPoint={}, QPointF={}, QRect={}, QRectF={}, QLine={}, QLineF={}, QMargins={}",
             qtUuid,
             qtVersion,
             qtRegularExpression,
             qtSize,
             qtSizeF,
             qtPoint,
             qtPointF,
             qtRect,
             qtRectF,
             qtLine,
             qtLineF,
             qtMargins);
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
