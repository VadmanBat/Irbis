// Pure unit tests for chart_utils::niceNumber / niceAxisRange (no Qt).
// Build: see CMakeLists option IRBIS_BUILD_TESTS.

#include "code/charts/utils/nice-axis.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
    using chart_utils::finerNiceStep;
    using chart_utils::labelDecimals;
    using chart_utils::labelFieldWidth;
    using chart_utils::niceAxisRange;
    using chart_utils::niceNumber;
    using chart_utils::niceTickStepIn;
    using chart_utils::tickCountInRange;

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

    // Endpoint 0 after lo + n·step
    {
        using chart_utils::detail::snap_tick;
        expect_near("snap -0.4+2*0.2", snap_tick(-0.4 + 2.0 * 0.2, 0.2), 0.0);
        expect_near("snap 0.4-2*0.2", snap_tick(0.4 - 2.0 * 0.2, 0.2), 0.0);
    }

    // Label decimals from step (%.Nf rounds 2.22e-16 to 0)
    expect_true("dec 1", labelDecimals(1.0) == 0);
    expect_true("dec 2", labelDecimals(2.0) == 0);
    expect_true("dec 5", labelDecimals(5.0) == 0);
    expect_true("dec 20", labelDecimals(20.0) == 0);
    expect_true("dec 0.2", labelDecimals(0.2) == 1);
    expect_true("dec 0.5", labelDecimals(0.5) == 1);
    expect_true("dec 0.05", labelDecimals(0.05) == 2);
    expect_true("dec 2.5", labelDecimals(2.5) == 1);
    expect_true("dec 0.25", labelDecimals(0.25) == 2);
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.*f", labelDecimals(0.2), 2.2204460492503131e-16);
        expect_true("fmt 0.2 hides 2.22e-16", std::strcmp(buf, "0.0") == 0);
        std::snprintf(buf, sizeof(buf), "%.*f", labelDecimals(5.0), 2.2204460492503131e-16);
        expect_true("fmt 5 hides 2.22e-16", std::strcmp(buf, "0") == 0);
    }

    expect_near("finer 0.5", finerNiceStep(0.5), 0.2);
    expect_near("finer 0.2", finerNiceStep(0.2), 0.1);
    expect_near("finer 1", finerNiceStep(1.0), 0.5);

    // Unit-step h(t): range [0, 1.2] must not collapse to 0 / 0.5 / 1
    {
        const double step = niceTickStepIn(0.0, 1.2);
        expect_near("ht step", step, 0.2);
        expect_true("ht ticks >= 5", tickCountInRange(0.0, 1.2, step) >= 5);
    }
    {
        const double step = niceTickStepIn(0.0, 1.0);
        expect_true("unit ticks >= 5", tickCountInRange(0.0, 1.0, step) >= 5);
    }

    // h(t) T=200 → 200.5: same step, last tick stays 200, same field width
    {
        const double s200  = niceTickStepIn(0.0, 200.0);
        const double s2005 = niceTickStepIn(0.0, 200.5);
        expect_near("T200 step", s200, 50.0);
        expect_near("T200.5 step", s2005, 50.0);
        expect_true("T200 last is 200", tickCountInRange(0.0, 200.0, s200) == 5);
        expect_true("T200.5 last is 200", tickCountInRange(0.0, 200.5, s2005) == 5);
        expect_true("T200 width", labelFieldWidth(0.0, 200.0, s200) == labelFieldWidth(0.0, 200.5, s2005));
    }

    if (g_failed) {
        std::fprintf(stderr, "\n%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("\nAll nice-axis tests passed\n");
    return 0;
}
