#include "MainWindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(EyAbstractWidget *parent)
    : EyAbstractWidget(parent)
{
    setupUi();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initItems()
{
    m_labelMessage = new QLabel("以CMake构建的Qt项目示例模板", this);
}

void MainWindow::initLayout()
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(m_labelMessage);
}

void MainWindow::initConnections()
{
}

void MainWindow::initWidget()
{
}