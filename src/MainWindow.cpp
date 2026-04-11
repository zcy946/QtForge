#include "MainWindow.h"
#include <QVBoxLayout>
#include "XlcLogger.hpp"
#include <QPainter>
#include <QFontMetrics>

MainWindow::MainWindow(XlcAbstractWidget *parent)
    : XlcAbstractWidget(parent)
{
    // 初始化UI
    setupUi();

    LOG_DEBUG("Successfully init");
}

MainWindow::~MainWindow() = default;

void MainWindow::initItems()
{
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    const QString text("以CMake构建的Qt项目示例模板");

    QPainter painter(this);
    const QFontMetrics fm(painter.font());
    const qreal x = (width() - fm.horizontalAdvance(text)) / 2;
    const qreal y = (height() - fm.height()) / 2;
    painter.drawText(QPointF(x, y), text);
}