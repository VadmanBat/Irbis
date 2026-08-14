#include "code/dialogs/chart-viewer/chart-viewer-window.h"

#include "code/charts/utils/chart-utils.hpp"
#include "code/util/dialog-icons.hxx"
#include "code/util/format.hxx"

#include <QGuiApplication>
#include <QLabel>
#include <QScreen>
#include <QValueAxis>

ChartViewerWindow::ChartViewerWindow(QChart* source_chart, QWidget* parent) : QMainWindow(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("ChartViewerWindow"));
    dialog_icons::apply(this, dialog_icons::Kind::ChartProps);

    const QString chart_title = source_chart ? source_chart->title() : tr("График");
    setWindowTitle(chart_title.isEmpty() ? tr("Просмотр графика") : tr("Просмотр — %1").arg(chart_title));
    resize(1000, 700);

    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect ag = screen->availableGeometry();
        resize(qMin(1100, ag.width() - 80), qMin(750, ag.height() - 80));
        move(ag.center() - QPoint(width() / 2, height() / 2));
    }

    chart_ = chart_utils::cloneChart(source_chart);
    if (auto* ax = qobject_cast<QValueAxis*>(chart_->axes(Qt::Horizontal).value(0, nullptr)))
        home_x_ = {ax->min(), ax->max()};
    if (auto* ay = qobject_cast<QValueAxis*>(chart_->axes(Qt::Vertical).value(0, nullptr)))
        home_y_ = {ay->min(), ay->max()};

    view_ = new chart_viewer::InteractiveChartView(chart_, this);
    view_->setObjectName(QStringLiteral("chartViewerView"));
    setCentralWidget(view_);

    build_toolbar();
    build_status();
    setup_shortcuts();

    connect(view_, &chart_viewer::InteractiveChartView::cursorMoved, this, [this](double x, double y, bool inside) {
        if (!coord_label_)
            return;
        if (!inside) {
            coord_label_->setText(tr("Курсор вне области графика"));
            return;
        }
        coord_label_->setText(tr("x = %1    y = %2").arg(num_format::format(x), num_format::format(y)));
    });
}

void ChartViewerWindow::open(QChart* source_chart, QWidget* parent) {
    if (!source_chart)
        return;
    auto* w = new ChartViewerWindow(source_chart, parent);
    w->show();
    w->raise();
    w->activateWindow();
}
