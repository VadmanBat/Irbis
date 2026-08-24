// closedLoop / plant: Padé order comes from the caller (model settings).
// Build: see CMakeLists option IRBIS_BUILD_TESTS.

#include "code/util/tf-builder.hpp"

#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <numbers>

namespace {

int g_failed = 0;

void expect_true(const char* name, bool cond) {
    if (!cond) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

void expect_eq(const char* name, int got, int expected) {
    if (got != expected) {
        std::fprintf(stderr, "FAIL %s: got %d, expected %d\n", name, got, expected);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

bool same_poly(const numina::Polynomial& a, const numina::Polynomial& b) {
    if (a.degree() != b.degree())
        return false;
    for (int i = 0; i <= a.degree(); ++i) {
        if (std::abs(a[static_cast<std::size_t>(i)] - b[static_cast<std::size_t>(i)]) > 1e-12)
            return false;
    }
    return true;
}

} // namespace

int main() {
    // Plant 1/(p+1), P-controller Kp=1. Closed den degree = 1 + Padé order.
    const std::vector<double> plant_num{1.0};
    const std::vector<double> plant_den{1.0, 1.0};
    const std::vector<double> reg_num{1.0};
    const std::vector<double> reg_den{1.0};
    const double tau = 0.5;

    for (const int order : {1, 2, 3, 6}) {
        const auto cl = tf_builder::closedLoop(plant_num, plant_den, reg_num, reg_den, tau, order);
        char name[64];
        std::snprintf(name, sizeof(name), "closed den deg order=%d", order);
        expect_eq(name, cl.denominator().degree(), 1 + order);

        const auto pl = tf_builder::plant(plant_num, plant_den, tau, order);
        std::snprintf(name, sizeof(name), "plant den deg order=%d", order);
        expect_eq(name, pl.denominator().degree(), 1 + order);
    }

    const auto a = tf_builder::closedLoop(plant_num, plant_den, reg_num, reg_den, tau, 2);
    const auto b = tf_builder::closedLoop(plant_num, plant_den, reg_num, reg_den, tau, 6);
    expect_true("order 2 vs 6 differ", !same_poly(a.denominator(), b.denominator()));

    const auto z2 = tf_builder::closedLoop(plant_num, plant_den, reg_num, reg_den, 0.0, 2);
    const auto z6 = tf_builder::closedLoop(plant_num, plant_den, reg_num, reg_den, 0.0, 6);
    expect_true("tau=0 ignores order", same_poly(z2.denominator(), z6.denominator()));
    expect_eq("tau=0 den deg", z2.denominator().degree(), 1);

    {
        const auto w0 = tf_builder::plant(plant_num, plant_den);
        ModelParam pade;
        pade.usePadeApprox = true;
        pade.approxOrder   = 4;
        ModelParam exact;
        exact.usePadeApprox = false;
        exact.approxOrder   = 4;
        expect_eq("exact withDelay keeps deg", tf_builder::withDelay(w0, tau, exact).denominator().degree(), 1);
        expect_eq("pade withDelay adds order", tf_builder::withDelay(w0, tau, pade).denominator().degree(), 5);
    }

    {
        tf_builder::VecPair pts{{0.0, 1.0}, {1.0, 2.0}};
        tf_builder::shiftTimeByDelay(pts, 0.5);
        expect_eq("shift prepends 0", static_cast<int>(pts.size()), 3);
        expect_true("shift t0", std::abs(pts[0].first) <= 1e-15);
        expect_true("shift t1", std::abs(pts[1].first - 0.5) <= 1e-15);
        expect_true("shift t2", std::abs(pts[2].first - 1.5) <= 1e-15);
    }

    {
        tf_builder::FrequencyBundle bundle;
        bundle.nyquist.push_back({1.0, 0.0});
        bundle.amplitude.push_back({2.0, 1.0});
        bundle.phase.push_back({2.0, 0.0});
        tf_builder::applyExactFreqDelay(bundle, 0.5);
        const double want_deg = -2.0 * 0.5 * 180.0 / std::numbers::pi;
        expect_true("exact phase −ωτ", std::abs(bundle.phase[0].second - want_deg) <= 1e-12);
        expect_true("|W| unchanged", std::abs(bundle.amplitude[0].second - 1.0) <= 1e-15);
    }

    {
        // W₀ = 1/(p+1): numina даёт W₀(jω), Irbis домножает КЧХ на e^{-jωτ}.
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox     = false;
        p.autoFreqRange     = false;
        p.autoFreqIntervals = false;
        p.freqMin           = 1.0;
        p.freqMax           = 2.0;
        p.freqIntervals     = 4;
        const auto f0 = tf_builder::frequencyBundle(w0, p, 0.0);
        const auto fd = tf_builder::frequencyBundle(w0, p, 0.5);
        expect_true("bundle has point", !f0.nyquist.empty() && !fd.nyquist.empty());
        if (!f0.nyquist.empty() && !fd.nyquist.empty()) {
            const double w        = fd.amplitude[0].first;
            const auto want       = f0.nyquist[0] * std::exp(std::complex<double>(0.0, -w * 0.5));
            expect_true("КЧХ · exp", std::abs(fd.nyquist[0] - want) <= 1e-12);
            expect_true("АЧХ |W₀|", std::abs(fd.amplitude[0].second - std::abs(f0.nyquist[0])) <= 1e-12);
        }
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        numina::DelayedPlant plant(w0, 0.5);
        tf_builder::VecPair pts{{0.0, 99.0}, {0.25, 99.0}, {0.75, 99.0}};
        tf_builder::apply_delayed_time_samples(pts, plant, false);
        expect_true("h(t<τ)=0", std::abs(pts[0].second) <= 1e-15);
        expect_true("h(t<τ) mid", std::abs(pts[1].second) <= 1e-15);
        expect_true("h(t>τ)=h0(t−τ)", std::abs(pts[2].second - w0.transientResponse(0.25)) <= 1e-12);
    }

    if (g_failed) {
        std::fprintf(stderr, "%d failed\n", g_failed);
        return EXIT_FAILURE;
    }
    std::printf("all ok\n");
    return EXIT_SUCCESS;
}
