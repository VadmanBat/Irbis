#include "irbis/charts/utils/chart-utils.hpp"
#include "irbis/dialogs/chart-dialog.h"
#include "irbis/dialogs/chart-viewer/chart-viewer-window.h"
#include "irbis/util/dialog-icons.hxx"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>

#define CHART_TR(str) QCoreApplication::translate("chart_utils", str)

namespace chart_utils {
namespace {
QIcon menu_icon(QChar glyph) {
    return dialog_icons::paletteGlyphIcon(glyph, 16, QApplication::palette());
}
} // namespace

void openChartViewer(QChart* chart, QWidget* parent) {
    ChartViewerWindow::open(chart, parent);
}

void createChartContextMenu(QChartView* chart_view) {
    auto* open_viewer_action =
        new QAction(menu_icon(QChar(0xf08e)), CHART_TR("Открыть в окне…"), chart_view); // arrow-up-right-from-square
    auto* save_as_action = new QAction(menu_icon(QChar(0xf0c7)), CHART_TR("Сохранить как…"), chart_view); // floppy-disk
    auto* copy_action = new QAction(menu_icon(QChar(0xf0c5)), CHART_TR("Копировать изображение"), chart_view); // copy
    auto* properties_action = new QAction(menu_icon(QChar(0xf013)), CHART_TR("Свойства"), chart_view);         // gear

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
