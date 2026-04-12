/**
 * @file XlcLogWidget.h
 * @author xialichen (xlc946@qq.com)
 * @brief 使用 QListView 展示 spdlog 日志的控件；通过附加 sink 接入 default_logger。
 * @version 0.1
 * @date 2026-04-12
 *
 * @copyright Copyright (c) 2026  夏黎辰
 *
 */

#ifndef XLCLOGWIDGET_H
#define XLCLOGWIDGET_H

#include <QWidget>
#include <memory>
#include <spdlog/sinks/sink.h>
#include <string>

class QListView;
class QStandardItemModel;
class QLineEdit;
class QComboBox;
class XlcLogFilterProxyModel;

/**
 * @brief 内部使用 QListView + 源模型 + 过滤代理显示日志行，可注册为 spdlog 的附加 sink。
 */
class XlcLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XlcLogWidget(QWidget *parent = nullptr);
    ~XlcLogWidget() override;

    /// 在 XlcLogger::init() 之后调用：向 default_logger 追加本控件的 sink。
    void attachToDefaultLogger(const std::string &pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    /// 从 default_logger 移除本控件注册的 sink（析构时自动调用）。
    void detachFromDefaultLogger();

    void setMaxLineCount(int max);
    int maxLineCount() const;

public slots:
    void appendLine(const QString &text);

private slots:
    /// sink 投递：携带 spdlog 级别（与 appendLine 二选一由内部调用）。
    void appendLineWithLevel(const QString &text, int spdlogLevel);
    void onFilterInputsChanged();

private:
    void trimIfNeeded();

    QListView *m_listView{nullptr};
    QStandardItemModel *m_model{nullptr};
    XlcLogFilterProxyModel *m_proxy{nullptr};

    QLineEdit *m_searchEdit{nullptr};
    QComboBox *m_modeCombo{nullptr};
    QComboBox *m_levelCombo{nullptr};

    int m_maxLineCount{5000};

    std::shared_ptr<spdlog::sinks::sink> m_sink;
};

#endif
