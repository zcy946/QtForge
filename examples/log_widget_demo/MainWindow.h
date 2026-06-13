/**
 * @file MainWindow.h
 * @author xialichen (xlc946@qq.com)
 * @brief CMake构建的Qt项目示例模板
 * @version 0.1
 * @date 2026-03-06
 *
 * @copyright Copyright (c) 2026  夏黎辰
 *
 */


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

class QPushButton;
class QPlainTextEdit;
class XlcLogWidget;
class MainWindow : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

private:
    void setupUi();
    void initItems();
    void initLayout();
    void initConnections();
    void onPushButtonAddTraceClicked();
    void onPushButtonAddDebugClicked();
    void onPushButtonAddInfoClicked();
    void onPushButtonAddWarnClicked();
    void onPushButtonAddErrorClicked();
    void onPushButtonAddCriticalClicked();

    QPushButton *m_pushButtonAddTrace;
    QPushButton *m_pushButtonAddDebug;
    QPushButton *m_pushButtonAddInfo;
    QPushButton *m_pushButtonAddWarn;
    QPushButton *m_pushButtonAddError;
    QPushButton *m_pushButtonAddCritical;
    QPlainTextEdit *m_plainTextEdit;
    XlcLogWidget *m_xlcLogWidget;
};

#endif // MAINWINDOW_H
