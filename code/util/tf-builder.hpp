#pragma once

#include "code/model/model-param.hpp"
#include "numina/classes/control/delayed-plant.h"
#include "numina/classes/control/transfer-function.h"
#include "numina/classes/control/transfer-function/quality-report.h"
#include "numina/classes/control/transfer-function/response-lab.h"
#include "numina/classes/polynomial/polynomial.h"
#include "numina/core/space.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <numbers>
#include <utility>
#include <vector>

namespace tf_builder {
using VecPair = numina::ResponseLab::VecPair;
using VecComp = numina::ResponseLab::VecComp;

/// One frequency sweep shared by КЧХ / АЧХ / ФЧХ.
struct FrequencyBundle {
    VecComp nyquist;   ///< W(jω) on complex plane
    VecPair amplitude; ///< (ω, |W(jω)|)
    VecPair phase;     ///< (ω, arg W in degrees)
};

inline numina::Polynomial poly(std::vector<double> coeffs) {
    if (coeffs.empty())
        coeffs = {0.0};
    return numina::Polynomial(std::move(coeffs));
}

inline numina::TransferFunction plant(std::vector<double> num, std::vector<double> den, double tau = 0.0,
                                      int order = 6) {
    if (tau == 0.0)
        return numina::TransferFunction(poly(std::move(num)), poly(std::move(den)));
    return numina::TransferFunction{
        numina::TransferFunction(poly(std::move(num)), poly(std::move(den))),
        numina::TransferFunction::makeDelay(tau, static_cast<std::uint8_t>(order)),
    };
}

/// W₀, or W₀·Padé(τ) when settings ask for a rational delay.
inline numina::TransferFunction withDelay(const numina::TransferFunction& w0, double tau, const ModelParam& p) {
    if (!(tau > 0.0) || !p.usePadeApprox)
        return w0;
    return numina::DelayedPlant(w0, tau).pade(static_cast<std::uint8_t>(p.approxOrder));
}

inline void apply_delayed_time_samples(VecPair& pts, const numina::DelayedPlant& plant, bool shift_axis) {
    const double tau = plant.delay();
    if (!(tau > 0.0) || pts.empty())
        return;
    if (shift_axis) {
        for (auto& pt : pts)
            pt.first += tau;
        if (pts.front().first > 0.0)
            pts.insert(pts.begin(), {0.0, 0.0});
    }
    for (auto& pt : pts)
        pt.second = plant.transientResponse(pt.first);
}

inline void apply_delayed_impulse_samples(VecPair& pts, const numina::DelayedPlant& plant, bool shift_axis) {
    const double tau = plant.delay();
    if (!(tau > 0.0) || pts.empty())
        return;
    if (shift_axis) {
        for (auto& pt : pts)
            pt.first += tau;
        if (pts.front().first > 0.0)
            pts.insert(pts.begin(), {0.0, 0.0});
    }
    for (auto& pt : pts)
        pt.second = plant.impulseResponse(pt.first);
}

inline void shiftTimeByDelay(VecPair& pts, double tau) {
    if (!(tau > 0.0) || pts.empty())
        return;
    for (auto& pt : pts)
        pt.first += tau;
    if (pts.front().first > 0.0)
        pts.insert(pts.begin(), {0.0, 0.0});
}

inline void applyExactFreqDelay(FrequencyBundle& bundle, double tau) {
    if (!(tau > 0.0))
        return;
    constexpr double rad2deg = 180.0 / std::numbers::pi;
    const std::size_t n      = bundle.nyquist.size();
    for (std::size_t i = 0; i < n; ++i) {
        const double w = (i < bundle.amplitude.size()) ? bundle.amplitude[i].first : 0.0;
        if (!(w > 0.0))
            continue;
        bundle.nyquist[i] *= std::exp(std::complex<double>(0.0, -w * tau));
        if (i < bundle.phase.size())
            bundle.phase[i].second -= w * tau * rad2deg;
    }
}

inline numina::TransferFunction closedLoop(std::vector<double> plantNum, std::vector<double> plantDen,
                                           std::vector<double> regNum, std::vector<double> regDen, double tau = 0.0,
                                           int order = 6) {
    const auto pade = static_cast<std::uint8_t>(order);
    if (tau != 0.0) {
        return numina::TransferFunction::closed({
            {poly(std::move(plantNum)), poly(std::move(plantDen))},
            {poly(std::move(regNum)), poly(std::move(regDen))},
            numina::TransferFunction::makeDelayPair(tau, pade),
        });
    }
    return numina::TransferFunction::closed({
        {poly(std::move(plantNum)), poly(std::move(plantDen))},
        {poly(std::move(regNum)), poly(std::move(regDen))},
    });
}

inline std::pair<double, double> timeRange(const ModelParam& p) {
    double t0 = p.timeMin;
    double t1 = p.timeMax;
    if (!(t1 > t0))
        std::swap(t0, t1);
    if (!(t1 > t0))
        t1 = t0 + 1.0;
    return {t0, t1};
}

inline VecPair transient(numina::ResponseLab& lab, const ModelParam& p) {
    if (p.autoTimeRange)
        return lab.transient();
    const auto range = timeRange(p);
    if (p.autoTimeIntervals)
        return lab.transient(range);
    return lab.transient(range, static_cast<std::size_t>(std::max(2, p.timeIntervals)));
}

inline VecPair impulse(numina::ResponseLab& lab, const ModelParam& p) {
    if (p.autoTimeRange)
        return lab.impulse();
    const auto range = timeRange(p);
    if (p.autoTimeIntervals)
        return lab.impulse(range);
    return lab.impulse(range, static_cast<std::size_t>(std::max(2, p.timeIntervals)));
}

/// Lab samples W₀; exact τ via DelayedPlant (ResponseLab cannot bind it yet).
inline VecPair transient(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    if (tau > 0.0 && p.usePadeApprox) {
        const auto delayed = withDelay(tf, tau, p);
        numina::ResponseLab lab(delayed);
        return transient(lab, p);
    }
    numina::ResponseLab lab(tf);
    auto pts = transient(lab, p);
    if (tau > 0.0)
        apply_delayed_time_samples(pts, numina::DelayedPlant(tf, tau), p.autoTimeRange);
    return pts;
}

inline VecPair impulse(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    if (tau > 0.0 && p.usePadeApprox) {
        const auto delayed = withDelay(tf, tau, p);
        numina::ResponseLab lab(delayed);
        return impulse(lab, p);
    }
    numina::ResponseLab lab(tf);
    auto pts = impulse(lab, p);
    if (tau > 0.0)
        apply_delayed_impulse_samples(pts, numina::DelayedPlant(tf, tau), p.autoTimeRange);
    return pts;
}

/// True when free term of denominator is ~0 (pole at s=0 / free integrator) — avoid ω=0.
inline bool hasZeroDenConstant(const numina::TransferFunction& tf) noexcept {
    const auto& den = tf.denominator();
    const int deg   = den.degree();
    if (deg < 0)
        return true;
    // coeffs: [0]=leading … [deg]=free term — no coeffs() copy.
    const double lead = den[0];
    const double free = tf.denominatorConstant();
    return std::abs(free) <= 1e-14 * (1.0 + std::abs(lead));
}

/// КЧХ + АЧХ + ФЧХ — log ω-grid. tau>0: DelayedPlant::frequencyResponse (lab stays on W₀).
inline FrequencyBundle frequencyBundle(numina::ResponseLab& lab, const ModelParam& p, double tau = 0.0) {
    const numina::TransferFunction& tf = lab.tf();
    const numina::DelayedPlant plant(tf, tau);
    std::pair<double, double> range    = p.autoFreqRange ? lab.frequencyRange() : std::make_pair(p.freqMin, p.freqMax);

    if (hasZeroDenConstant(tf)) {
        constexpr double w_min_floor = 1e-4;
        range.first                  = std::max(range.first, w_min_floor);
        if (!(range.second > range.first))
            range.second = range.first * 1e3;
    }
    if (!(range.first > 0.0))
        range.first = 1e-4;
    if (!(range.second > range.first))
        range.second = range.first * 1e3;

    const std::size_t n = p.autoFreqIntervals ? 120 : static_cast<std::size_t>(std::max(2, p.freqIntervals));

    const std::vector<double> omegas = numina::core::logspace(range, n, /*from_scratch=*/true);

    FrequencyBundle out;
    out.nyquist.reserve(omegas.size());
    out.amplitude.reserve(omegas.size());
    out.phase.reserve(omegas.size());

    constexpr double rad2deg = 180.0 / std::numbers::pi;
    double prev_deg          = 0.0;
    bool have_prev           = false;
    for (const double w : omegas) {
        if (!(w > 0.0))
            continue;
        const auto W = plant.frequencyResponse({0.0, w});
        out.nyquist.push_back(W);
        out.amplitude.emplace_back(w, std::abs(W));

        // Continuous phase (unwrap ±180° jumps from principal arg).
        double deg = std::arg(W) * rad2deg;
        if (have_prev) {
            while (deg - prev_deg > 180.0)
                deg -= 360.0;
            while (deg - prev_deg < -180.0)
                deg += 360.0;
        }
        have_prev = true;
        prev_deg  = deg;
        out.phase.emplace_back(w, deg);
    }
    return out;
}

inline FrequencyBundle frequencyBundle(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    if (tau > 0.0 && p.usePadeApprox) {
        const auto delayed = withDelay(tf, tau, p);
        numina::ResponseLab lab(delayed);
        return frequencyBundle(lab, p);
    }
    numina::ResponseLab lab(tf);
    return frequencyBundle(lab, p, tau);
}

inline numina::QualityReport quality(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    if (tau > 0.0 && p.usePadeApprox) {
        const auto delayed = withDelay(tf, tau, p);
        numina::ResponseLab lab(delayed);
        return lab.evaluate();
    }
    numina::ResponseLab lab(tf);
    auto q = lab.evaluate();
    if (tau > 0.0) {
        q.settling_time += tau;
        q.rise_time += tau;
        q.peak_time += tau;
    }
    return q;
}

inline bool validInput(const std::vector<double>& num, const std::vector<double>& den) {
    if (den.empty() || den.size() == 1)
        return false;
    if (num.size() > den.size())
        return false;
    return true;
}
} // namespace tf_builder
