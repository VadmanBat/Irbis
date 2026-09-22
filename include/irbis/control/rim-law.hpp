#pragma once

#include "numina/classes/control/discrete/pid-controller.h"
#include "numina/classes/control/models/transfer-function.h"

#include <cmath>
#include <optional>
#include <variant>
#include <vector>

namespace rim {
using Regulator =
    std::variant<numina::PidController<numina::ControlLaw::P>, numina::PidController<numina::ControlLaw::Pd>,
                 numina::PidController<numina::ControlLaw::Pi>, numina::PidController<numina::ControlLaw::Pid>,
                 numina::PidController<numina::ControlLaw::I>>;

[[nodiscard]] constexpr bool hasP(const numina::ControlLaw law) noexcept {
    return law == numina::ControlLaw::P || law == numina::ControlLaw::Pd || law == numina::ControlLaw::Pi ||
           law == numina::ControlLaw::Pid;
}

[[nodiscard]] constexpr bool hasI(const numina::ControlLaw law) noexcept {
    return law == numina::ControlLaw::I || law == numina::ControlLaw::Pi || law == numina::ControlLaw::Pid;
}

[[nodiscard]] constexpr bool hasD(const numina::ControlLaw law) noexcept {
    return law == numina::ControlLaw::Pd || law == numina::ControlLaw::Pid;
}

/// 1.0 доли шкалы = 100 % хода. K в Wоу — приращение y на 1 % хода.
inline constexpr double FULL_SCALE_PERCENT = 100.0;

/// |y| на упоре клапана μ = ±0.5 для статического объекта: 50·|b₀/a₀|.
/// Пусто, если a₀ = 0 (астатический объект, конечного потолка нет).
[[nodiscard]] inline std::optional<double> staticOutputReach(const std::vector<double>& num,
                                                             const std::vector<double>& den) {
    if (num.empty() || den.empty())
        return std::nullopt;
    const double b0   = num.back();
    const double a0   = den.back();
    const double lead = std::abs(den.front());
    if (!(std::abs(a0) > 1e-14 * (1.0 + lead)))
        return std::nullopt;
    const double reach = 0.5 * FULL_SCALE_PERCENT * std::abs(b0 / a0);
    if (!std::isfinite(reach))
        return std::nullopt;
    return reach;
}

/// Kп и Tи — идеальный Wр, выход в процентах хода (это вход объекта).
/// numina хранит μ в долях, поэтому коэффициент закона делится на 100.
/// Закон И без Kп: Tи умножается на 100.
[[nodiscard]] inline numina::PidSettings fractionSettings(const numina::ControlLaw law,
                                                          const numina::PidSettings& settings) {
    numina::PidSettings scaled = settings;
    if (hasP(law))
        scaled.kp /= FULL_SCALE_PERCENT;
    else if (hasI(law) && scaled.ti > 0.0)
        scaled.ti *= FULL_SCALE_PERCENT;
    return scaled;
}

[[nodiscard]] inline Regulator makeRegulator(const numina::ControlLaw law, const double dt,
                                             const numina::PidSettings& settings) {
    switch (law) {
        case numina::ControlLaw::P:
            return numina::PidController<numina::ControlLaw::P>(dt, settings);
        case numina::ControlLaw::Pd:
            return numina::PidController<numina::ControlLaw::Pd>(dt, settings);
        case numina::ControlLaw::Pi:
            return numina::PidController<numina::ControlLaw::Pi>(dt, settings);
        case numina::ControlLaw::I:
            return numina::PidController<numina::ControlLaw::I>(dt, settings);
        case numina::ControlLaw::Pid:
        default:
            return numina::PidController<numina::ControlLaw::Pid>(dt, settings);
    }
}

/// Эталонный W_reg(p): TransferFunction::makeController (идеальный ПИД, без Td/8).
[[nodiscard]] inline numina::TransferFunction::PolyPair idealPair(const numina::ControlLaw law,
                                                                  const numina::PidSettings& s) {
    const double kp = hasP(law) ? s.kp : -1.0;
    const double ti = hasI(law) ? s.ti : -1.0;
    const double td = hasD(law) ? s.td : -1.0;
    return numina::TransferFunction::makeController(kp, ti, td);
}

[[nodiscard]] inline double updateRegulator(Regulator& reg, const double error) {
    return std::visit([error](auto& c) { return c.update(error); }, reg);
}

inline void resetRegulator(Regulator& reg, const double valve, const double error) {
    std::visit([valve, error](auto& c) { c.reset(valve, error); }, reg);
}
}
