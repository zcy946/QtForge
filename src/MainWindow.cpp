#include "MainWindow.h"
#include <QVBoxLayout>
#include "EyLogger.hpp"

MainWindow::MainWindow(EyAbstractWidget *parent)
    : EyAbstractWidget(parent)
{
    // 初始化UI
    setupUi();

    LOG_DEBUG("Successfully init");
}

MainWindow::~MainWindow() = default;

void MainWindow::initItems()
{
    m_labelMessage = new QLabel("以CMake构建的Qt项目示例模板", this);
}

void MainWindow::initLayout()
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(m_labelMessage, 0, Qt::AlignCenter);
}

void MainWindow::initConnections()
{
}

void MainWindow::initWidget()
{
}