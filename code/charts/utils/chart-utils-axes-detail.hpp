#pragma once

#include "code/charts/utils/chart-utils.hpp"
#include "code/charts/utils/nice-axis.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QString>
#include <QValueAxis>

namespace chart_utils {
namespace axes_detail {
inline void ensure_minor_ticks(QValueAxis* axis) {
    if (axis->minorTickCount() != 1)
        axis->setMinorTickCount(1);
}

/// Tab charts always show grid. Viewer may hide it — do not use there.
inline void force_grid_on(QValueAxis* axis) {
    if (!axis->isGridLineVisible())
        axis->setGridLineVisible(true);
    if (!axis->isMinorGridLineVisible())
        axis->setMinorGridLineVisible(true);
    ensure_minor_ticks(axis);
}

/// Qt Charts TicksDynamic: first = anchor − floor((anchor−min)/step)·step,
/// then value += step while value <= max (chartaxiselement.cpp).
/// TicksFixed interpolates min + i·(max−min)/(n−1) and cannot keep 0 exact
/// unless 0 is an endpoint. We only use Dynamic: lattice k·step through 0.
///
/// Labels: format from the step (`%.Nf`), same as matplotlib ScalarFormatter.
/// `%.6g` would print the IEEE leftover of `value += step` as 2.22e-16.

inline QString label_format_for_range(double lo, double hi, double step) {
    // Leading '%' must be a printf mark, not a QString::arg slot (`%1.%2f` → `3.0f`).
    return QLatin1Char('%') + QString::number(labelFieldWidth(lo, hi, step)) + QLatin1Char('.')
         + QString::number(labelDecimals(step)) + QLatin1Char('f');
}

inline void pin_zero_edges(double& lo, double& hi) noexcept {
    const double span = hi - lo;
    const double eps =
        1e-12 * std::max(1.0, std::max(span, std::max(std::abs(lo), std::abs(hi))));
    if (std::abs(lo) <= eps)
        lo = 0.0;
    if (std::abs(hi) <= eps)
        hi = 0.0;
}

/// Anchor 0 so the origin tick is the real 0. Far from 0, park at the first
/// in-window multiple — otherwise Qt walks 10^n ticks from the origin.
inline double tick_anchor(double lo, double step) noexcept {
    if (!(step > 0.0) || !std::isfinite(step))
        return 0.0;
    const double steps_from_zero = lo / step;
    constexpr double k_max_walk  = 64.0;
    if (!std::isfinite(steps_from_zero) || std::abs(steps_from_zero) > k_max_walk) {
        const double n = std::floor(lo / step);
        return std::isfinite(n) ? n * step : lo;
    }
    return 0.0;
}

/// Interval first, then range: a leftover tiny interval + a wide window freezes Qt.
inline void apply_dynamic_grid(QValueAxis* axis, double lo, double hi, bool adjust_range) {
    if (!axis || !std::isfinite(lo) || !std::isfinite(hi) || !(hi > lo))
        return;

    if (adjust_range) {
        pin_zero_edges(lo, hi);
        if (!(hi > lo))
            hi = lo + 1.0;
    }

    const double span = hi - lo;
    // 1–2–5 inside the window — do not force a tick on max (T = 200 → 200.5
    // would relabel the right edge and Qt would shrink the plot).
    double step = niceTickStepIn(lo, hi, kMajorTicks);
    if (!(step > 0.0) || !std::isfinite(step))
        step = span;

    // Closed high edge at 0: Qt tests `value <= max` after `value += step`.
    // Same idea as matplotlib `arange(vmin, vmax + 0.5*step)` — keep the 0 tick.
    if (adjust_range && hi == 0.0 && lo < 0.0)
        hi = std::abs(lo) * 16.0 * std::numeric_limits<double>::epsilon();

    const double anchor = tick_anchor(lo, step);
    const QString fmt   = label_format_for_range(lo, hi, step);

    if (axis->tickType() != QValueAxis::TicksDynamic)
        axis->setTickType(QValueAxis::TicksDynamic);
    axis->setTickInterval(step);
    axis->setTickAnchor(anchor);
    if (axis->labelFormat() != fmt)
        axis->setLabelFormat(fmt);
    if (axis->min() != lo || axis->max() != hi)
        axis->setRange(lo, hi);
    ensure_minor_ticks(axis);
}

inline void apply_tab_grid(QValueAxis* axis, double lo, double hi, bool /*snap*/) {
    apply_dynamic_grid(axis, lo, hi, /*adjust_range=*/true);
    force_grid_on(axis);
}

inline void apply_viewer_grid(QValueAxis* axis, double lo, double hi) {
    apply_dynamic_grid(axis, lo, hi, /*adjust_range=*/false);
}

inline void apply_viewer_grid(QValueAxis* axis) {
    if (axis)
        apply_viewer_grid(axis, axis->min(), axis->max());
}

inline void attach_all_series(QChart* chart, QValueAxis* axis_x, QValueAxis* axis_y) {
    for (auto* series : chart->series()) {
        if (!series->attachedAxes().contains(axis_x))
            series->attachAxis(axis_x);
        if (!series->attachedAxes().contains(axis_y))
            series->attachAxis(axis_y);
    }
}
} // namespace axes_detail
} // namespace chart_utils
