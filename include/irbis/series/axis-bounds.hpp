#pragma once

#include <complex>
#include <utility>
#include <vector>

/// Axis extents for a single curve (no point storage — charts keep QLineSeries).
/// Invalid/empty bounds must not participate in stacked min/max (BoundsSet skips them).
struct AxisBounds {
    double min_x{0.0};
    double max_x{0.0};
    double min_y{0.0};
    double max_y{0.0};
    bool valid{false};
};

[[nodiscard]] inline AxisBounds boundsOfReal(const std::vector<std::pair<double, double>>& data) noexcept {
    if (data.empty())
        return {};
    AxisBounds b;
    b.min_x = b.max_x = data.front().first;
    b.min_y = b.max_y = data.front().second;
    b.valid           = true;
    for (const auto& [x, y] : data) {
        if (x < b.min_x)
            b.min_x = x;
        if (x > b.max_x)
            b.max_x = x;
        if (y < b.min_y)
            b.min_y = y;
        if (y > b.max_y)
            b.max_y = y;
    }
    return b;
}

[[nodiscard]] inline AxisBounds boundsOfComplex(const std::vector<std::complex<double>>& data) noexcept {
    if (data.empty())
        return {};
    AxisBounds b;
    b.min_x = b.max_x = data.front().real();
    b.min_y = b.max_y = data.front().imag();
    b.valid           = true;
    for (const auto& z : data) {
        const double x = z.real();
        const double y = z.imag();
        if (x < b.min_x)
            b.min_x = x;
        if (x > b.max_x)
            b.max_x = x;
        if (y < b.min_y)
            b.min_y = y;
        if (y > b.max_y)
            b.max_y = y;
    }
    return b;
}
