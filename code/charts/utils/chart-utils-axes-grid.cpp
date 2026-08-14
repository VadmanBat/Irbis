#include "code/charts/utils/chart-utils-axes-detail.hpp"

#include <QValueAxis>

namespace chart_utils {
void applyViewerGrid(QValueAxis* axis) {
    if (axis)
        axes_detail::apply_viewer_grid(axis);
}

void applyViewerGrid(QValueAxis* axis, double lo, double hi) {
    if (axis)
        axes_detail::apply_viewer_grid(axis, lo, hi);
}

void applyViewerGrid(QChart* chart) {
    if (!chart)
        return;
    axes_detail::apply_viewer_grid(qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0, nullptr)));
    axes_detail::apply_viewer_grid(qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0, nullptr)));
}

void createAxes(QChart* chart, const QString& titleX, const QString& titleY) {
    auto* axis_x = new QValueAxis(chart);
    auto* axis_y = new QValueAxis(chart);
    axis_x->setTitleText(titleX);
    axis_y->setTitleText(titleY);
    axes_detail::apply_tab_grid_exact(axis_x);
    axes_detail::apply_tab_grid_exact(axis_y);
    const ChartTheme theme = currentTheme();
    applyAxisTheme(axis_x, theme);
    applyAxisTheme(axis_y, theme);
    chart->addAxis(axis_x, Qt::AlignBottom);
    chart->addAxis(axis_y, Qt::AlignLeft);
}
} // namespace chart_utils
