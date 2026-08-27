#pragma once

#include "code/charts/utils/chart-utils.hpp"
#include "code/series/axis-bounds.hpp"

#include <QAreaSeries>
#include <QChart>
#include <QLineSeries>
#include <QList>
#include <QPointF>
#include <QValueAxis>

namespace chart_utils {
namespace detail {
struct PointsWithBounds {
    QList<QPointF> points;
    AxisBounds bounds;
};

[[nodiscard]] inline bool isGuideSeries(const QString& name) {
    return name == QLatin1String(kHorGuide) || name == QLatin1String(kVerGuide);
}

[[nodiscard]] inline bool isAreaEdge(const QChart* chart, const QAbstractSeries* series) {
    if (!chart || !series)
        return false;
    for (auto* s : chart->series()) {
        auto* area = qobject_cast<QAreaSeries*>(s);
        if (area && (area->upperSeries() == series || area->lowerSeries() == series))
            return true;
    }
    return false;
}

/// One pass: QPointF list + axis extents (avoids boundsOf + toPoints double scan).
[[nodiscard]] inline PointsWithBounds toPointsWithBounds(const VecPair& data) {
    PointsWithBounds out;
    if (data.empty())
        return out;
    out.points.reserve(static_cast<int>(data.size()));
    const auto& [x0, y0] = data.front();
    out.bounds.min_x = out.bounds.max_x = x0;
    out.bounds.min_y = out.bounds.max_y = y0;
    out.bounds.valid                    = true;
    out.points.append(QPointF(x0, y0));
    for (std::size_t i = 1; i < data.size(); ++i) {
        const auto& [x, y] = data[i];
        out.points.append(QPointF(x, y));
        if (x < out.bounds.min_x)
            out.bounds.min_x = x;
        if (x > out.bounds.max_x)
            out.bounds.max_x = x;
        if (y < out.bounds.min_y)
            out.bounds.min_y = y;
        if (y > out.bounds.max_y)
            out.bounds.max_y = y;
    }
    return out;
}

[[nodiscard]] inline PointsWithBounds toPointsWithBounds(const VecComp& data) {
    PointsWithBounds out;
    if (data.empty())
        return out;
    out.points.reserve(static_cast<int>(data.size()));
    const double x0  = data.front().real();
    const double y0  = data.front().imag();
    out.bounds.min_x = out.bounds.max_x = x0;
    out.bounds.min_y = out.bounds.max_y = y0;
    out.bounds.valid                    = true;
    out.points.append(QPointF(x0, y0));
    for (std::size_t i = 1; i < data.size(); ++i) {
        const double x = data[i].real();
        const double y = data[i].imag();
        out.points.append(QPointF(x, y));
        if (x < out.bounds.min_x)
            out.bounds.min_x = x;
        if (x > out.bounds.max_x)
            out.bounds.max_x = x;
        if (y < out.bounds.min_y)
            out.bounds.min_y = y;
        if (y > out.bounds.max_y)
            out.bounds.max_y = y;
    }
    return out;
}

[[nodiscard]] inline QLineSeries* lastDataSeries(QChart* chart) {
    if (!chart)
        return nullptr;
    const auto all      = chart->series();
    const auto n_series = all.size();
    for (auto i = n_series; i > 0; --i) {
        auto* s = qobject_cast<QLineSeries*>(all[i - 1]);
        if (!s || isGuideSeries(s->name()) || isAreaEdge(chart, s))
            continue;
        return s;
    }
    return nullptr;
}

inline void attachToAxes(QChart* chart, QAbstractSeries* series) {
    auto* axis_x = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0, nullptr));
    auto* axis_y = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0, nullptr));
    if (axis_x && !series->attachedAxes().contains(axis_x))
        series->attachAxis(axis_x);
    if (axis_y && !series->attachedAxes().contains(axis_y))
        series->attachAxis(axis_y);
}
} // namespace detail
} // namespace chart_utils
