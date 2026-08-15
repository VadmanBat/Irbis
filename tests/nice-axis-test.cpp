// Pure unit tests for chart_utils::niceNumber / niceAxisRange (no Qt).
// Build: see CMakeLists option IRBIS_BUILD_TESTS.

#include "code/charts/utils/nice-axis.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

int g_failed = 0;

void expect_near(const char* name, double got, double expected, double eps = 1e-9) {
    if (!(std::abs(got - expected) <= eps * std::max(1.0, std::abs(expected)))) {
        std::fprintf(stderr, "FAIL %s: got %.12g, expected %.12g\n", name, got, expected);
        ++g_failed;
    } else {
        std::printf("ok   %s\n", name);
    }
}

void expect_true(const char* name, bool cond) {
    if (!cond) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("ok   %s\n", name);
    }
}

} // namespace

int main() {
    using chart_utils::niceAxisRange;
    using chart_utils::niceNumber;

    // 1–2–5 rounding
    expect_near("niceNumber(25.9, round)", niceNumber(25.9, true), 20.0);
    expect_near("niceNumber(31, round)", niceNumber(31.0, true), 50.0);
    expect_near("niceNumber(100, ceil)", niceNumber(100.0, false), 100.0);

    // User example: 0 .. 155.456 → 0 .. 160
    {
        const auto r = niceAxisRange(0.0, 155.456, true);
        expect_near("range155 lo", r.first, 0.0);
        expect_near("range155 hi", r.second, 160.0);
        expect_true("range155 includes 0", r.first <= 0.0 && r.second >= 0.0);
    }

    // Negative + positive (Nyquist-like)
    {
        const auto r = niceAxisRange(-12.3, 47.8, true);
        expect_true("nyquist lo <= -12.3", r.first <= -12.3);
        expect_true("nyquist hi >= 47.8", r.second >= 47.8);
        expect_true("nyquist includes 0", r.first <= 0.0 && r.second >= 0.0);
    }

    // Single point
    {
        const auto r = niceAxisRange(5.0, 5.0, true);
        expect_true("point span", r.second > r.first);
        expect_true("point includes 0", r.first <= 0.0 && r.second >= 0.0);
    }

    // Tiny numerical undershoot → pin lo at 0 (no axis flip while tuning)
    {
        const auto r = niceAxisRange(-0.01, 1.0, true);
        expect_near("noise_neg lo", r.first, 0.0);
        expect_true("noise_neg hi >= 1", r.second >= 1.0);
    }

    // Real undershoot (~10%) must stay visible
    {
        const auto r = niceAxisRange(-0.1, 1.0, true);
        expect_true("undershoot lo < 0", r.first < 0.0);
        expect_true("undershoot hi >= 1", r.second >= 1.0);
    }

    if (g_failed) {
        std::fprintf(stderr, "\n%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("\nAll nice-axis tests passed\n");
    return 0;
}
