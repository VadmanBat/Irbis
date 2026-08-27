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

/// Next smaller 1–2–5 step: 5→2→1→0.5.
[[nodiscard]] inline double finerNiceStep(double step) noexcept {
    if (!std::isfinite(step) || !(step > 0.0))
        return 1.0;
    const double exp  = std::floor(std::log10(step));
    const double base = std::pow(10.0, exp);
    const double mant = step / base;
    if (mant > 3.0)
        return 2.0 * base;
    if (mant > 1.5)
        return 1.0 * base;
    return 5.0 * (base / 10.0);
}

/// How many k·step ticks lie in [lo, hi] (closed).
[[nodiscard]] inline int tickCountInRange(double lo, double hi, double step) noexcept {
    if (!(step > 0.0) || !std::isfinite(step) || !std::isfinite(lo) || !std::isfinite(hi) || !(hi > lo))
        return 0;
    const double first = std::ceil(lo / step - 1e-12);
    const double last  = std::floor(hi / step + 1e-12);
    if (last < first)
        return 0;
    return static_cast<int>(last - first) + 1;
}

/// 1–2–5 step for a window: ~`major_ticks`, but not fewer than `min_ticks`.
/// Heckbert nearest can jump 0.3→0.5 and leave h(t)∈[0, 1.2] with only 0 / 0.5 / 1.
[[nodiscard]] inline double niceTickStepIn(double lo, double hi, int major_ticks = 5, int min_ticks = 5) noexcept {
    const double span       = hi - lo;
    double step             = niceTickStep(span, major_ticks);
    const double floor_step = span / 12.0;
    while (tickCountInRange(lo, hi, step) < min_ticks && step > floor_step) {
        const double finer = finerNiceStep(step);
        if (!(finer < step) || !(finer > 0.0))
            break;
        step = finer;
    }
    return step;
}

/// Digits after the point so `k·step` prints cleanly (Heckbert / matplotlib).
/// 1–2–5 keep `−log10(step)` digits; 2.5 needs one more (`2.5` not `2`).
[[nodiscard]] inline int labelDecimals(double step) noexcept {
    if (!std::isfinite(step) || !(std::abs(step) > 0.0))
        return 0;
    const double ax   = std::abs(step);
    const double exp  = std::floor(std::log10(ax));
    const double mant = ax / std::pow(10.0, exp); // [1, 10)
    int d             = std::max(0, static_cast<int>(-exp));
    if (mant > 2.0000001 && mant < 4.9999999)
        ++d;
    return d > 10 ? 10 : d;
}

/// Minimum printf width so `200` and `200.5` occupy the same slot (`%5.1f`).
[[nodiscard]] inline int labelFieldWidth(double lo, double hi, double step) noexcept {
    const int dec     = labelDecimals(step);
    const double peak = std::max(std::abs(lo), std::abs(hi));
    int int_digits    = 1;
    if (std::isfinite(peak) && peak >= 10.0)
        int_digits = static_cast<int>(std::floor(std::log10(peak))) + 1;
    int w = int_digits + (dec > 0 ? dec + 1 : 0);
    if ((std::isfinite(lo) && lo < 0.0) || (std::isfinite(hi) && hi < 0.0))
        ++w;
    return w < 1 ? 1 : w;
}
} // namespace chart_utils
