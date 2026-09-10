// closedLoop / plant: Padé order comes from the caller (model settings).
// Build: see CMakeLists option IRBIS_BUILD_TESTS.

#include "irbis/util/tf-builder.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
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
}

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

    expect_true("valid 1/(p+1)", tf_builder::validInput({1.0}, {1.0, 1.0}));
    expect_true("zero num invalid", !tf_builder::validInput({}, {1.0, 1.0}));
    expect_true("zero num {0} invalid", !tf_builder::validInput({0.0}, {1.0, 1.0}));
    expect_true("zero num {0,0} invalid", !tf_builder::validInput({0.0, 0.0}, {1.0, 1.0}));
    expect_true("den degree 0 invalid", !tf_builder::validInput({1.0}, {2.0}));
    expect_true("num deg > den invalid", !tf_builder::validInput({1.0, 1.0, 1.0}, {1.0, 1.0}));

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
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox     = false;
        p.autoFreqRange     = false;
        p.autoFreqIntervals = false;
        p.freqMin           = 1.0;
        p.freqMax           = 2.0;
        p.freqIntervals     = 4;
        const auto f0       = tf_builder::frequencyBundle(w0, p, 0.0);
        const auto fd       = tf_builder::frequencyBundle(w0, p, 0.5);
        expect_true("bundle has point", !f0.nyquist.empty() && !fd.nyquist.empty());
        expect_true("same n", f0.nyquist.size() == fd.nyquist.size());
        bool checked        = false;
        const std::size_t n = std::min(fd.nyquist.size(), fd.amplitude.size());
        for (std::size_t i = 0; i < n; ++i) {
            const double w = fd.amplitude[i].first;
            if (!(w > 0.0))
                continue;
            const auto want = f0.nyquist[i] * std::exp(std::complex<double>(0.0, -w * 0.5));
            expect_true("КЧХ · exp", std::abs(fd.nyquist[i] - want) <= 1e-12);
            expect_true("АЧХ |W₀|", std::abs(fd.amplitude[i].second - std::abs(f0.nyquist[i])) <= 1e-12);
            const double want_deg = f0.phase[i].second - w * 0.5 * 180.0 / std::numbers::pi;
            expect_true("ФЧХ −ωτ", std::abs(fd.phase[i].second - want_deg) <= 1e-9);
            checked = true;
            break;
        }
        expect_true("found ω>0", checked);
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox = false;
        const auto b    = tf_builder::frequencyBundle(w0, p);
        expect_true("auto КЧХ nonempty", !b.nyquist.empty() && b.nyquist.size() == b.amplitude.size());
        bool finite = !b.nyquist.empty();
        for (const auto& z : b.nyquist)
            finite = finite && std::isfinite(z.real()) && std::isfinite(z.imag());
        expect_true("auto КЧХ finite", finite);
        expect_true("auto ФЧХ ° near 0 at DC", !b.phase.empty() && std::abs(b.phase.front().second) < 5.0);
    }

    {
        const auto integ = tf_builder::plant({1.0}, {1.0, 0.0});
        expect_true("1/p D(0)=0", tf_builder::hasZeroDenConstant(integ));
        const auto wr = tf_builder::astaticFreqRange(integ);
        expect_true("1/p auto ω>0", wr.first > 0.0 && wr.second > wr.first);

        ModelParam p;
        p.usePadeApprox = false;
        const auto b    = tf_builder::frequencyBundle(integ, p);
        expect_true("astatic nonempty", !b.nyquist.empty() && !b.amplitude.empty());
        bool ok = !b.amplitude.empty() && b.amplitude.front().first > 0.0;
        for (std::size_t i = 0; i < b.amplitude.size(); ++i) {
            const double w = b.amplitude[i].first;
            const auto z   = b.nyquist[i];
            ok             = ok && std::isfinite(w) && std::isfinite(z.real()) && std::isfinite(z.imag());
            ok             = ok && std::abs(b.amplitude[i].second - 1.0 / w) <= 1e-9 * (1.0 + 1.0 / w);
            ok             = ok && std::abs(b.phase[i].second + 90.0) <= 1e-6;
        }
        expect_true("astatic 1/p finite |W|=1/ω φ=−90°", ok);
    }

    {
        const auto w = tf_builder::plant({1.0}, {1.0, 1.0, 0.0}); // 1/(p(p+1))
        const auto r = tf_builder::astaticFreqRange(w);
        expect_true("1/(p²+p) lo = ωc/1e3", r.first > 0.0 && std::abs(r.first - 1e-3) <= 1e-6);
        expect_true("1/(p²+p) hi = ωc", std::abs(r.second - 1.0) <= 1e-6);
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox     = false;
        p.autoFreqRange     = false;
        p.autoFreqIntervals = true;
        p.freqMin           = 0.1;
        p.freqMax           = 10.0;
        const auto b        = tf_builder::frequencyBundle(w0, p);
        expect_true("range-adaptive nonempty", b.amplitude.size() >= 2);
        expect_true("range-adaptive ω in span", !b.amplitude.empty() && b.amplitude.front().first >= 0.1 - 1e-12 &&
                                                    b.amplitude.back().first <= 10.0 + 1e-9);
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox     = false;
        p.autoTimeRange     = false;
        p.autoTimeIntervals = false;
        p.timeMin           = 0.0;
        p.timeMax           = 2.0;
        p.timeIntervals     = 9;
        const auto pts      = tf_builder::transient(w0, p, 0.5);
        expect_true("delay h nonempty", pts.size() >= 2);
        bool dead  = true;
        bool after = false;
        for (const auto& pt : pts) {
            if (pt.first < 0.5 - 1e-12 && std::abs(pt.second) > 1e-12)
                dead = false;
            if (pt.first > 0.5 + 1e-9) {
                const double want = w0.transientResponse(pt.first - 0.5);
                after             = after || std::abs(pt.second - want) <= 1e-9;
            }
        }
        expect_true("h(t<τ)=0 via lab", dead);
        expect_true("h(t>τ)=h0(t−τ) via lab", after);
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        ModelParam p;
        p.usePadeApprox = false;
        const auto q0   = tf_builder::quality(w0, p);
        const auto q    = tf_builder::quality(w0, p, 0.5);
        expect_true("ts += τ", std::abs(q.settling_time - q0.settling_time - 0.5) <= 1e-12);
        expect_true("tr unchanged", std::abs(q.rise_time - q0.rise_time) <= 1e-12);
        expect_true("iae += |y∞|τ", std::abs(q.iae - q0.iae - 0.5) <= 1e-9);
    }

    {
        const auto w0 = tf_builder::plant({1.0}, {1.0, 1.0});
        const tf_builder::VecPair times{{0.0, 99.0}, {1.0, 99.0}};
        const auto pts = tf_builder::sampleTransientAt(w0, times);
        expect_eq("sample n", static_cast<int>(pts.size()), 2);
        expect_true("sample t1", !pts.empty() && std::abs(pts[1].first - 1.0) <= 1e-15);
        expect_true("sample h0", pts.size() > 1 && std::abs(pts[0].second) <= 1e-12);
        expect_true("sample h1", pts.size() > 1 && std::abs(pts[1].second - (1.0 - std::exp(-1.0))) <= 1e-9);
        const auto delayed = tf_builder::sampleTransientAt(w0, times, 2.0);
        expect_true("sample τ silences t<τ", delayed.size() > 1 && std::abs(delayed[1].second) <= 1e-12);
    }

    if (g_failed) {
        std::fprintf(stderr, "%d failed\n", g_failed);
        return EXIT_FAILURE;
    }
    std::printf("all ok\n");
    return EXIT_SUCCESS;
}
