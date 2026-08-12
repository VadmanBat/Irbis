#include "code/charts/utils/chart-utils.hpp"
#include "code/dialogs/chart-dialog.h"
#include "code/dialogs/chart-viewer/chart-viewer-window.h"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

#define CHART_TR(str) QCoreApplication::translate("chart_utils", str)

namespace chart_utils {
namespace {
/// Menu icon from emoji (same visual language as chart viewer toolbar).
QIcon menu_icon(const QString& emoji) {
    constexpr int size = 18;
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::TextAntialiasing);
    QFont font(QStringLiteral("Segoe UI Emoji"));
    font.setPixelSize(14);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, emoji);
    return QIcon(pm);
}
} // namespace

void openChartViewer(QChart* chart, QWidget* parent) {
    ChartViewerWindow::open(chart, parent);
}

void createChartContextMenu(QChartView* chart_view) {
    auto* open_viewer_action = new QAction(menu_icon(QStringLiteral("🖼️")), CHART_TR("Открыть в окне…"), chart_view);
    auto* save_as_action     = new QAction(menu_icon(QStringLiteral("💾")), CHART_TR("Сохранить как…"), chart_view);
    auto* copy_action = new QAction(menu_icon(QStringLiteral("📋")), CHART_TR("Копировать изображение"), chart_view);
    auto* properties_action = new QAction(menu_icon(QStringLiteral("⚙️")), CHART_TR("Свойства"), chart_view);

    QChart* chart = chart_view->chart();

    QObject::connect(open_viewer_action, &QAction::triggered,
                     [chart, chart_view] { openChartViewer(chart, chart_view->window()); });

    QObject::connect(save_as_action, &QAction::triggered, [chart, chart_view] {
        const QString name = chart ? chart->title() : QString{};
        saveChartAsDialog(chart_view->window(), chart_view, name);
    });

    QObject::connect(copy_action, &QAction::triggered,
                     [chart_view] { QApplication::clipboard()->setImage(chart_view->grab().toImage()); });

    QObject::connect(properties_action, &QAction::triggered, [chart] {
        ChartDialog dialog(chart);
        dialog.exec();
    });

    auto* context_menu = new QMenu(chart_view);
    context_menu->addAction(open_viewer_action);
    context_menu->addSeparator();
    context_menu->addAction(save_as_action);
    context_menu->addAction(copy_action);
    context_menu->addSeparator();
    context_menu->addAction(properties_action);

    chart_view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(
        chart_view, &QChartView::customContextMenuRequested,
        [context_menu, chart_view](const QPoint& pos) { context_menu->exec(chart_view->mapToGlobal(pos)); });

    struct DblOpen final : QObject {
        QChartView* view;
        explicit DblOpen(QChartView* v) : QObject(v), view(v) { v->viewport()->installEventFilter(this); }
        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (obj == view->viewport() && ev->type() == QEvent::MouseButtonDblClick) {
                auto* me = static_cast<QMouseEvent*>(ev);
                if (me->button() == Qt::LeftButton) {
                    openChartViewer(view->chart(), view->window());
                    return true;
                }
            }
            return QObject::eventFilter(obj, ev);
        }
    };
    new DblOpen(chart_view);
}
} // namespace chart_utils
