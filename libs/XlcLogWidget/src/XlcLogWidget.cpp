/**
 * @file XlcLogWidget.cpp
 * @brief `XlcLogWidget` 的实现：含 PIMPL、列表代理、委托绘制与 spdlog sink。
 */

#include "XlcLogWidget.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMetaObject>
#include <QPainter>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <mutex>

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

namespace
{

constexpr int kLevelCount = static_cast<int>(spdlog::level::critical) + 1;

/**
 * @brief 判断 `spdlog` 级别是否属于本控件可着色、可筛选的显示范围。
 * @param level 与 `spdlog::level::level_enum` 数值一致。
 * @return `trace`…`critical` 为 `true`，否则为 `false`。
 */
bool isDisplayLevel(int level)
{
    return level >= static_cast<int>(spdlog::level::trace) && level <= static_cast<int>(spdlog::level::critical);
}

/**
 * @brief 从一行日志文本中解析 `[level]` 标签得到 `spdlog` 级别。
 * @param text 完整一行日志。
 * @return 解析到的级别；无匹配时返回 `info`。
 */
int parseLevelFromText(const QString &text)
{
    static const QRegularExpression re(
        QStringLiteral(R"(\[(trace|debug|info|warn|warning|err|error|critical)\])"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(text);
    if (!m.hasMatch())
    {
        return static_cast<int>(spdlog::level::info);
    }
    const QString w = m.captured(1).toLower();
    if (w == QLatin1String("trace"))
    {
        return static_cast<int>(spdlog::level::trace);
    }
    if (w == QLatin1String("debug"))
    {
        return static_cast<int>(spdlog::level::debug);
    }
    if (w == QLatin1String("info"))
    {
        return static_cast<int>(spdlog::level::info);
    }
    if (w == QLatin1String("warn") || w == QLatin1String("warning"))
    {
        return static_cast<int>(spdlog::level::warn);
    }
    if (w == QLatin1String("err") || w == QLatin1String("error"))
    {
        return static_cast<int>(spdlog::level::err);
    }
    if (w == QLatin1String("critical"))
    {
        return static_cast<int>(spdlog::level::critical);
    }
    return static_cast<int>(spdlog::level::info);
}

/**
 * @brief 从 `QVariant` 中读取存于 `Qt::UserRole` 的级别整数。
 * @param v 模型项上的 `UserRole` 数据。
 * @param fallback 无法解析为整数时的回退值，默认 `info`。
 * @return 解析得到的级别数值。
 */
int logLevelFromVariant(const QVariant &v, int fallback = static_cast<int>(spdlog::level::info))
{
    bool ok = false;
    const int n = v.toInt(&ok);
    return ok ? n : fallback;
}

/**
 * @brief 将日志关键字（含 `warning`/`error` 等同义）映射为 `0`…`5` 的下标。
 * @param kwLower 已小写化的关键字（不含方括号）。
 * @return 对应 `spdlog` 级别下标；无法识别时返回 `-1`。
 */
int keywordToLevelIndex(const QString &kwLower)
{
    if (kwLower == QLatin1String("trace"))
    {
        return static_cast<int>(spdlog::level::trace);
    }
    if (kwLower == QLatin1String("debug"))
    {
        return static_cast<int>(spdlog::level::debug);
    }
    if (kwLower == QLatin1String("info"))
    {
        return static_cast<int>(spdlog::level::info);
    }
    if (kwLower == QLatin1String("warn") || kwLower == QLatin1String("warning"))
    {
        return static_cast<int>(spdlog::level::warn);
    }
    if (kwLower == QLatin1String("err") || kwLower == QLatin1String("error"))
    {
        return static_cast<int>(spdlog::level::err);
    }
    if (kwLower == QLatin1String("critical"))
    {
        return static_cast<int>(spdlog::level::critical);
    }
    return -1;
}

} // namespace

class XlcLogWidgetPrivate;

void drawColoredLogLine(QPainter *painter, const QRect &rect, const QString &text, const QColor &defaultColor,
                        const XlcLogWidgetPrivate *style);

class XlcLogItemDelegate final : public QStyledItemDelegate
{
public:
    XlcLogItemDelegate(QListView *parent, const XlcLogWidgetPrivate *style)
        : QStyledItemDelegate(parent), m_style(style)
    {
    }

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const XlcLogWidgetPrivate *m_style;
};

class XlcLogFilterProxyModel final : public QSortFilterProxyModel
{
public:
    explicit XlcLogFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    enum FilterMode
    {
        ModeShowAll = 0,
        ModeAtLeast = 1,
        ModeExact = 2
    };

    void setSearchText(QString s)
    {
        m_search = std::move(s);
        invalidateFilter();
    }

    void setLevelRule(FilterMode mode, int spdlogLevel)
    {
        m_mode = mode;
        m_level = spdlogLevel;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        if (!idx.isValid())
        {
            return false;
        }
        const QString line = idx.data(Qt::DisplayRole).toString();
        if (!m_search.isEmpty() && !line.contains(m_search, Qt::CaseInsensitive))
        {
            return false;
        }
        const int rowLevel = logLevelFromVariant(idx.data(Qt::UserRole));
        if (m_mode == ModeShowAll)
        {
            return true;
        }
        if (m_mode == ModeAtLeast)
        {
            return rowLevel >= m_level;
        }
        return rowLevel == m_level;
    }

private:
    QString m_search;
    FilterMode m_mode{ModeShowAll};
    int m_level{static_cast<int>(spdlog::level::trace)};
};

class XlcLogListView final : public QListView
{
public:
    using QListView::QListView;

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_C && event->modifiers() == Qt::ControlModifier)
        {
            copySelectionToClipboard();
            event->accept();
            return;
        }
        QListView::keyPressEvent(event);
    }

private:
    void copySelectionToClipboard()
    {
        QModelIndexList rows = selectionModel()->selectedRows(0);
        std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b)
                  { return a.row() < b.row(); });
        QStringList out;
        out.reserve(rows.size());
        for (const QModelIndex &idx : rows)
        {
            out.push_back(idx.data(Qt::DisplayRole).toString());
        }
        QApplication::clipboard()->setText(out.join(QLatin1Char('\n')));
    }
};

template <typename Mutex>
class QtListSink : public spdlog::sinks::base_sink<Mutex>
{
public:
    explicit QtListSink(QPointer<XlcLogWidget> xlcLogWidget)
        : m_xlcLogWidget(std::move(xlcLogWidget))
    {
    }

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        spdlog::memory_buf_t formatted;
        if (this->formatter_)
        {
            this->formatter_->format(msg, formatted);
        }
        else
        {
            return;
        }

        XlcLogWidget *w = m_xlcLogWidget.data();
        if (!w)
        {
            return;
        }

        const QString line = QString::fromUtf8(formatted.data(), static_cast<int>(formatted.size()));
        const int lvl = static_cast<int>(msg.level);
        QMetaObject::invokeMethod(w, "slot_appendLineWithLevel", Qt::QueuedConnection, Q_ARG(QString, line),
                                  Q_ARG(int, lvl));
    }

    void flush_() override {}

private:
    QPointer<XlcLogWidget> m_xlcLogWidget;
};

using QtListSinkMt = QtListSink<std::mutex>;

class XlcLogWidgetPrivate
{
    friend class XlcLogWidget;

public:
    explicit XlcLogWidgetPrivate(XlcLogWidget *q)
        : q_ptr(q)
    {
        initDefaultColors();
    }

    void setupUi()
    {
        initItems();
        initLayout();
        initConnections();
        initWidget();
    }

    void initItems()
    {
        m_model = new QStandardItemModel(q_ptr);
        m_proxy = new XlcLogFilterProxyModel(q_ptr);
        m_proxy->setSourceModel(m_model);
        m_proxy->setDynamicSortFilter(true);

        m_listView = new XlcLogListView(q_ptr);
        m_listView->setModel(m_proxy);
        m_listView->setItemDelegate(new XlcLogItemDelegate(m_listView, this));
        m_listView->setUniformItemSizes(true);
        m_listView->setAlternatingRowColors(true);
        m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_listView->setFocusPolicy(Qt::StrongFocus);
        m_listView->setWordWrap(false);
        m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_labelSearch = new QLabel(QStringLiteral("\u641c\u7d22:"), q_ptr);
        m_searchEdit = new QLineEdit(q_ptr);
        m_searchEdit->setPlaceholderText(QStringLiteral("\u641c\u7d22\u5173\u952e\u5b57"));

        m_labelLevel = new QLabel(QStringLiteral("\u7ea7\u522b:"), q_ptr);
        m_modeCombo = new QComboBox(q_ptr);
        m_modeCombo->addItem(QStringLiteral("\u663e\u793a\u5168\u90e8"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeShowAll)));
        m_modeCombo->addItem(QStringLiteral("\u4e0d\u4f4e\u4e8e\u6240\u9009\u7ea7\u522b"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeAtLeast)));
        m_modeCombo->addItem(QStringLiteral("\u4ec5\u663e\u793a\u6240\u9009\u7ea7\u522b"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeExact)));

        m_levelCombo = new QComboBox(q_ptr);
        const struct
        {
            const char *name;
            int level;
        } levels[] = {
            {"trace", static_cast<int>(spdlog::level::trace)},
            {"debug", static_cast<int>(spdlog::level::debug)},
            {"info", static_cast<int>(spdlog::level::info)},
            {"warn", static_cast<int>(spdlog::level::warn)},
            {"err", static_cast<int>(spdlog::level::err)},
            {"critical", static_cast<int>(spdlog::level::critical)},
        };
        for (const auto &e : levels)
        {
            m_levelCombo->addItem(QString::fromUtf8(e.name), e.level);
        }
        m_levelCombo->setCurrentIndex(3);
    }

    void initLayout()
    {
        auto *filterRow = new QHBoxLayout();
        filterRow->addStretch();
        filterRow->addWidget(m_labelSearch);
        filterRow->addWidget(m_searchEdit, 1);
        filterRow->addWidget(m_labelLevel);
        filterRow->addWidget(m_modeCombo);
        filterRow->addWidget(m_levelCombo);

        auto *layout = new QVBoxLayout(q_ptr);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addLayout(filterRow);
        layout->addWidget(m_listView, 1);
    }

    void initConnections()
    {
        QObject::connect(m_searchEdit, &QLineEdit::textChanged, q_ptr, &XlcLogWidget::slot_searchEdit_textChanged);
        QObject::connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), q_ptr,
                         &XlcLogWidget::slot_modeCombo_currentIndexChanged);
        QObject::connect(m_levelCombo, qOverload<int>(&QComboBox::currentIndexChanged), q_ptr,
                         &XlcLogWidget::slot_levelCombo_currentIndexChanged);
    }

    void initWidget()
    {
        applyFilterRule();
    }

    void applyFilterRule()
    {
        const QString s = m_searchEdit->text();
        m_proxy->setSearchText(s);

        const int mode = m_modeCombo->currentData().toInt();
        const int level = m_levelCombo->currentData().toInt();
        m_proxy->setLevelRule(static_cast<XlcLogFilterProxyModel::FilterMode>(mode), level);
    }

    void trimIfNeeded()
    {
        while (m_model->rowCount() > m_maxLineCount)
        {
            m_model->removeRow(0);
        }
    }

    void appendLine(const QString &text, int spdlogLevel)
    {
        auto *item = new QStandardItem(text);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setData(spdlogLevel, Qt::UserRole);
        m_model->appendRow(item);
        trimIfNeeded();
        const int row = m_model->rowCount() - 1;
        if (row >= 0)
        {
            const QModelIndex src = m_model->index(row, 0);
            const QModelIndex inProxy = m_proxy->mapFromSource(src);
            m_listView->scrollTo(inProxy);
        }
    }

    void setLevelBackgroundColor(int spdlogLevel, const QColor &color)
    {
        if (!isDisplayLevel(spdlogLevel))
        {
            return;
        }
        m_rowBg[static_cast<size_t>(spdlogLevel)] = color;
        if (m_listView)
        {
            m_listView->viewport()->update();
        }
    }

    QColor levelBackgroundColor(int spdlogLevel) const
    {
        if (!isDisplayLevel(spdlogLevel))
        {
            return QColor();
        }
        return m_rowBg[static_cast<size_t>(spdlogLevel)];
    }

    void setLevelKeywordColor(int spdlogLevel, const QColor &color)
    {
        if (!isDisplayLevel(spdlogLevel))
        {
            return;
        }
        m_keyword[static_cast<size_t>(spdlogLevel)] = color;
        if (m_listView)
        {
            m_listView->viewport()->update();
        }
    }

    QColor levelKeywordColor(int spdlogLevel) const
    {
        if (!isDisplayLevel(spdlogLevel))
        {
            return QColor();
        }
        return m_keyword[static_cast<size_t>(spdlogLevel)];
    }

    void resetLevelColors()
    {
        initDefaultColors();
        if (m_listView)
        {
            m_listView->viewport()->update();
        }
    }

    void attachToDefaultLogger(const std::string &pattern)
    {
        if (m_sink)
        {
            return;
        }
        auto logger = spdlog::default_logger();
        if (!logger)
        {
            return;
        }

        auto sink = std::make_shared<QtListSinkMt>(q_ptr);
        sink->set_pattern(pattern);
        logger->sinks().push_back(sink);
        m_sink = std::move(sink);
    }

    void detachFromDefaultLogger()
    {
        if (!m_sink)
        {
            return;
        }
        auto logger = spdlog::default_logger();
        if (logger)
        {
            auto &sinks = logger->sinks();
            const auto it = std::find(sinks.begin(), sinks.end(), m_sink);
            if (it != sinks.end())
            {
                sinks.erase(it);
            }
        }
        m_sink.reset();
    }

    QColor keywordBrushForToken(const QString &kwLower) const
    {
        const int idx = keywordToLevelIndex(kwLower);
        if (idx < 0)
        {
            return QColor();
        }
        return m_keyword[static_cast<size_t>(idx)];
    }

    QColor rowBackground(int spdlogLevel, bool selected, bool alternate, const QPalette &pal) const
    {
        if (selected)
        {
            return pal.color(QPalette::Highlight);
        }
        if (isDisplayLevel(spdlogLevel) && m_rowBg[static_cast<size_t>(spdlogLevel)].isValid())
        {
            return m_rowBg[static_cast<size_t>(spdlogLevel)];
        }
        if (alternate)
        {
            return pal.color(QPalette::AlternateBase);
        }
        return pal.color(QPalette::Base);
    }

private:
    XlcLogWidget *q_ptr{nullptr};
    QListView *m_listView{nullptr};
    QStandardItemModel *m_model{nullptr};
    XlcLogFilterProxyModel *m_proxy{nullptr};

    QLabel *m_labelSearch{nullptr};
    QLabel *m_labelLevel{nullptr};
    QLineEdit *m_searchEdit{nullptr};
    QComboBox *m_modeCombo{nullptr};
    QComboBox *m_levelCombo{nullptr};

    int m_maxLineCount{5000};

    std::array<QColor, kLevelCount> m_rowBg{};
    std::array<QColor, kLevelCount> m_keyword{};
    std::shared_ptr<spdlog::sinks::sink> m_sink;

    void initDefaultColors()
    {
        m_keyword[static_cast<size_t>(spdlog::level::trace)] = QColor(120, 120, 120);
        m_keyword[static_cast<size_t>(spdlog::level::debug)] = QColor(34, 139, 34);
        m_keyword[static_cast<size_t>(spdlog::level::info)] = QColor(21, 101, 192);
        m_keyword[static_cast<size_t>(spdlog::level::warn)] = QColor(245, 124, 0);
        m_keyword[static_cast<size_t>(spdlog::level::err)] = QColor(198, 40, 40);
        m_keyword[static_cast<size_t>(spdlog::level::critical)] = QColor(183, 28, 28);

        m_rowBg.fill(QColor());
        m_rowBg[static_cast<size_t>(spdlog::level::critical)] = QColor(255, 210, 210);
    }
};

void drawColoredLogLine(QPainter *painter, const QRect &rect, const QString &text, const QColor &defaultColor,
                        const XlcLogWidgetPrivate *style)
{
    static const QRegularExpression re(
        QStringLiteral(R"((\[(?:trace|debug|info|warn|warning|err|error|critical)\])|)"
                       R"((\b(?:trace|debug|info|warn|err|critical)\b))"),
        QRegularExpression::CaseInsensitiveOption);

    const QFontMetrics fm(painter->font());
    int x = rect.left();
    const int baselineY = rect.top() + (rect.height() + fm.ascent() - fm.descent()) / 2;

    int pos = 0;
    auto it = re.globalMatch(text);
    while (it.hasNext())
    {
        const QRegularExpressionMatch m = it.next();
        const int start = m.capturedStart();
        if (start > pos)
        {
            painter->setPen(defaultColor);
            painter->drawText(x, baselineY, text.mid(pos, start - pos));
            x += fm.horizontalAdvance(text.mid(pos, start - pos));
        }
        const QString cap = m.captured();
        QString key = cap;
        if (key.startsWith(QLatin1Char('[')))
        {
            key = key.mid(1, key.size() - 2);
        }
        const QColor accent = style->keywordBrushForToken(key.toLower());
        painter->setPen(accent.isValid() ? accent : defaultColor);
        painter->drawText(x, baselineY, cap);
        x += fm.horizontalAdvance(cap);
        pos = m.capturedEnd();
    }
    if (pos < text.size())
    {
        painter->setPen(defaultColor);
        painter->drawText(x, baselineY, text.mid(pos));
    }
}

void XlcLogItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString text = index.data(Qt::DisplayRole).toString();
    const int lvl = logLevelFromVariant(index.data(Qt::UserRole));

    painter->save();
    painter->setClipRect(opt.rect);

    const QColor bgColor =
        m_style->rowBackground(lvl, opt.state.testFlag(QStyle::State_Selected),
                               opt.features.testFlag(QStyleOptionViewItem::Alternate), opt.palette);
    painter->fillRect(opt.rect, bgColor);

    const QColor defaultText =
        opt.state.testFlag(QStyle::State_Selected) ? opt.palette.color(QPalette::HighlightedText)
                                                   : opt.palette.color(QPalette::Text);

    QRect textRect = opt.rect;
    textRect.adjust(4, 0, -4, 0);
    drawColoredLogLine(painter, textRect, text, defaultText, m_style);

    if (opt.state.testFlag(QStyle::State_HasFocus))
    {
        QStyleOptionFocusRect fropt;
        fropt.QStyleOption::operator=(opt);
        fropt.rect = opt.rect;
        fropt.state = opt.state | QStyle::State_KeyboardFocusChange;
        const QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_FrameFocusRect, &fropt, painter, opt.widget);
    }

    painter->restore();
}

XlcLogWidget::XlcLogWidget(QWidget *parent)
    : QWidget(parent), d_ptr(std::make_unique<XlcLogWidgetPrivate>(this))
{
    d_ptr->setupUi();
}

XlcLogWidget::~XlcLogWidget()
{
    detachFromDefaultLogger();
}

void XlcLogWidget::setLevelBackgroundColor(int spdlogLevel, const QColor &color)
{
    d_ptr->setLevelBackgroundColor(spdlogLevel, color);
}

QColor XlcLogWidget::levelBackgroundColor(int spdlogLevel) const
{
    return d_ptr->levelBackgroundColor(spdlogLevel);
}

void XlcLogWidget::setLevelKeywordColor(int spdlogLevel, const QColor &color)
{
    d_ptr->setLevelKeywordColor(spdlogLevel, color);
}

QColor XlcLogWidget::levelKeywordColor(int spdlogLevel) const
{
    return d_ptr->levelKeywordColor(spdlogLevel);
}

void XlcLogWidget::resetLevelColors()
{
    d_ptr->resetLevelColors();
}

void XlcLogWidget::attachToDefaultLogger(const std::string &pattern)
{
    d_ptr->attachToDefaultLogger(pattern);
}

void XlcLogWidget::detachFromDefaultLogger()
{
    d_ptr->detachFromDefaultLogger();
}

void XlcLogWidget::setMaxLineCount(int max)
{
    d_ptr->m_maxLineCount = max > 0 ? max : 5000;
    d_ptr->trimIfNeeded();
}

int XlcLogWidget::maxLineCount() const
{
    return d_ptr->m_maxLineCount;
}

void XlcLogWidget::slot_appendLine(const QString &text)
{
    const int lvl = parseLevelFromText(text);
    d_ptr->appendLine(text, lvl);
    Q_EMIT sig_lineAppended(text, lvl);
}

void XlcLogWidget::slot_appendLineWithLevel(const QString &text, int spdlogLevel)
{
    d_ptr->appendLine(text, spdlogLevel);
    Q_EMIT sig_lineAppended(text, spdlogLevel);
}

void XlcLogWidget::slot_searchEdit_textChanged(const QString & /*text*/)
{
    d_ptr->applyFilterRule();
    Q_EMIT sig_filterRuleChanged();
}

void XlcLogWidget::slot_modeCombo_currentIndexChanged(int /*index*/)
{
    d_ptr->applyFilterRule();
    Q_EMIT sig_filterRuleChanged();
}

void XlcLogWidget::slot_levelCombo_currentIndexChanged(int /*index*/)
{
    d_ptr->applyFilterRule();
    Q_EMIT sig_filterRuleChanged();
}
