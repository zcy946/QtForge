/**
 * @file XlcLogWidget.h
 * @author xialichen (xlc946@qq.com)
 * @brief 使用 QListView 展示 spdlog 日志的控件；通过附加 sink 接入 default_logger。
 * @version 0.1
 * @date 2026-04-12
 *
 * @copyright Copyright (c) 2026  夏黎辰
 */

#ifndef XLCLOGWIDGET_H
#define XLCLOGWIDGET_H

#include <QColor>
#include <QWidget>
#include <memory>
#include <string>

class XlcLogWidgetPrivate;

/**
 * @brief 日志列表控件（PIMPL：实现细节在编译单元内，本头仅公开 API）。
 *
 * @note 级别下标与 `spdlog::level::level_enum` 一致：`0` trace … `5` critical（不含 `off`）。
 */
class XlcLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XlcLogWidget(QWidget *parent = nullptr);
    ~XlcLogWidget() override;

    /**
     * @brief 向 `spdlog::default_logger` 追加本控件使用的 sink。
     * @param pattern 传给 spdlog 的 pattern 字符串，与 `spdlog::sinks::sink::set_pattern` 一致。
     * @note 须在应用侧完成 `XlcLogger::init()` 等初始化后再调用；重复调用无效。
     */
    void attachToDefaultLogger(const std::string &pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    /**
     * @brief 从 `default_logger` 上移除本控件注册的 sink。
     * @note 析构时会自动调用；可多次调用，后几次无操作。
     */
    void detachFromDefaultLogger();

    /**
     * @brief 设置内存中保留的最大日志行数，超出则从头部删除旧行。
     * @param max 最大行数；非正数时按内部默认（如 5000）处理。
     */
    void setMaxLineCount(int max);

    /**
     * @brief 查询当前最大日志行数上限。
     * @return 当前生效的最大行数。
     */
    int maxLineCount() const;

    /**
     * @brief 设置指定 spdlog 级别的整行背景色。
     * @param spdlogLevel 与 `spdlog::level::level_enum` 数值一致（`0`…`5`）。
     * @param color 行背景；无效 `QColor()` 表示该行使用主题默认（含交替行）背景。
     */
    void setLevelBackgroundColor(int spdlogLevel, const QColor &color);

    /**
     * @brief 查询某级别的行背景色设置。
     * @param spdlogLevel 与 `spdlog::level::level_enum` 数值一致。
     * @return 已设置的颜色；未设置或非法级别时可能为无效色。
     */
    QColor levelBackgroundColor(int spdlogLevel) const;

    /**
     * @brief 设置该级别在文本中的关键字高亮色（如 `[info]`、独立单词 `warn`）。
     * @param spdlogLevel 与 `spdlog::level::level_enum` 数值一致。
     * @param color 关键字前景色。
     */
    void setLevelKeywordColor(int spdlogLevel, const QColor &color);

    /**
     * @brief 查询某级别的关键字高亮色。
     * @param spdlogLevel 与 `spdlog::level::level_enum` 数值一致。
     * @return 已设置的颜色。
     */
    QColor levelKeywordColor(int spdlogLevel) const;

    /**
     * @brief 将背景色与关键字色恢复为控件内置默认。
     * @note 默认策略下仅 critical 行有固定浅红背景，其余级别背景跟随主题。
     */
    void resetLevelColors();

Q_SIGNALS:
    /**
     * @brief 有一条日志行已追加到模型并显示（经代理过滤后仍可见与否由当前筛选决定）。
     * @param text 完整行文本。
     * @param spdlogLevel 与 `spdlog::level::level_enum` 一致的级别数值。
     */
    void sig_lineAppended(const QString &text, int spdlogLevel);

    /**
     * @brief 搜索框或级别筛选相关控件发生变化，且过滤规则已应用到代理模型之后发出。
     */
    void sig_filterRuleChanged();

public Q_SLOTS:
    /**
     * @brief 追加一行日志；级别由解析行内 `[level]` 得到，失败时按 info 处理。
     * @param text 一行日志字符串。
     */
    void slot_appendLine(const QString &text);

private Q_SLOTS:
    /**
     * @brief 由 spdlog sink 投递，携带格式化文本与真实 `msg.level`。
     * @param text 格式化后的一行。
     * @param spdlogLevel `static_cast<int>(msg.level)`。
     */
    void slot_appendLineWithLevel(const QString &text, int spdlogLevel);

    /**
     * @brief 响应搜索框 `textChanged`，刷新代理过滤并发 `sig_filterRuleChanged`。
     */
    void slot_searchEdit_textChanged(const QString &text);

    /**
     * @brief 响应「级别模式」下拉 `currentIndexChanged`，刷新代理过滤并发 `sig_filterRuleChanged`。
     */
    void slot_modeCombo_currentIndexChanged(int index);

    /**
     * @brief 响应「级别」下拉 `currentIndexChanged`，刷新代理过滤并发 `sig_filterRuleChanged`。
     */
    void slot_levelCombo_currentIndexChanged(int index);

private:
    friend class XlcLogWidgetPrivate;

    std::unique_ptr<XlcLogWidgetPrivate> d_ptr;
};

#endif
