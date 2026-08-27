#include "code/charts/utils/chart-utils-detail.hpp"
#include "code/charts/utils/chart-utils.hpp"

#include <QAreaSeries>
#include <QBrush>
#include <QFile>
#include <QLegend>
#include <QLegendMarker>
#include <QLineSeries>
#include <QTextStream>
#include <QXYSeries>
#include <utility>

namespace chart_utils {
namespace {
void mount_area_series(QChart* chart, QAreaSeries* area) {
    auto* up = area->upperSeries();
    auto* lo = area->lowerSeries();
    // QAreaSeries::attachAxis forwards to the bounds; they must already be on the chart.
    if (up && !chart->series().contains(up))
        chart->addSeries(up);
    if (lo && !chart->series().contains(lo))
        chart->addSeries(lo);
    if (!chart->series().contains(area))
        chart->addSeries(area);
    if (up) {
        up->setPen(Qt::NoPen);
        detail::attachToAxes(chart, up);
    }
    if (lo) {
        lo->setPen(Qt::NoPen);
        detail::attachToAxes(chart, lo);
    }
    detail::attachToAxes(chart, area);
    if (QLegend* legend = chart->legend()) {
        if (up)
            for (auto* marker : legend->markers(up))
                marker->setVisible(false);
        if (lo)
            for (auto* marker : legend->markers(lo))
                marker->setVisible(false);
    }
}
}

SeriesWrite addRealSeries(QChart* chart, const VecPair& points, const QString& title, std::size_t index) {
    if (!chart || points.empty())
        return {};
    auto pb      = detail::toPointsWithBounds(points);
    auto* series = new QLineSeries;
    series->setName(title);
    series->setPen(penForIndex(index));
    series->replace(std::move(pb.points));
    chart->addSeries(series);
    detail::attachToAxes(chart, series);
    return {true, pb.bounds};
}

SeriesWrite addBandSeries(QChart* chart, const VecPair& lower, const VecPair& upper, const QString& title) {
    if (!chart || lower.empty() || upper.empty())
        return {};
    auto lo_pb = detail::toPointsWithBounds(lower);
    auto hi_pb = detail::toPointsWithBounds(upper);
    auto* lo   = new QLineSeries;
    auto* hi   = new QLineSeries;
    lo->replace(std::move(lo_pb.points));
    hi->replace(std::move(hi_pb.points));

    auto* area = new QAreaSeries(hi, lo);
    area->setName(title);
    const QColor stroke = penForIndex(0).color();
    QColor fill         = stroke;
    fill.setAlpha(isDarkTheme() ? 55 : 40);
    area->setBrush(QBrush(fill));
    QPen outline(stroke, 1.0, Qt::DashLine);
    outline.setCosmetic(true);
    area->setPen(outline);
    mount_area_series(chart, area);

    AxisBounds b = lo_pb.bounds;
    if (hi_pb.bounds.valid) {
        if (!b.valid)
            b = hi_pb.bounds;
        else {
            if (hi_pb.bounds.min_x < b.min_x)
                b.min_x = hi_pb.bounds.min_x;
            if (hi_pb.bounds.max_x > b.max_x)
                b.max_x = hi_pb.bounds.max_x;
            if (hi_pb.bounds.min_y < b.min_y)
                b.min_y = hi_pb.bounds.min_y;
            if (hi_pb.bounds.max_y > b.max_y)
                b.max_y = hi_pb.bounds.max_y;
        }
    }
    return {true, b};
}

SeriesWrite addComplexSeries(QChart* chart, const VecComp& points, const QString& title, std::size_t index) {
    if (!chart || points.empty())
        return {};
    auto pb      = detail::toPointsWithBounds(points);
    auto* series = new QLineSeries;
    series->setName(title);
    series->setPen(penForIndex(index));
    series->replace(std::move(pb.points));
    chart->addSeries(series);
    detail::attachToAxes(chart, series);
    return {true, pb.bounds};
}

SeriesWrite replaceLastRealSeries(QChart* chart, const VecPair& points, const QString& title) {
    auto* series = detail::lastDataSeries(chart);
    if (!series)
        return {};
    series->setName(title);
    if (points.empty()) {
        series->clear();
        return {true, {}};
    }
    auto pb = detail::toPointsWithBounds(points);
    series->replace(std::move(pb.points));
    return {true, pb.bounds};
}

SeriesWrite replaceLastComplexSeries(QChart* chart, const VecComp& points, const QString& title) {
    auto* series = detail::lastDataSeries(chart);
    if (!series)
        return {};
    series->setName(title);
    if (points.empty()) {
        series->clear();
        return {true, {}};
    }
    auto pb = detail::toPointsWithBounds(points);
    series->replace(std::move(pb.points));
    return {true, pb.bounds};
}

bool removeLastDataSeries(QChart* chart) {
    auto* series = detail::lastDataSeries(chart);
    if (!series)
        return false;
    chart->removeSeries(series);
    delete series;
    return true;
}

bool saveChartToFile(const QString& fileName, QChart* chart) {
    if (!chart)
        return false;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    for (QAbstractSeries* series : chart->series()) {
        if (isAccessorySeries(chart, series))
            continue;
        out << "Name: " << series->name() << '\n';
        if (auto* xy = qobject_cast<QXYSeries*>(series)) {
            const int n = xy->count();
            for (int i = 0; i < n; ++i)
                out << xy->at(i).x() << ", " << xy->at(i).y() << '\n';
        }
        out << '\n';
    }
    return true;
}
} // namespace chart_utils
