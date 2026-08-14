#pragma once

#include <algorithm>
#include <QSpinBox>

/// Integer percent spin: arrows snap to a 5% lattice, typing any integer in range is allowed.
/// 99 ↓ → 95, 94 ↑ → 95, 95 ↑ → max (99), 5 ↓ → min (1).
class PercentSpinBox : public QSpinBox {
public:
    using QSpinBox::QSpinBox;

protected:
    void stepBy(int steps) override {
        if (steps == 0)
            return;
        constexpr int k_grid = 5;
        const int lo         = minimum();
        const int hi         = maximum();
        int v                = value();
        while (steps > 0) {
            const int next = (v / k_grid + 1) * k_grid;
            v              = std::min(hi, next);
            --steps;
        }
        while (steps < 0) {
            const int prev = (v % k_grid == 0) ? (v - k_grid) : (v / k_grid) * k_grid;
            v              = std::max(lo, prev);
            ++steps;
        }
        setValue(v);
    }
};
