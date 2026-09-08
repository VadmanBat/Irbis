#pragma once

#include "irbis/series/axis-bounds.hpp"

#include <vector>

/// LIFO stack of curve extents. Invalid entries keep alignment but are ignored in min/max.
/// H is small (≤24) — O(H) scan is simpler than multisets.
class BoundsSet {
    std::vector<AxisBounds> stack_;

public:
    void push_back(AxisBounds b) { stack_.push_back(b); }

    void pop_back() {
        if (!stack_.empty())
            stack_.pop_back();
    }

    void clear() { stack_.clear(); }

    [[nodiscard]] bool empty() const noexcept { return stack_.empty(); }

    [[nodiscard]] double min_x() const noexcept {
        double m = 0.0;
        bool any = false;
        for (const auto& b : stack_) {
            if (!b.valid)
                continue;
            m   = any ? (b.min_x < m ? b.min_x : m) : b.min_x;
            any = true;
        }
        return any ? m : 0.0;
    }
    [[nodiscard]] double max_x() const noexcept {
        double m = 1.0;
        bool any = false;
        for (const auto& b : stack_) {
            if (!b.valid)
                continue;
            m   = any ? (b.max_x > m ? b.max_x : m) : b.max_x;
            any = true;
        }
        return any ? m : 1.0;
    }
    [[nodiscard]] double min_y() const noexcept {
        double m = 0.0;
        bool any = false;
        for (const auto& b : stack_) {
            if (!b.valid)
                continue;
            m   = any ? (b.min_y < m ? b.min_y : m) : b.min_y;
            any = true;
        }
        return any ? m : 0.0;
    }
    [[nodiscard]] double max_y() const noexcept {
        double m = 1.0;
        bool any = false;
        for (const auto& b : stack_) {
            if (!b.valid)
                continue;
            m   = any ? (b.max_y > m ? b.max_y : m) : b.max_y;
            any = true;
        }
        return any ? m : 1.0;
    }
};
