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
#include <QVariant>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QVBoxLayout>

#include <algorithm>
#include <mutex>

#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

/// 供 appendLine() 与匿名命名空间外代码使用：从行内 [level] 解析 spdlog 级别。
static int parseLevelFromText(const QString &text)
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

/// Qt 5 的 QVariant::toInt 无默认值重载；用 bool * 解析失败时回退。
static int logLevelFromVariant(const QVariant &v, int fallback = static_cast<int>(spdlog::level::info))
{
    bool ok = false;
    const int n = v.toInt(&ok);
    return ok ? n : fallback;
}

namespace
{

/// 与源模型中 Qt::UserRole 一致：存 spdlog::level::level_enum 的 int
constexpr int kLogLevelRole = Qt::UserRole;

QColor keywordColor(const QString &kwLower)
{
    if (kwLower == QLatin1String("trace"))
    {
        return QColor(120, 120, 120);
    }
    if (kwLower == QLatin1String("debug"))
    {
        return QColor(34, 139, 34);
    }
    if (kwLower == QLatin1String("info"))
    {
        return QColor(21, 101, 192);
    }
    if (kwLower == QLatin1String("warn") || kwLower == QLatin1String("warning"))
    {
        return QColor(245, 124, 0);
    }
    if (kwLower == QLatin1String("err") || kwLower == QLatin1String("error"))
    {
        return QColor(198, 40, 40);
    }
    if (kwLower == QLatin1String("critical"))
    {
        return QColor(183, 28, 28);
    }
    return QColor();
}

/// 高亮 [level] 以及独立单词形式的级别关键字（大小写不敏感）
void drawColoredLogLine(QPainter *painter, const QRect &rect, const QString &text, const QColor &defaultColor)
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
        const QColor accent = keywordColor(key.toLower());
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

class XlcLogItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        const QString text = index.data(Qt::DisplayRole).toString();
        const int lvl = logLevelFromVariant(index.data(kLogLevelRole));
        const bool isCritical = (lvl == static_cast<int>(spdlog::level::critical));

        painter->save();
        painter->setClipRect(opt.rect);

        QColor bgColor = opt.palette.color(QPalette::Base);
        if (opt.state.testFlag(QStyle::State_Selected))
        {
            bgColor = opt.palette.color(QPalette::Highlight);
        }
        else if (isCritical)
        {
            bgColor = QColor(255, 210, 210);
        }
        else if (opt.features.testFlag(QStyleOptionViewItem::Alternate))
        {
            bgColor = opt.palette.color(QPalette::AlternateBase);
        }
        painter->fillRect(opt.rect, bgColor);

        const QColor defaultText =
            opt.state.testFlag(QStyle::State_Selected) ? opt.palette.color(QPalette::HighlightedText)
                                                       : opt.palette.color(QPalette::Text);

        QRect textRect = opt.rect;
        textRect.adjust(4, 0, -4, 0);
        drawColoredLogLine(painter, textRect, text, defaultText);

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
};

} // namespace

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

namespace
{

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
        std::sort(rows.begin(), rows.end(), [](const QModelIndex &a, const QModelIndex &b) {
            return a.row() < b.row();
        });
        QStringList out;
        out.reserve(rows.size());
        for (const QModelIndex &idx : rows)
        {
            out.push_back(idx.data(Qt::DisplayRole).toString());
        }
        QApplication::clipboard()->setText(out.join(QLatin1Char('\n')));
    }
};

/// 将 spdlog 输出投递到 XlcLogWidget（主线程、队列连接）。
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
        QMetaObject::invokeMethod(w, "appendLineWithLevel", Qt::QueuedConnection, Q_ARG(QString, line),
                                  Q_ARG(int, lvl));
    }

    void flush_() override {}

private:
    QPointer<XlcLogWidget> m_xlcLogWidget;
};

using QtListSinkMt = QtListSink<std::mutex>;

} // namespace

XlcLogWidget::XlcLogWidget(QWidget *parent)
    : QWidget(parent)
{
    m_model = new QStandardItemModel(this);
    m_proxy = new XlcLogFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setDynamicSortFilter(true);

    m_listView = new XlcLogListView(this);
    m_listView->setModel(m_proxy);
    m_listView->setItemDelegate(new XlcLogItemDelegate(m_listView));
    m_listView->setUniformItemSizes(true);
    m_listView->setAlternatingRowColors(true);
    m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setFocusPolicy(Qt::StrongFocus);
    m_listView->setWordWrap(false);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("\u641c\u7d22\u5173\u952e\u5b57"));
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(QStringLiteral("\u663e\u793a\u5168\u90e8"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeShowAll)));
    m_modeCombo->addItem(QStringLiteral("\u4e0d\u4f4e\u4e8e\u6240\u9009\u7ea7\u522b"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeAtLeast)));
    m_modeCombo->addItem(QStringLiteral("\u4ec5\u663e\u793a\u6240\u9009\u7ea7\u522b"), QVariant::fromValue(static_cast<int>(XlcLogFilterProxyModel::ModeExact)));

    m_levelCombo = new QComboBox(this);
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

    auto *filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(QStringLiteral("\u641c\u7d22:"), this));
    filterRow->addWidget(m_searchEdit, 1);
    filterRow->addWidget(new QLabel(QStringLiteral("\u7ea7\u522b:"), this));
    filterRow->addWidget(m_modeCombo);
    filterRow->addWidget(m_levelCombo);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(filterRow);
    layout->addWidget(m_listView, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &XlcLogWidget::onFilterInputsChanged);
    connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &XlcLogWidget::onFilterInputsChanged);
    connect(m_levelCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &XlcLogWidget::onFilterInputsChanged);

    onFilterInputsChanged();
}

XlcLogWidget::~XlcLogWidget()
{
    detachFromDefaultLogger();
}

void XlcLogWidget::attachToDefaultLogger(const std::string &pattern)
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

    auto sink = std::make_shared<QtListSinkMt>(this);
    sink->set_pattern(pattern);
    logger->sinks().push_back(sink);
    m_sink = std::move(sink);
}

void XlcLogWidget::detachFromDefaultLogger()
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

void XlcLogWidget::setMaxLineCount(int max)
{
    m_maxLineCount = max > 0 ? max : 5000;
    trimIfNeeded();
}

int XlcLogWidget::maxLineCount() const
{
    return m_maxLineCount;
}

void XlcLogWidget::appendLine(const QString &text)
{
    appendLineWithLevel(text, parseLevelFromText(text));
}

void XlcLogWidget::appendLineWithLevel(const QString &text, int spdlogLevel)
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

void XlcLogWidget::onFilterInputsChanged()
{
    const QString s = m_searchEdit->text();
    m_proxy->setSearchText(s);

    const int mode = m_modeCombo->currentData().toInt();
    const int level = m_levelCombo->currentData().toInt();
    m_proxy->setLevelRule(static_cast<XlcLogFilterProxyModel::FilterMode>(mode), level);
}

void XlcLogWidget::trimIfNeeded()
{
    while (m_model->rowCount() > m_maxLineCount)
    {
        m_model->removeRow(0);
    }
}
