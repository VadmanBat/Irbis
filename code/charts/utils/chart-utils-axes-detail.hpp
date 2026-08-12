#pragma once

#include "code/charts/utils/chart-utils-detail.hpp"
#include "code/charts/utils/chart-utils.hpp"
#include "code/charts/utils/nice-axis.hpp"

#include <algorithm>
#include <cmath>
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

inline void apply_tab_grid_exact(QValueAxis* axis) {
    if (axis->tickType() != QValueAxis::TicksFixed)
        axis->setTickType(QValueAxis::TicksFixed);
    if (axis->tickCount() != kMajorTicks)
        axis->setTickCount(kMajorTicks);
    force_grid_on(axis);
}

/// Fixed ticks on a 1–2–5 lattice through 0 so grid crosses origin.
inline void apply_tab_grid_snap(QValueAxis* axis) {
    const double data_lo = axis->min();
    const double data_hi = axis->max();
    if (!std::isfinite(data_lo) || !std::isfinite(data_hi) || !(data_hi > data_lo))
        return;

    const double span     = data_hi - data_lo;
    const bool nonneg     = data_lo >= 0.0;
    const bool nonpos     = data_hi <= 0.0;
    const bool cross_zero = data_lo < 0.0 && data_hi > 0.0;

    double best_lo   = data_lo;
    double best_hi   = data_hi;
    int best_count   = kMajorTicks;
    double best_cost = 1e300;
    const int target = kMajorTicks > 1 ? kMajorTicks - 1 : 1;

    for (int intervals = 3; intervals <= 8; ++intervals) {
        double step = niceNumber(span / static_cast<double>(intervals), /*round=*/true);
        if (!(step > 0.0))
            continue;

        double lo = data_lo;
        double hi = data_hi;
        if (cross_zero) {
            const int n_neg = std::max(1, static_cast<int>(std::ceil((-data_lo) / step - 1e-15)));
            const int n_pos = std::max(1, static_cast<int>(std::ceil(data_hi / step - 1e-15)));
            lo              = -static_cast<double>(n_neg) * step;
            hi              = static_cast<double>(n_pos) * step;
        }
        else if (nonneg) {
            lo              = 0.0;
            const int n_pos = std::max(1, static_cast<int>(std::ceil(data_hi / step - 1e-15)));
            hi              = static_cast<double>(n_pos) * step;
        }
        else if (nonpos) {
            hi              = 0.0;
            const int n_neg = std::max(1, static_cast<int>(std::ceil((-data_lo) / step - 1e-15)));
            lo              = -static_cast<double>(n_neg) * step;
        }
        else {
            lo = detail::snap_tick(std::floor(data_lo / step) * step, step);
            hi = detail::snap_tick(std::ceil(data_hi / step) * step, step);
        }

        lo = detail::snap_tick(lo, step);
        hi = detail::snap_tick(hi, step);
        if (!(hi > lo))
            hi = lo + step;

        const int n = static_cast<int>(std::lround((hi - lo) / step));
        if (n < 1)
            continue;
        hi = lo + static_cast<double>(n) * step;

        const double expand = (hi - lo) - span;
        const double cost   = expand * 1000.0 + std::abs(n - target);
        if (cost < best_cost) {
            best_cost  = cost;
            best_lo    = lo;
            best_hi    = hi;
            best_count = n + 1;
        }
    }

    axis->setTickType(QValueAxis::TicksFixed);
    axis->setTickCount(best_count);
    axis->setRange(best_lo, best_hi);
    force_grid_on(axis);
}

/// Viewer: refresh 1–2–5 tick step after pan/zoom (does not force grid on).
inline void apply_viewer_grid(QValueAxis* axis) {
    const double step = niceTickStep(axis->max() - axis->min(), kMajorTicks);
    if (axis->tickType() != QValueAxis::TicksDynamic)
        axis->setTickType(QValueAxis::TicksDynamic);
    if (axis->tickAnchor() != 0.0)
        axis->setTickAnchor(0.0);
    if (std::abs(axis->tickInterval() - step) > 1e-12 * std::max(1.0, step))
        axis->setTickInterval(step);
    ensure_minor_ticks(axis);
}

inline void apply_axis_style(QValueAxis* axis, GridMode mode, bool snap) {
    if (mode == GridMode::Viewer)
        apply_viewer_grid(axis);
    else if (snap)
        apply_tab_grid_snap(axis);
    else
        apply_tab_grid_exact(axis);
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
