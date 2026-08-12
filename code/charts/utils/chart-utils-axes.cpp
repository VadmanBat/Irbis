#include "code/charts/utils/chart-utils-axes-detail.hpp"

#include <QLegend>
#include <QLegendMarker>
#include <QLineSeries>
#include <QSignalBlocker>
#include <QValueAxis>

namespace chart_utils {
namespace {
QLineSeries* find_series_by_name(QChart* chart, const char* name) {
    for (auto* s : chart->series()) {
        if (s->name() == QLatin1String(name))
            return qobject_cast<QLineSeries*>(s);
    }
    return nullptr;
}

void hide_legend_marker(QChart* chart, QAbstractSeries* series) {
    if (!chart->legend())
        return;
    for (auto* marker : chart->legend()->markers(series))
        marker->setVisible(false);
}
} // namespace

void updateOriginGuides(QChart* chart, const Pair& range_x, const Pair& range_y) {
    if (!chart)
        return;

    auto ensure = [chart](const char* name) -> QLineSeries* {
        auto* s = find_series_by_name(chart, name);
        if (s)
            return s;
        s = new QLineSeries;
        s->setName(QString::fromLatin1(name));
        s->setPen(guidePen(currentTheme()));
        s->setPointsVisible(false);
        chart->addSeries(s);
        detail::attachToAxes(chart, s);
        hide_legend_marker(chart, s);
        return s;
    };

    auto* hor = ensure(kHorGuide);
    auto* ver = ensure(kVerGuide);

    const bool hor_ok = hor->count() == 2 && hor->at(0).x() == range_x.first && hor->at(1).x() == range_x.second &&
                        hor->at(0).y() == 0.0 && hor->at(1).y() == 0.0;
    const bool ver_ok = ver->count() == 2 && ver->at(0).y() == range_y.first && ver->at(1).y() == range_y.second &&
                        ver->at(0).x() == 0.0 && ver->at(1).x() == 0.0;
    if (!hor_ok)
        hor->replace(QList<QPointF>{{range_x.first, 0.0}, {range_x.second, 0.0}});
    if (!ver_ok)
        ver->replace(QList<QPointF>{{0.0, range_y.first}, {0.0, range_y.second}});

    const QPen gpen = guidePen(currentTheme());
    hor->setPen(gpen);
    ver->setPen(gpen);

    detail::attachToAxes(chart, hor);
    detail::attachToAxes(chart, ver);
    hide_legend_marker(chart, hor);
    hide_legend_marker(chart, ver);
}

void removeAllSeries(QChart* chart) {
    if (!chart)
        return;
    const auto all = chart->series();
    for (auto* series : all) {
        if (detail::isGuideSeries(series->name()))
            continue;
        chart->removeSeries(series);
        delete series;
    }
}

void updateAxes(QChart* chart, const Pair& range_x, const Pair& range_y, GridMode mode, bool snap_x, bool snap_y) {
    if (!chart)
        return;
    auto* axis_x = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0, nullptr));
    auto* axis_y = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0, nullptr));
    if (!axis_x || !axis_y)
        return;

    {
        const QSignalBlocker bx(axis_x);
        const QSignalBlocker by(axis_y);
        if (axis_x->min() != range_x.first || axis_x->max() != range_x.second)
            axis_x->setRange(range_x.first, range_x.second);
        if (axis_y->min() != range_y.first || axis_y->max() != range_y.second)
            axis_y->setRange(range_y.first, range_y.second);
        axes_detail::apply_axis_style(axis_x, mode, snap_x);
        axes_detail::apply_axis_style(axis_y, mode, snap_y);
        const ChartTheme theme = currentTheme();
        applyAxisTheme(axis_x, theme);
        applyAxisTheme(axis_y, theme);
    }

    updateOriginGuides(chart, {axis_x->min(), axis_x->max()}, {axis_y->min(), axis_y->max()});
    axes_detail::attach_all_series(chart, axis_x, axis_y);
}
} // namespace chart_utils
