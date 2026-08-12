#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

namespace chart_utils {
/// Heckbert «Nice Numbers for Graph Labels» (Graphics Gems, 1990).
/// 1 / 2 / 5 × 10^k.
/// round=true → nearest; false → ceiling (для оценки span).
[[nodiscard]] inline double niceNumber(double value, bool round) noexcept {
    if (!std::isfinite(value) || value == 0.0)
        return 0.0;
    const double ax   = std::abs(value);
    const double exp  = std::floor(std::log10(ax));
    const double base = std::pow(10.0, exp);
    const double f    = ax / base; // [1, 10)

    double nf = 10.0;
    if (round) {
        if (f < 1.5)
            nf = 1.0;
        else if (f < 3.0)
            nf = 2.0;
        else if (f < 7.0)
            nf = 5.0;
        else
            nf = 10.0;
    }
    else {
        if (f <= 1.0)
            nf = 1.0;
        else if (f <= 2.0)
            nf = 2.0;
        else if (f <= 5.0)
            nf = 5.0;
        else
            nf = 10.0;
    }
    return nf * base;
}

namespace detail {
[[nodiscard]] inline double snap_tick(double v, double step) noexcept {
    if (!std::isfinite(v))
        return 0.0;
    if (std::abs(v) < 1e-12 * std::max(1.0, step))
        return 0.0;
    return v;
}

/// Drop tiny sign-crossing noise so Y does not flip between [0,h] and [−ε,h] while tuning.
/// Real undershoot/overshoot (≥ ~2% of opposite peak) is kept.
inline void suppress_sign_noise(double& min_v, double& max_v) noexcept {
    if (!std::isfinite(min_v) || !std::isfinite(max_v))
        return;
    if (!(max_v > 0.0 && min_v < 0.0))
        return;

    constexpr double k_rel = 0.02;
    constexpr double k_abs = 1e-12;
    const double thr_lo    = std::max(k_abs, k_rel * max_v);
    const double thr_hi    = std::max(k_abs, k_rel * (-min_v));
    if (-min_v <= thr_lo)
        min_v = 0.0;
    else if (max_v <= thr_hi)
        max_v = 0.0;
}
} // namespace detail

/// Bounds from data only (no 1–2–5 snap, no % pad). For independent X (t, ω).
[[nodiscard]] inline std::pair<double, double> dataAxisRange(double min_v, double max_v,
                                                             bool include_zero = true) noexcept {
    if (!std::isfinite(min_v) || !std::isfinite(max_v))
        return {-1.0, 1.0};

    if (include_zero)
        detail::suppress_sign_noise(min_v, max_v);

    const double data_min = min_v;
    const double data_max = max_v;

    if (include_zero) {
        min_v = std::min(min_v, 0.0);
        max_v = std::max(max_v, 0.0);
    }

    if (!(max_v > min_v)) {
        const double c = min_v;
        const double a = std::max(1.0, std::abs(c) * 0.1);
        min_v          = c - a;
        max_v          = c + a;
        if (include_zero) {
            min_v = std::min(min_v, 0.0);
            max_v = std::max(max_v, 0.0);
        }
    }

    if (include_zero) {
        if (data_min >= 0.0)
            min_v = 0.0;
        if (data_max <= 0.0)
            max_v = 0.0;
    }
    if (!(max_v > min_v))
        max_v = min_v + 1.0;
    return {min_v, max_v};
}

/// Expand range with ~2% pad and optional origin; no 1–2–5 snap of bounds (value axes).
[[nodiscard]] inline std::pair<double, double> paddedAxisRange(double min_v, double max_v,
                                                               bool include_zero = true) noexcept {
    const auto base    = dataAxisRange(min_v, max_v, include_zero);
    const double lo0   = base.first;
    const double hi0   = base.second;
    const double span0 = hi0 - lo0;
    const double pad   = 0.02 * span0;
    double lo          = lo0;
    double hi          = hi0;
    // Do not pad past a one-sided zero edge.
    if (!(include_zero && lo0 == 0.0))
        lo -= pad;
    if (!(include_zero && hi0 == 0.0))
        hi += pad;
    if (include_zero) {
        lo = std::min(lo, 0.0);
        hi = std::max(hi, 0.0);
        if (lo0 == 0.0)
            lo = 0.0;
        if (hi0 == 0.0)
            hi = 0.0;
    }
    if (!(hi > lo))
        hi = lo + 1.0;
    return {lo, hi};
}

/// Expand [min,max] outward to nice tick multiples; optionally force-include 0.
/// No % pad here — pad + snap was expanding twice (e.g. 100 → 120). Example: [0, 155.456] → [0, 160].
[[nodiscard]] inline std::pair<double, double> niceAxisRange(double min_v, double max_v,
                                                             bool include_zero = true) noexcept {
    // Remember one-sided data (pin lo/hi to 0 after floor/ceil).
    const bool data_nonneg = std::isfinite(min_v) && min_v >= 0.0;
    const bool data_nonpos = std::isfinite(max_v) && max_v <= 0.0;

    auto [lo0, hi0] = dataAxisRange(min_v, max_v, include_zero);

    const double span = hi0 - lo0;
    // Denser than label count so outer bounds stay tight (155 → 160, not 200).
    double step = niceNumber(span / 6.0, /*round=*/true);
    if (!(step > 0.0))
        step = 1.0;

    double lo = std::floor(lo0 / step) * step;
    double hi = std::ceil(hi0 / step) * step;
    lo        = detail::snap_tick(lo, step);
    hi        = detail::snap_tick(hi, step);

    if (include_zero) {
        lo = std::min(lo, 0.0);
        hi = std::max(hi, 0.0);
        if (data_nonneg)
            lo = 0.0;
        if (data_nonpos)
            hi = 0.0;
    }
    if (!(hi > lo))
        hi = lo + step;

    return {lo, hi};
}

/// Major tick step for ~`major_ticks` labels across span (default 5 → 4 intervals).
[[nodiscard]] inline double niceTickStep(double span, int major_ticks = 5) noexcept {
    if (!std::isfinite(span) || !(span > 0.0))
        return 1.0;
    const int intervals = major_ticks > 1 ? major_ticks - 1 : 1;
    double step         = niceNumber(span / static_cast<double>(intervals), /*round=*/true);
    if (!(step > 0.0))
        step = 1.0;
    return step;
}
} // namespace chart_utils
