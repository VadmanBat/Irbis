#include "code/charts/chart-panel.h"
#include "code/series/bounds-set.hpp"
#include "code/tabs/rim-tab.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/format.hxx"
#include "code/util/secondary-text.hxx"
#include "code/util/tf-builder.hpp"
#include "ui_rim-tab.h"

#include <cmath>
#include <exception>
#include <utility>

namespace {
using Series                           = std::vector<std::pair<double, double>>;
constexpr std::size_t MAX_CHART_POINTS = 8000;
constexpr std::size_t MAX_STEPS        = 2'000'000;

Series downsample(const Series& src) {
    if (src.size() <= MAX_CHART_POINTS)
        return src;
    const std::size_t stride = (src.size() + MAX_CHART_POINTS - 1) / MAX_CHART_POINTS;
    Series out;
    out.reserve(src.size() / stride + 2);
    for (std::size_t i = 0; i < src.size(); i += stride)
        out.push_back(src[i]);
    if (out.empty() || out.back().first != src.back().first)
        out.push_back(src.back());
    return out;
}

void fit_panel(ChartPanel* panel, const BoundsSet& bounds) {
    panel->fitAxes(bounds.min_x(), bounds.max_x(), bounds.min_y(), bounds.max_y(), false, true);
}
}

bool RimTab::build_plant(numina::TransferFunction& out, double& tau) {
    auto num = form_->numerator();
    auto den = form_->denominator();
    if (const QString err = tab_ui::plantInputError(num, den); !err.isEmpty()) {
        show_error(err);
        return false;
    }
    out = tf_builder::plant(std::move(num), std::move(den));
    tau = form_->hasDelay() ? form_->delayTime() : 0.0;
    if (tau < 0.0)
        tau = 0.0;
    return true;
}

void RimTab::seed_initial_point(const double sp, const double mu0) {
    session_->sp.emplace_back(0.0, sp);
    session_->y_r.emplace_back(0.0, session_->y_real);
    session_->y_i.emplace_back(0.0, session_->y_ideal);
    session_->mu_r.emplace_back(0.0, mu0);
    session_->mu_i.emplace_back(0.0, mu0);
}

void RimTab::run_segment(const double duration, const double sp) {
    const double dt = session_->dt;
    const auto n    = static_cast<std::size_t>(std::llround(duration / dt));
    session_->sp.reserve(session_->sp.size() + n);
    session_->y_r.reserve(session_->y_r.size() + n);
    session_->y_i.reserve(session_->y_i.size() + n);
    session_->mu_r.reserve(session_->mu_r.size() + n);
    session_->mu_i.reserve(session_->mu_i.size() + n);

    if (!session_->sp.empty() && session_->sp.back().second != sp)
        session_->sp.emplace_back(session_->t, sp);

    for (std::size_t i = 0; i < n; ++i) {
        const double e_real   = sp - session_->y_real;
        const double e_ideal  = sp - session_->y_ideal;
        const double mu_real  = rim::updateRegulator(session_->real, e_real);
        const double mu_ideal = session_->ideal.update(e_ideal);
        session_->y_real      = session_->plant_real.update(mu_real);
        session_->y_ideal     = session_->plant_ideal.update(mu_ideal);
        session_->t += dt;
        session_->sp.emplace_back(session_->t, sp);
        session_->y_r.emplace_back(session_->t, session_->y_real);
        session_->y_i.emplace_back(session_->t, session_->y_ideal);
        session_->mu_r.emplace_back(session_->t, mu_real);
        session_->mu_i.emplace_back(session_->t, mu_ideal);
    }
}

void RimTab::redraw_charts() {
    y_chart_->clearCurves();
    mu_chart_->clearCurves();
    if (!session_)
        return;

    const auto sp = downsample(session_->sp);
    BoundsSet y_bounds;
    if (session_->deadzone > 0.0) {
        const double h2 = 0.5 * session_->deadzone;
        Series lo;
        Series hi;
        lo.reserve(sp.size());
        hi.reserve(sp.size());
        for (const auto& [t, y] : sp) {
            lo.emplace_back(t, y - h2);
            hi.emplace_back(t, y + h2);
        }
        y_bounds.push_back(y_chart_->addBand(lo, hi, tr("ЗН")));
    }
    y_bounds.push_back(y_chart_->addRealCurve(sp, tr("Уставка")));
    y_bounds.push_back(y_chart_->addRealCurve(downsample(session_->y_r), tr("Реальный РИМ")));
    y_bounds.push_back(y_chart_->addRealCurve(downsample(session_->y_i), tr("Идеальный")));
    fit_panel(y_chart_, y_bounds);

    BoundsSet mu_bounds;
    mu_bounds.push_back(mu_chart_->addRealCurve(downsample(session_->mu_r), tr("Реальный РИМ")));
    mu_bounds.push_back(mu_chart_->addRealCurve(downsample(session_->mu_i), tr("Идеальный")));
    fit_panel(mu_chart_, mu_bounds);

    ui->statusLabel->setText(tr("t = %1 с").arg(num_format::format(session_->t)));
    secondary_text::apply(ui->statusLabel);
}

void RimTab::runSimulation() {
    const double dt       = ui->dtSpin->value();
    const double duration = ui->durationSpin->value();
    const double sp       = ui->setpointSpin->value();
    if (!(dt > 0.0) || !(duration > 0.0)) {
        show_error(tr("Задайте положительные время моделирования и шаг dt."));
        return;
    }
    const double n_real = duration / dt;
    if (n_real < 0.5) {
        show_error(tr("Интервал моделирования меньше шага интегрирования."));
        return;
    }
    if (n_real > static_cast<double>(MAX_STEPS)) {
        show_error(tr("Слишком много шагов (%1). Увеличьте dt или уменьшите время.").arg(num_format::format(n_real)));
        return;
    }

    const auto law      = selected_law();
    const auto settings = read_pid_settings();
    if (rim::hasI(law) && !(settings.ti > 0.0)) {
        show_error(tr("Для закона с каналом И задайте Ti > 0."));
        return;
    }
    if (rim::hasP(law) && !(settings.kp >= 0.0)) {
        show_error(tr("Kp не может быть отрицательным."));
        return;
    }
    if (!(settings.travel_time > 0.0) || !(settings.pulse_time > 0.0)) {
        show_error(tr("Tим и tимп должны быть положительными."));
        return;
    }

    numina::TransferFunction plant;
    double tau = 0.0;
    if (!build_plant(plant, tau))
        return;

    try {
        const auto [ideal_num, ideal_den] = rim::idealPair(law, settings);
        auto real_reg                     = rim::makeRegulator(law, dt, settings);
        rim::resetRegulator(real_reg, settings.valve0, 0.0);

        TfStepper plant_real(plant, dt, tau);
        TfStepper plant_ideal(plant, dt, tau);
        TfStepper ideal_reg(ideal_num, ideal_den, dt);
        plant_real.reset();
        plant_ideal.reset();
        ideal_reg.reset(settings.valve0);

        session_ = std::make_unique<Session>(std::move(plant_real), std::move(plant_ideal), std::move(real_reg),
                                             std::move(ideal_reg), dt);
        session_->deadzone = settings.deadzone;
        seed_initial_point(sp, settings.valve0);
        run_segment(duration, sp);
        set_run_locked(true);
        redraw_charts();
    }
    catch (const std::exception& ex) {
        session_.reset();
        set_run_locked(false);
        show_error(QString::fromUtf8(ex.what()));
    }
}

void RimTab::continueSimulation() {
    if (!session_) {
        runSimulation();
        return;
    }
    const double duration = ui->durationSpin->value();
    const double sp       = ui->setpointSpin->value();
    if (!(duration > 0.0)) {
        show_error(tr("Задайте положительное время продолжения."));
        return;
    }
    const double n_real = duration / session_->dt;
    if (n_real < 0.5) {
        show_error(tr("Интервал продолжения меньше шага интегрирования."));
        return;
    }
    const auto next = session_->sp.size() + static_cast<std::size_t>(std::llround(n_real));
    if (next > MAX_STEPS) {
        show_error(tr("Слишком длинный прогон. Сбросьте график или увеличьте dt."));
        return;
    }
    try {
        run_segment(duration, sp);
        redraw_charts();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

void RimTab::resetSimulation() {
    session_.reset();
    y_chart_->clearCurves();
    mu_chart_->clearCurves();
    ui->statusLabel->setText(tr("Прогон не начат"));
    secondary_text::apply(ui->statusLabel);
    set_run_locked(false);
}
