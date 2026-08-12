#pragma once

#include "code/model/model-param.hpp"
#include "numina/classes/control/transfer-function.h"
#include "numina/classes/control/transfer-function/response-lab.h"
#include "numina/classes/polynomial/polynomial.h"
#include "numina/core/space.hpp"

#include <algorithm>
#include <cmath>
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

inline numina::TransferFunction closedLoop(std::vector<double> plantNum, std::vector<double> plantDen,
                                           std::vector<double> regNum, std::vector<double> regDen, double tau = 0.0,
                                           int order = 6) {
    numina::TransferFunction open{
        numina::TransferFunction(poly(std::move(plantNum)), poly(std::move(plantDen))),
        numina::TransferFunction(poly(std::move(regNum)), poly(std::move(regDen))),
    };
    if (tau != 0.0)
        open *= numina::TransferFunction::makeDelay(tau, static_cast<std::uint8_t>(order));
    open.closeLoop();
    return open;
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

inline VecPair transient(const numina::TransferFunction& tf, const ModelParam& p) {
    numina::ResponseLab lab(tf);
    return transient(lab, p);
}

inline VecPair impulse(const numina::TransferFunction& tf, const ModelParam& p) {
    numina::ResponseLab lab(tf);
    return impulse(lab, p);
}

/// True when free term of denominator is ~0 (pole at s=0 / free integrator) — avoid ω=0.
inline bool hasZeroDenConstant(const numina::TransferFunction& tf) noexcept {
    const auto& den = tf.getDenominator();
    const int deg   = den.degree();
    if (deg < 0)
        return true;
    // coeffs: [0]=leading … [deg]=free term — no vector() copy.
    const double lead = den[0];
    const double free = den[static_cast<std::size_t>(deg)];
    return std::abs(free) <= 1e-14 * (1.0 + std::abs(lead));
}

/// КЧХ + АЧХ + ФЧХ — always logarithmic ω-grid (one ResponseLab / one TF evaluation path).
inline FrequencyBundle frequencyBundle(numina::ResponseLab& lab, const ModelParam& p) {
    const numina::TransferFunction& tf = lab.tf();
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
        const auto W = tf.frequencyResponse({0.0, w});
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

inline FrequencyBundle frequencyBundle(const numina::TransferFunction& tf, const ModelParam& p) {
    numina::ResponseLab lab(tf);
    return frequencyBundle(lab, p);
}

inline bool validInput(const std::vector<double>& num, const std::vector<double>& den) {
    if (den.empty() || den.size() == 1)
        return false;
    if (num.size() > den.size())
        return false;
    return true;
}
} // namespace tf_builder
