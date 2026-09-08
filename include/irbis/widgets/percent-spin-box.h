#pragma once

#include <algorithm>
#include <cmath>
#include <QDoubleSpinBox>

/// Percent spin: arrows snap to a 1% lattice; typing any value in range (0.01 %) is allowed.
/// 99 ↑ → 99,99 (max), 99,99 ↓ → 99, 75,25 ↑ → 76, 1 ↓ → min (1).
class PercentSpinBox : public QDoubleSpinBox {
public:
    using QDoubleSpinBox::QDoubleSpinBox;

protected:
    void stepBy(int steps) override {
        if (steps == 0)
            return;
        constexpr double k_grid = 1.0;
        constexpr double k_eps  = 1e-9;
        const double lo         = minimum();
        const double hi         = maximum();
        double v                = value();
        while (steps > 0) {
            const double next = (std::floor((v + k_eps) / k_grid) + 1.0) * k_grid;
            v                 = std::min(hi, next);
            --steps;
        }
        while (steps < 0) {
            const double n     = v / k_grid;
            const bool on_grid = std::abs(n - std::round(n)) < 1e-6;
            const double prev  = on_grid ? (v - k_grid) : std::floor(n + k_eps) * k_grid;
            v                  = std::max(lo, prev);
            ++steps;
        }
        setValue(v);
    }
};
