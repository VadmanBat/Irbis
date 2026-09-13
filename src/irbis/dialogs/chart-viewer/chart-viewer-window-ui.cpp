#include "irbis/dialogs/chart-dialog.h"
#include "irbis/dialogs/chart-viewer/chart-viewer-window.h"
#include "irbis/util/dialog-icons.hxx"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLegend>
#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOption>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

namespace {
constexpr int kTbIcon        = 18;
constexpr int kTbIconTextGap = 8;

/// QSS QToolButton left-aligns the icon and sits it above the text baseline.
/// Paint the pair ourselves: same vertical midline, group centered in the button.
class ChartToolButton final : public QToolButton {
public:
    explicit ChartToolButton(QWidget* parent = nullptr) : QToolButton(parent) {
        setAutoRaise(true);
        setFocusPolicy(Qt::TabFocus);
        setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }

    QSize sizeHint() const override {
        QSize s = QToolButton::sizeHint();
        if (!text().isEmpty() && !icon().isNull())
            s.rwidth() += kTbIconTextGap;
        return s;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QStyleOptionToolButton opt;
        initStyleOption(&opt);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QStyleOptionToolButton panel = opt;
        panel.text.clear();
        panel.icon = QIcon();
        style()->drawComplexControl(QStyle::CC_ToolButton, &panel, &p, this);

        const QRect r       = contentsRect();
        const QSize is      = iconSize();
        const bool has_icon = !opt.icon.isNull();
        const bool has_text = !opt.text.isEmpty();
        const int gap       = (has_icon && has_text) ? kTbIconTextGap : 0;
        const int text_w    = has_text ? opt.fontMetrics.horizontalAdvance(opt.text) : 0;
        const int icon_w    = has_icon ? is.width() : 0;
        const int total_w   = icon_w + gap + text_w;
        const int x0        = r.x() + (r.width() - total_w) / 2;
        const int mid_y     = r.center().y();

        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if (opt.state & (QStyle::State_On | QStyle::State_Sunken))
            mode = QIcon::Selected;
        else if (opt.state & QStyle::State_MouseOver)
            mode = QIcon::Active;
        const QIcon::State istate = (opt.state & QStyle::State_On) ? QIcon::On : QIcon::Off;

        if (has_icon)
            opt.icon.paint(&p, QRect(x0, mid_y - is.height() / 2, is.width(), is.height()), Qt::AlignCenter, mode,
                           istate);
        if (has_text) {
            const QColor color = (opt.state & (QStyle::State_On | QStyle::State_Sunken))
                                     ? opt.palette.color(QPalette::HighlightedText)
                                     : opt.palette.color(QPalette::ButtonText);
            p.setFont(opt.font);
            p.setPen(color);
            p.drawText(QRect(x0 + icon_w + gap, r.y(), text_w, r.height()), Qt::AlignVCenter | Qt::AlignLeft, opt.text);
        }
    }
};

struct ChartTbItem {
    QAction* act;
    QToolButton* btn;
};

ChartTbItem make_chart_item(QWidget* parent, const QPalette& pal, QChar glyph, const QString& label) {
    auto* act = new QAction(dialog_icons::paletteGlyphIcon(glyph, kTbIcon, pal), label, parent);
    auto* btn = new ChartToolButton(parent);
    btn->setDefaultAction(act);
    btn->setIconSize(QSize(kTbIcon, kTbIcon));
    return {act, btn};
}

QAction* add_tb_action(QToolBar* tb, QChar glyph, const QString& label) {
    auto item = make_chart_item(tb, tb->palette(), glyph, label);
    tb->addWidget(item.btn);
    return item.act;
}
} // namespace

void ChartViewerWindow::build_toolbar() {
    auto* tb = addToolBar(tr("Навигация"));
    tb->setObjectName(QStringLiteral("chartViewerToolBar"));
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setIconSize(QSize(kTbIcon, kTbIcon));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setStyleSheet(QString()); // use app QSS via objectName

    auto* tools = new QActionGroup(this);
    tools->setExclusive(true);

    // Z / H switch tools — they do not zoom by themselves.
    act_zoom_ = add_tb_action(tb, QChar(0xf002), tr("Рамка")); // magnifying-glass
    act_zoom_->setCheckable(true);
    act_zoom_->setChecked(true);
    act_zoom_->setToolTip(
        tr("Инструмент: зум рамкой\n"
           "ЛКМ — выделить область (приблизить)\n"
           "Колесо — зум к курсору · клавиша Z — выбрать этот режим"));
    act_zoom_->setShortcut(QKeySequence(Qt::Key_Z));
    tools->addAction(act_zoom_);

    act_pan_ = add_tb_action(tb, QChar(0xf0b2), tr("Сдвиг")); // up-down-left-right
    act_pan_->setCheckable(true);
    act_pan_->setToolTip(
        tr("Инструмент: панорамирование\n"
           "ЛКМ или СКМ — сдвиг · клавиша H — выбрать этот режим"));
    act_pan_->setShortcut(QKeySequence(Qt::Key_H));
    tools->addAction(act_pan_);

    tb->addSeparator();

    act_grid_ = add_tb_action(tb, QChar(0xf84c), tr("Сетка")); // border-all
    act_grid_->setCheckable(true);
    act_grid_->setChecked(true);
    act_grid_->setToolTip(tr("Показать / скрыть сетку · G"));
    act_grid_->setShortcut(QKeySequence(tr("G")));

    act_legend_ = add_tb_action(tb, QChar(0xf02b), tr("Легенда")); // tag
    act_legend_->setCheckable(true);
    act_legend_->setChecked(chart_->legend() && chart_->legend()->isVisible());
    act_legend_->setToolTip(tr("Показать / скрыть легенду · L"));
    act_legend_->setShortcut(QKeySequence(tr("L")));

    tb->addSeparator();

    act_save_  = add_tb_action(tb, QChar(0xf0c7), tr("Сохранить"));  // floppy-disk
    act_copy_  = add_tb_action(tb, QChar(0xf0c5), tr("Копировать")); // copy
    act_props_ = add_tb_action(tb, QChar(0xf013), tr("Свойства"));   // gear
    act_save_->setToolTip(tr("Сохранить как PNG / SVG / TXT"));
    act_copy_->setToolTip(tr("Копировать изображение в буфер"));
    act_props_->setToolTip(tr("Цвета, толщины, подписи"));

    // Push window chrome to the right.
    auto* spacer = new QWidget(tb);
    spacer->setObjectName(QStringLiteral("chartViewerToolBarSpacer"));
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    act_fullscreen_ = add_tb_action(tb, QChar(0xf065), tr("На весь экран")); // expand
    act_fullscreen_->setCheckable(true);
    act_fullscreen_->setShortcut(QKeySequence(Qt::Key_F11));
    act_fullscreen_->setToolTip(tr("Полноэкранный режим · F11"));

    const auto close_item = make_chart_item(tb, tb->palette(), QChar(0xf00d), {});
    act_close_            = close_item.act;
    act_close_->setToolTip(tr("Закрыть окно"));
    close_tb_action_ = tb->addWidget(close_item.btn);
    close_tb_action_->setVisible(false);

    connect(act_zoom_, &QAction::triggered, this,
            [this] { view_->setTool(chart_viewer::InteractiveChartView::Tool::ZoomRect); });
    connect(act_pan_, &QAction::triggered, this,
            [this] { view_->setTool(chart_viewer::InteractiveChartView::Tool::Pan); });
    connect(act_grid_, &QAction::toggled, view_, &chart_viewer::InteractiveChartView::setGridVisible);
    connect(act_legend_, &QAction::toggled, this, [this](bool on) {
        if (chart_->legend())
            chart_->legend()->setVisible(on);
    });
    connect(act_save_, &QAction::triggered, this, &ChartViewerWindow::save_as);
    connect(act_copy_, &QAction::triggered, this, &ChartViewerWindow::copy_image);
    connect(act_props_, &QAction::triggered, this, &ChartViewerWindow::open_properties);
    connect(act_fullscreen_, &QAction::toggled, this, &ChartViewerWindow::toggle_fullscreen);
    connect(act_close_, &QAction::triggered, this, &QWidget::close);

    view_->viewport()->installEventFilter(this);
}

void ChartViewerWindow::build_status() {
    coord_label_ = new QLabel(tr("Наведите курсор на график"));
    coord_label_->setObjectName(QStringLiteral("chartViewerCoordLabel"));
    statusBar()->setObjectName(QStringLiteral("chartViewerStatusBar"));
    statusBar()->setSizeGripEnabled(true);
    statusBar()->addWidget(coord_label_, 1);

    auto* hints = new QLabel(tr("Z — рамка · H — сдвиг · колесо — масштаб · F11 · Esc"));
    hints->setObjectName(QStringLiteral("chartViewerHintsLabel"));
    statusBar()->addPermanentWidget(hints);

    auto add_view_btn = [this](QChar glyph, const QString& label) {
        auto item = make_chart_item(statusBar(), palette(), glyph, label);
        statusBar()->addPermanentWidget(item.btn);
        return item.act;
    };
    act_home_     = add_view_btn(QChar(0xf015), tr("Исходный")); // house
    act_zoom_in_  = add_view_btn(QChar(0xf067), {});             // plus
    act_zoom_out_ = add_view_btn(QChar(0xf068), {});             // minus
    act_home_->setToolTip(tr("Сбросить вид (Home / двойной клик)"));
    act_zoom_in_->setToolTip(tr("Приблизить (+)"));
    act_zoom_out_->setToolTip(tr("Отдалить (−)"));
    act_home_->setShortcut(QKeySequence(Qt::Key_Home));
    act_zoom_in_->setShortcut(QKeySequence(Qt::Key_Plus));
    act_zoom_out_->setShortcut(QKeySequence(Qt::Key_Minus));

    connect(act_home_, &QAction::triggered, this, [this] { view_->resetView(home_x_, home_y_); });
    connect(act_zoom_in_, &QAction::triggered, view_, &chart_viewer::InteractiveChartView::zoomInStep);
    connect(act_zoom_out_, &QAction::triggered, view_, &chart_viewer::InteractiveChartView::zoomOutStep);
}

void ChartViewerWindow::setup_shortcuts() {
    addAction(act_zoom_);
    addAction(act_pan_);
    addAction(act_home_);
    addAction(act_grid_);
    addAction(act_legend_);
    addAction(act_fullscreen_);
    addAction(act_close_);
    addAction(act_zoom_in_);
    addAction(act_zoom_out_);
}

void ChartViewerWindow::save_as() {
    chart_utils::saveChartAsDialog(this, view_, chart_ ? chart_->title() : QString{});
}

void ChartViewerWindow::copy_image() {
    QApplication::clipboard()->setImage(view_->grab().toImage());
    statusBar()->showMessage(tr("Изображение скопировано"), 2000);
}

void ChartViewerWindow::open_properties() {
    ChartDialog dialog(chart_, this);
    dialog.exec();
}

void ChartViewerWindow::sync_close_action() {
    if (close_tb_action_)
        close_tb_action_->setVisible(isFullScreen());
}

void ChartViewerWindow::toggle_fullscreen(bool on) {
    if (on)
        showFullScreen();
    else
        showNormal();
    sync_close_action();
}

void ChartViewerWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        sync_close_action();
}

void ChartViewerWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (isFullScreen()) {
            act_fullscreen_->setChecked(false);
            event->accept();
            return;
        }
        close();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

bool ChartViewerWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == view_->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            view_->resetView(home_x_, home_y_);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
