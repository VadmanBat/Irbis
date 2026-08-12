#include "code/dialogs/chart-dialog.h"
#include "code/dialogs/chart-viewer/chart-viewer-window.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLegend>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QStatusBar>
#include <QToolBar>
#include <QWidget>

namespace {
/// Toolbar action: emoji + label (emoji must be in Segoe UI Emoji / color font).
QAction* add_tb_action(QToolBar* tb, const QString& emoji, const QString& label) {
    const QString text = label.isEmpty() ? emoji : (emoji + QStringLiteral("  ") + label);
    return tb->addAction(text);
}
} // namespace

void ChartViewerWindow::build_toolbar() {
    auto* tb = addToolBar(tr("Навигация"));
    tb->setObjectName(QStringLiteral("chartViewerToolBar"));
    tb->setMovable(false);
    tb->setFloatable(false);
    tb->setIconSize(QSize(18, 18));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setStyleSheet(QString()); // use app QSS via objectName

    auto* tools = new QActionGroup(this);
    tools->setExclusive(true);

    // Tools — only widely-supported emoji (no geometric symbols that become □).
    // Z / H switch tools — they do not zoom by themselves.
    act_zoom_ = add_tb_action(tb, QStringLiteral("🔍"), tr("Рамка"));
    act_zoom_->setCheckable(true);
    act_zoom_->setChecked(true);
    act_zoom_->setToolTip(
        tr("Инструмент: зум рамкой\n"
           "ЛКМ — выделить область (приблизить)\n"
           "Колесо — зум к курсору · клавиша Z — выбрать этот режим"));
    act_zoom_->setShortcut(QKeySequence(Qt::Key_Z));
    tools->addAction(act_zoom_);

    act_pan_ = add_tb_action(tb, QStringLiteral("✋"), tr("Сдвиг"));
    act_pan_->setCheckable(true);
    act_pan_->setToolTip(
        tr("Инструмент: панорамирование\n"
           "ЛКМ или СКМ — сдвиг · клавиша H — выбрать этот режим"));
    act_pan_->setShortcut(QKeySequence(Qt::Key_H));
    tools->addAction(act_pan_);

    tb->addSeparator();

    act_zoom_in_  = add_tb_action(tb, QStringLiteral("➕"), {});
    act_zoom_out_ = add_tb_action(tb, QStringLiteral("➖"), {});
    act_home_     = add_tb_action(tb, QStringLiteral("🏠"), tr("Исходный"));
    act_zoom_in_->setToolTip(tr("Приблизить (+)"));
    act_zoom_out_->setToolTip(tr("Отдалить (−)"));
    act_home_->setToolTip(tr("Сбросить вид (Home / двойной клик)"));
    act_home_->setShortcut(QKeySequence(Qt::Key_Home));
    act_zoom_in_->setShortcut(QKeySequence(Qt::Key_Plus));
    act_zoom_out_->setShortcut(QKeySequence(Qt::Key_Minus));

    tb->addSeparator();

    act_grid_ = add_tb_action(tb, QStringLiteral("🔲"), tr("Сетка"));
    act_grid_->setCheckable(true);
    act_grid_->setChecked(true);
    act_grid_->setToolTip(tr("Показать / скрыть сетку · G"));
    act_grid_->setShortcut(QKeySequence(tr("G")));

    act_legend_ = add_tb_action(tb, QStringLiteral("🏷️"), tr("Легенда"));
    act_legend_->setCheckable(true);
    act_legend_->setChecked(chart_->legend() && chart_->legend()->isVisible());
    act_legend_->setToolTip(tr("Показать / скрыть легенду · L"));
    act_legend_->setShortcut(QKeySequence(tr("L")));

    tb->addSeparator();

    act_save_  = add_tb_action(tb, QStringLiteral("💾"), tr("Сохранить"));
    act_copy_  = add_tb_action(tb, QStringLiteral("📋"), tr("Копировать"));
    act_props_ = add_tb_action(tb, QStringLiteral("⚙️"), tr("Свойства"));
    act_save_->setToolTip(tr("Сохранить как PNG / SVG / TXT"));
    act_copy_->setToolTip(tr("Копировать изображение в буфер"));
    act_props_->setToolTip(tr("Цвета, толщины, подписи"));

    // Push window chrome to the right.
    auto* spacer = new QWidget(tb);
    spacer->setObjectName(QStringLiteral("chartViewerToolBarSpacer"));
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->addWidget(spacer);

    act_fullscreen_ = add_tb_action(tb, QStringLiteral("🖥️"), tr("На весь экран"));
    act_fullscreen_->setCheckable(true);
    act_fullscreen_->setShortcut(QKeySequence(Qt::Key_F11));
    act_fullscreen_->setToolTip(tr("Полноэкранный режим · F11"));

    act_close_ = add_tb_action(tb, QStringLiteral("❌"), {});
    act_close_->setToolTip(tr("Закрыть · Esc"));
    act_close_->setShortcut(QKeySequence(Qt::Key_Escape));

    connect(act_zoom_, &QAction::triggered, this,
            [this] { view_->setTool(chart_viewer::InteractiveChartView::Tool::ZoomRect); });
    connect(act_pan_, &QAction::triggered, this,
            [this] { view_->setTool(chart_viewer::InteractiveChartView::Tool::Pan); });
    connect(act_zoom_in_, &QAction::triggered, view_, &chart_viewer::InteractiveChartView::zoomInStep);
    connect(act_zoom_out_, &QAction::triggered, view_, &chart_viewer::InteractiveChartView::zoomOutStep);
    connect(act_home_, &QAction::triggered, this, [this] { view_->resetView(home_x_, home_y_); });
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

    auto* hints = new QLabel(tr("Z — режим рамки · H — сдвиг · +/− / колесо — масштаб · Home — сброс · F11 · Esc"));
    hints->setObjectName(QStringLiteral("chartViewerHintsLabel"));
    statusBar()->addPermanentWidget(hints);
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

void ChartViewerWindow::toggle_fullscreen(bool on) {
    if (on)
        showFullScreen();
    else
        showNormal();
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
