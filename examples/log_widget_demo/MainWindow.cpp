#include "MainWindow.h"
#include "XlcLogWidget.h"
#include "XlcLogger.hpp"
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    // 初始化UI
    setupUi();

    LOG_DEBUG("Successfully init");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    initItems();
    initLayout();
    initConnections();
}

void MainWindow::initItems()
{
    m_pushButtonAddTrace = new QPushButton("Trace", this);
    m_pushButtonAddDebug = new QPushButton("Debug", this);
    m_pushButtonAddInfo = new QPushButton("Info", this);
    m_pushButtonAddWarn = new QPushButton("Warn", this);
    m_pushButtonAddError = new QPushButton("Error", this);
    m_pushButtonAddCritical = new QPushButton("Critical", this);

    m_plainTextEdit = new QPlainTextEdit(this);
    m_plainTextEdit->setPlaceholderText("Input text here");

    m_xlcLogWidget = new XlcLogWidget(this);
    m_xlcLogWidget->attachToDefaultLogger();
}

void MainWindow::initLayout()
{
    QVBoxLayout *vLayoutRoot = new QVBoxLayout(this);
    {
        QHBoxLayout *hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);
        hLayout->addStretch();
        hLayout->addWidget(m_pushButtonAddTrace);
        hLayout->addWidget(m_pushButtonAddDebug);
        hLayout->addWidget(m_pushButtonAddInfo);
        hLayout->addWidget(m_pushButtonAddWarn);
        hLayout->addWidget(m_pushButtonAddError);
        hLayout->addWidget(m_pushButtonAddCritical);
        vLayoutRoot->addLayout(hLayout);
    }
    vLayoutRoot->addWidget(m_plainTextEdit, 1);
    vLayoutRoot->addWidget(m_xlcLogWidget);
}

void MainWindow::initConnections()
{
    connect(m_pushButtonAddTrace,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_TRACE("{}", log);
            });
    connect(m_pushButtonAddDebug,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_DEBUG("{}", log);
            });
    connect(m_pushButtonAddInfo,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_INFO("{}", log);
            });
    connect(m_pushButtonAddWarn,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_WARN("{}", log);
            });
    connect(m_pushButtonAddError,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_ERROR("{}", log);
            });
    connect(m_pushButtonAddCritical,
            &QPushButton::clicked,
            this,
            [this]()
            {
                const QString log = m_plainTextEdit->toPlainText();
                if (log.isEmpty())
                {
                    return;
                }

                LOG_CRITICAL("{}", log);
            });
}
