#pragma once

#include "numina/classes/control/models/transfer-function.h"
#include "numina/classes/polynomial/polynomial.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

/// Discrete W(p) via backward Euler: p = (1 − q)/dt, q = z^{-1}.
/// Polynomial::compose matches TransferFunction::implicitEulerEquations.
/// Optional input delay line for exact e^{−τp} (rounded to n·dt).
class TfStepper {
public:
    using Type = double;

private:
    std::vector<Type> b_;
    std::vector<Type> a_;
    std::vector<Type> u_z_;
    std::vector<Type> y_z_;
    std::vector<Type> delay_;
    std::size_t delay_i_{};
    Type y_{};
    Type dt_{};

    void shift_hist(const Type u, const Type y) noexcept {
        const std::size_t n = u_z_.size();
        if (n == 0)
            return;
        for (std::size_t k = n; k-- > 1;) {
            u_z_[k] = u_z_[k - 1];
            y_z_[k] = y_z_[k - 1];
        }
        u_z_[0] = u;
        y_z_[0] = y;
    }

    [[nodiscard]] Type filter(const Type u) const noexcept {
        Type y              = b_[0] * u;
        const std::size_t n = a_.size();
        for (std::size_t k = 1; k < n; ++k)
            y += b_[k] * u_z_[k - 1] - a_[k] * y_z_[k - 1];
        return y;
    }

    void build(const numina::Polynomial& num, const numina::Polynomial& den, const Type dt, const Type tau) {
        dt_ = dt;
        const numina::Polynomial p_poly(std::vector<Type>{-Type(1) / dt, Type(1) / dt});
        const numina::Polynomial nd = num.compose(p_poly);
        const numina::Polynomial dd = den.compose(p_poly);
        if (dd.isZero())
            throw std::invalid_argument("TfStepper: denominator vanished after discretization");

        const int n_deg = dd.degree();
        const int m_deg = nd.isZero() ? 0 : nd.degree();
        const Type a0   = dd[static_cast<std::size_t>(n_deg)];
        if (!(std::abs(a0) > Type(0)))
            throw std::invalid_argument("TfStepper: singular implicit Euler (a0 = 0)");

        const int order = std::max(n_deg, m_deg);
        b_.assign(static_cast<std::size_t>(order) + 1, Type{});
        a_.assign(static_cast<std::size_t>(order) + 1, Type{});
        if (!nd.isZero()) {
            for (int k = 0; k <= m_deg; ++k)
                b_[static_cast<std::size_t>(k)] = nd[static_cast<std::size_t>(m_deg - k)] / a0;
        }
        for (int k = 0; k <= n_deg; ++k)
            a_[static_cast<std::size_t>(k)] = dd[static_cast<std::size_t>(n_deg - k)] / a0;

        if (order > 0) {
            u_z_.assign(static_cast<std::size_t>(order), Type{});
            y_z_.assign(static_cast<std::size_t>(order), Type{});
        }
        if (tau > Type(0)) {
            auto n_delay = static_cast<std::size_t>(std::llround(tau / dt));
            if (n_delay == 0)
                n_delay = 1;
            delay_.assign(n_delay, Type{});
        }
    }

public:
    TfStepper(const numina::Polynomial& num, const numina::Polynomial& den, const Type dt, const Type tau = {}) {
        build(num, den, dt, tau);
    }

    TfStepper(const numina::TransferFunction& tf, const Type dt, const Type tau = {}) {
        build(tf.numerator(), tf.denominator(), dt, tau);
    }

    void reset(const Type y0 = {}) noexcept {
        y_       = y0;
        delay_i_ = 0;
        std::fill(u_z_.begin(), u_z_.end(), Type{});
        std::fill(y_z_.begin(), y_z_.end(), y0);
        std::fill(delay_.begin(), delay_.end(), Type{});
    }

    Type update(Type u) noexcept {
        if (!delay_.empty()) {
            const Type delayed = delay_[delay_i_];
            delay_[delay_i_]   = u;
            delay_i_           = delay_i_ + 1;
            if (delay_i_ == delay_.size())
                delay_i_ = 0;
            u = delayed;
        }
        const Type y = filter(u);
        shift_hist(u, y);
        y_ = y;
        return y;
    }

    [[nodiscard]] Type value() const noexcept { return y_; }
    [[nodiscard]] Type dt() const noexcept { return dt_; }
};
