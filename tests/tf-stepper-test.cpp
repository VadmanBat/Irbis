#include "code/util/tf-stepper.hpp"

#include "code/control/rim-law.hpp"
#include "numina/classes/polynomial/polynomial.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

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

void expect_near(const char* name, double got, double want, double eps) {
    const double scale = 1.0 + std::abs(want);
    if (std::abs(got - want) > eps * scale) {
        std::fprintf(stderr, "FAIL %s: got=%.8g want=%.8g\n", name, got, want);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

void test_first_order_step() {
    constexpr double t_end = 1.0;
    constexpr double t_lag = 1.0;
    constexpr double dt    = 0.001;
    numina::Polynomial num(1.0);
    numina::Polynomial den(std::vector<double>{t_lag, 1.0});
    TfStepper plant(num, den, dt);
    const auto n = static_cast<std::size_t>(std::llround(t_end / dt));
    double y     = 0.0;
    for (std::size_t i = 0; i < n; ++i)
        y = plant.update(1.0);
    expect_near("FO implicit Euler vs 1-exp(-t/T)", y, 1.0 - std::exp(-t_end / t_lag), 5e-3);
}

void test_pure_delay() {
    constexpr double dt  = 0.05;
    constexpr double tau = 0.2;
    numina::Polynomial one(1.0);
    TfStepper plant(one, one, dt, tau);
    double y = 0.0;
    for (int i = 0; i < 3; ++i)
        y = plant.update(1.0);
    expect_near("delay holds zero before τ", y, 0.0, 1e-12);
    for (int i = 0; i < 2; ++i)
        y = plant.update(1.0);
    expect_near("delay releases after τ", y, 1.0, 1e-12);
}

void test_pi_ideal_step() {
    numina::PidSettings s;
    s.kp                = 0.8;
    s.ti                = 10.0;
    const auto [n, d]   = rim::idealPair(numina::ControlLaw::Pi, s);
    constexpr double dt = 0.01;
    constexpr double e0 = 0.05;
    constexpr double t  = 16.0;
    TfStepper c(n, d, dt);
    const auto steps = static_cast<std::size_t>(std::llround(t / dt));
    double mu        = 0.0;
    for (std::size_t i = 0; i < steps; ++i)
        mu = c.update(e0);
    const double want = s.kp * e0 * (1.0 + t / s.ti);
    expect_near("PI ideal μ = Kp e (1+t/Ti)", mu, want, 2e-3);
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

void test_ideal_pair_is_make_controller() {
    numina::PidSettings s;
    s.kp = 1.2;
    s.ti = 8.0;
    s.td = 1.5;

    const auto pi  = rim::idealPair(numina::ControlLaw::Pi, s);
    const auto npi = numina::TransferFunction::makeController(s.kp, s.ti);
    expect_true("PI pair equals makeController", same_poly(pi.first, npi.first) && same_poly(pi.second, npi.second));

    const auto pid  = rim::idealPair(numina::ControlLaw::Pid, s);
    const auto npid = numina::TransferFunction::makeController(s.kp, s.ti, s.td);
    expect_true("PID pair equals makeController (no Td/8)",
                same_poly(pid.first, npid.first) && same_poly(pid.second, npid.second));
}
}

void test_integrator_pi_closed_loop() {
    constexpr double dt    = 0.05;
    constexpr double sp    = 0.2;
    constexpr double t_end = 80.0;
    numina::Polynomial num(1.0);
    numina::Polynomial den(std::vector<double>{1.0, 0.0});
    numina::PidSettings s;
    s.kp          = 1.0;
    s.ti          = 10.0;
    s.pulse_time  = 0.25;
    s.travel_time = 25.0;

    TfStepper plant_i(num, den, dt);
    TfStepper plant_r(num, den, dt);
    const auto [cn, cd] = rim::idealPair(numina::ControlLaw::Pi, s);
    TfStepper ideal(cn, cd, dt);
    auto real = rim::makeRegulator(numina::ControlLaw::Pi, dt, s);

    double y_i = 0.0;
    double y_r = 0.0;
    const auto n = static_cast<std::size_t>(std::llround(t_end / dt));
    double mu_i  = 0.0;
    double mu_r  = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        mu_i = ideal.update(sp - y_i);
        mu_r = rim::updateRegulator(real, sp - y_r);
        y_i  = plant_i.update(mu_i);
        y_r  = plant_r.update(mu_r);
    }
    expect_near("ideal integrator PI y→sp", y_i, sp, 0.05);
    expect_near("real integrator PI y→sp", y_r, sp, 0.08);
    (void)mu_i;
    (void)mu_r;
}

void test_integrator_p_closed_loop() {
    constexpr double dt    = 0.05;
    constexpr double sp    = 0.2;
    constexpr double t_end = 40.0;
    numina::Polynomial num(1.0);
    numina::Polynomial den(std::vector<double>{1.0, 0.0});
    numina::PidSettings s;
    s.kp          = 1.0;
    s.pulse_time  = 0.25;
    s.travel_time = 25.0;

    TfStepper plant_i(num, den, dt);
    TfStepper plant_r(num, den, dt);
    const auto [cn, cd] = rim::idealPair(numina::ControlLaw::P, s);
    TfStepper ideal(cn, cd, dt);
    auto real = rim::makeRegulator(numina::ControlLaw::P, dt, s);

    double y_i = 0.0;
    double y_r = 0.0;
    const auto n = static_cast<std::size_t>(std::llround(t_end / dt));
    for (std::size_t i = 0; i < n; ++i) {
        const double mu_i = ideal.update(sp - y_i);
        const double mu_r = rim::updateRegulator(real, sp - y_r);
        y_i               = plant_i.update(mu_i);
        y_r               = plant_r.update(mu_r);
        (void)mu_i;
        (void)mu_r;
    }
    expect_near("ideal integrator P y→sp", y_i, sp, 0.05);
}

void test_stop_holds_with_deadzone() {
    constexpr double dt = 0.05;
    numina::PidSettings s;
    s.kp          = 1.0;
    s.ti          = 10.0;
    s.pulse_time  = 0.25;
    s.travel_time = 8.0;
    s.deadzone    = 0.1;

    // I-law, constant error well outside ЗН: must stay at +0.5 after hitting the stop.
    {
        numina::PidController<numina::ControlLaw::I> c(dt, s);
        const int n = static_cast<int>(40.0 / dt);
        int hit     = -1;
        for (int i = 0; i < n; ++i) {
            c.update(0.4);
            if (hit < 0 && c.valve() >= 0.5 - 1e-12)
                hit = i;
            if (hit >= 0 && i > hit + 5 && c.valve() < 0.5 - 1e-6) {
                std::fprintf(stderr, "I+ЗН left stop: t=%g μ=%g\n", i * dt, c.valve());
                ++g_failed;
                break;
            }
        }
        expect_near("I+ЗН holds +0.5", c.valve(), 0.5, 1e-6);
    }

    // PI + integrator plant: after hitting +0.5, while e still > H/2, must not reverse.
    {
        TfStepper plant(numina::Polynomial(0.05), numina::Polynomial(std::vector<double>{1.0, 0.0}), dt);
        auto c          = rim::makeRegulator(numina::ControlLaw::Pi, dt, s);
        double y        = 0.0;
        constexpr double sp = 0.8;
        int hit         = -1;
        bool left       = false;
        double e_left   = 0.0;
        const int n     = static_cast<int>(80.0 / dt);
        for (int i = 0; i < n; ++i) {
            const double e  = sp - y;
            const double mu = rim::updateRegulator(c, e);
            y               = plant.update(mu);
            if (hit < 0 && mu >= 0.5 - 1e-12)
                hit = i;
            if (hit >= 0 && !left && mu < 0.5 - 0.01 && e > 0.5 * s.deadzone + 1e-6) {
                left   = true;
                e_left = e;
            }
        }
        if (left) {
            std::fprintf(stderr, "FAIL PI+ЗН reversed at stop with e=%g still outside ЗН\n", e_left);
            ++g_failed;
        }
        else {
            expect_true("PI+ЗН held the stop", true);
        }
    }

    // P-law + integrator after a downward step that hits −0.5: y must return near sp
    // (hold-until-sign-change would freeze μ=−0.5 and settle at the wrong level).
    {
        constexpr double k_gain = 0.2;
        TfStepper plant(numina::Polynomial(k_gain), numina::Polynomial(std::vector<double>{1.0, 0.0}), dt);
        TfStepper plant_i(numina::Polynomial(k_gain), numina::Polynomial(std::vector<double>{1.0, 0.0}), dt);
        numina::PidSettings p;
        p.kp          = 2.0;
        p.pulse_time  = 0.25;
        p.travel_time = 8.0;
        p.deadzone    = 0.05;
        auto real                   = rim::makeRegulator(numina::ControlLaw::P, dt, p);
        const auto [cn, cd]         = rim::idealPair(numina::ControlLaw::P, p);
        TfStepper ideal(cn, cd, dt);
        double y_r = 0.4;
        double y_i = 0.4;
        plant.reset(y_r);
        plant_i.reset(y_i);
        constexpr double sp = 0.05;
        const int n         = static_cast<int>(40.0 / dt);
        for (int i = 0; i < n; ++i) {
            y_r = plant.update(rim::updateRegulator(real, sp - y_r));
            y_i = plant_i.update(ideal.update(sp - y_i));
        }
        expect_near("P+tank after −stop, ideal y→sp", y_i, sp, 0.08);
        expect_near("P+tank after −stop, real y→sp", y_r, sp, 0.12);
    }
}

int main() {
    test_first_order_step();
    test_pure_delay();
    test_pi_ideal_step();
    test_ideal_pair_is_make_controller();
    test_integrator_pi_closed_loop();
    test_integrator_p_closed_loop();
    test_stop_holds_with_deadzone();
    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return EXIT_FAILURE;
    }
    std::printf("all tests passed\n");
    return EXIT_SUCCESS;
}
