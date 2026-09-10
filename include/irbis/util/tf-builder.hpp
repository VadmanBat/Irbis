#pragma once

#include "irbis/model/model-param.hpp"
#include "numina/classes/control/models/delayed-plant.h"
#include "numina/classes/control/models/quality-report.h"
#include "numina/classes/control/models/response-lab.h"
#include "numina/classes/control/models/transfer-function.h"
#include "numina/classes/polynomial/polynomial.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
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

/// Evaluate the step response at the given time stamps (exact e^{−τp} via DelayedPlant).
inline VecPair sampleTransientAt(const numina::TransferFunction& tf, const VecPair& times, double tau = 0.0) {
    VecPair out;
    out.reserve(times.size());
    if (tau > 0.0) {
        const numina::DelayedPlant plant(tf, tau);
        for (const auto& pt : times)
            out.emplace_back(pt.first, plant.transientResponse(pt.first));
        return out;
    }
    for (const auto& pt : times)
        out.emplace_back(pt.first, tf.transientResponse(pt.first));
    return out;
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

inline std::pair<double, double> freqRange(const ModelParam& p) {
    double w0 = p.freqMin;
    double w1 = p.freqMax;
    if (!(w1 > w0))
        std::swap(w0, w1);
    if (!(w1 > w0))
        w1 = (w0 > 0.0) ? w0 * 1e3 : 1.0;
    return {w0, w1};
}

/// True when free term of denominator is ~0 (pole at s=0 / free integrator) — avoid ω=0.
inline bool hasZeroDenConstant(const numina::TransferFunction& tf) noexcept {
    const auto& den = tf.denominator();
    const int deg   = den.degree();
    if (deg < 0)
        return true;
    const double lead = den[0];
    const double free = tf.denominatorConstant();
    return std::abs(free) <= 1e-14 * (1.0 + std::abs(lead));
}

/// Auto КЧХ starts at ω=0; D(0)=0 → W(j0) undefined. Range: pole cutoffs, lower by 3 decades.
inline std::pair<double, double> astaticFreqRange(const numina::TransferFunction& tf) {
    double w_lo = 0.0;
    double w_hi = 0.0;
    const auto consider = [&](double a) {
        if (!(a > 1e-12))
            return;
        if (!(w_lo > 0.0) || a < w_lo)
            w_lo = a;
        if (a > w_hi)
            w_hi = a;
    };
    for (const auto& e : tf.poles().first)
        consider(std::abs(e.first));
    for (const auto& e : tf.poles().second)
        consider(std::abs(e.first));
    if (!(w_hi > 0.0))
        return {1e-4, 100.0};
    const double w_min = w_lo / 1e3;
    return {w_min > 0.0 ? w_min : 1e-4, w_hi};
}

template <typename Fn>
auto with_lab(const numina::TransferFunction& tf, const ModelParam& p, double tau, Fn&& fn) {
    if (tau > 0.0 && p.usePadeApprox) {
        const auto delayed = withDelay(tf, tau, p);
        const numina::ResponseLab lab(delayed);
        return fn(lab);
    }
    if (tau > 0.0) {
        const numina::DelayedPlant plant(tf, tau);
        const numina::ResponseLab lab(plant);
        return fn(lab);
    }
    const numina::ResponseLab lab(tf);
    return fn(lab);
}

inline VecPair transient(const numina::ResponseLab& lab, const ModelParam& p) {
    if (p.autoTimeRange)
        return lab.transient();
    const auto range = timeRange(p);
    if (p.autoTimeIntervals)
        return lab.transient(range);
    return lab.transient(range, static_cast<std::size_t>(std::max(2, p.timeIntervals)));
}

inline VecPair impulse(const numina::ResponseLab& lab, const ModelParam& p) {
    if (p.autoTimeRange)
        return lab.impulse();
    const auto range = timeRange(p);
    if (p.autoTimeIntervals)
        return lab.impulse(range);
    return lab.impulse(range, static_cast<std::size_t>(std::max(2, p.timeIntervals)));
}

inline VecPair transient(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    return with_lab(tf, p, tau, [&](const numina::ResponseLab& lab) { return transient(lab, p); });
}

inline VecPair impulse(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    return with_lab(tf, p, tau, [&](const numina::ResponseLab& lab) { return impulse(lab, p); });
}

inline FrequencyBundle make_freq_bundle(VecComp nyquist, VecPair amplitude, VecPair phase) {
    FrequencyBundle out;
    const std::size_t n = std::min({nyquist.size(), amplitude.size(), phase.size()});
    out.nyquist.reserve(n);
    out.amplitude.reserve(n);
    out.phase.reserve(n);
    constexpr double rad2deg = 180.0 / std::numbers::pi;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& z    = nyquist[i];
        const double w   = amplitude[i].first;
        const double mag = amplitude[i].second;
        const double phi = phase[i].second;
        if (!std::isfinite(z.real()) || !std::isfinite(z.imag()) || !std::isfinite(w) || !std::isfinite(mag) ||
            !std::isfinite(phi))
            continue;
        out.nyquist.push_back(z);
        out.amplitude.emplace_back(w, mag);
        out.phase.emplace_back(w, phi * rad2deg);
    }
    return out;
}

/// КЧХ + АЧХ + ФЧХ via ResponseLab: auto / range / range+N. Delay is on the lab (DelayedPlant).
inline FrequencyBundle frequencyBundle(const numina::ResponseLab& lab, const ModelParam& p) {
    const bool astatic_auto = p.autoFreqRange && hasZeroDenConstant(lab.tf());
    if (p.autoFreqRange && !astatic_auto)
        return make_freq_bundle(lab.frequency(), lab.amplitudeFrequency(), lab.phaseFrequency());
    const auto range = astatic_auto ? astaticFreqRange(lab.tf()) : freqRange(p);
    if (p.autoFreqRange || p.autoFreqIntervals)
        return make_freq_bundle(lab.frequency(range), lab.amplitudeFrequency(range), lab.phaseFrequency(range));
    const std::size_t n = static_cast<std::size_t>(std::max(2, p.freqIntervals));
    return make_freq_bundle(lab.frequency(range, n), lab.amplitudeFrequency(range, n), lab.phaseFrequency(range, n));
}

inline FrequencyBundle frequencyBundle(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    return with_lab(tf, p, tau, [&](const numina::ResponseLab& lab) { return frequencyBundle(lab, p); });
}

inline numina::QualityReport quality(const numina::TransferFunction& tf, const ModelParam& p, double tau = 0.0) {
    return with_lab(tf, p, tau, [](const numina::ResponseLab& lab) { return lab.evaluate(); });
}

inline bool validInput(const std::vector<double>& num, const std::vector<double>& den) {
    bool num_ok = false;
    for (double c : num) {
        if (c != 0.0) {
            num_ok = true;
            break;
        }
    }
    if (!num_ok)
        return false;
    if (den.size() < 2)
        return false;
    if (num.size() > den.size())
        return false;
    return true;
}
}
