#include "code/charts/chart-panel.h"
#include "code/series/bounds-set.hpp"
#include "code/tabs/id-tab.h"
#include "code/util/tf-builder.hpp"
#include "numina/classes/control/identification/dead-time-estimator.h"
#include "numina/classes/control/identification/integrator-identifier.h"
#include "numina/classes/control/identification/simoyu-identifier.h"
#include "ui_id-tab.h"

#include <algorithm>
#include <utility>

namespace {
using Series = std::vector<std::pair<double, double>>;

Series integrator_output(const Series& valve, const Series& signal, double k, double u_eq) {
    Series out;
    if (valve.size() < 2 || valve.size() != signal.size())
        return out;
    out.reserve(signal.size());
    const double y0 = signal.front().second;
    double acc      = 0.0;
    out.emplace_back(signal.front().first, y0);
    for (std::size_t i = 1; i < signal.size(); ++i) {
        const double dt = valve[i].first - valve[i - 1].first;
        const double u0 = valve[i - 1].second - u_eq;
        const double u1 = valve[i].second - u_eq;
        acc += 0.5 * (u0 + u1) * dt;
        out.emplace_back(signal[i].first, y0 + k * acc);
    }
    return out;
}
}

void IdTab::show_file_preview() {
    if (!chart_)
        return;
    chart_->clearCurves();
    BoundsSet bounds;
    const auto method = static_cast<Method>(ui->methodCombo->currentIndex());
    if (method == Method::StepResponse) {
        chart_->setChartTitle(tr("h(t): эксперимент"));
        bounds.push_back(chart_->addRealCurve(step_series_, tr("Эксперимент")));
    }
    else {
        chart_->setChartTitle(tr("u(t), y(t): эксперимент"));
        bounds.push_back(chart_->addRealCurve(valve_series_, tr("u(t)")));
        bounds.push_back(chart_->addRealCurve(signal_series_, tr("y(t)")));
    }
    chart_->fitAxes(bounds.min_x(), bounds.max_x(), bounds.min_y(), bounds.max_y(), /*niceX=*/false, /*niceY=*/true);
}

void IdTab::show_overlay(const Series& experiment, const Series& model) {
    if (!chart_)
        return;
    chart_->clearCurves();
    BoundsSet bounds;
    bounds.push_back(chart_->addRealCurve(experiment, tr("Эксперимент")));
    bounds.push_back(chart_->addRealCurve(model, tr("Модель")));
    chart_->fitAxes(bounds.min_x(), bounds.max_x(), bounds.min_y(), bounds.max_y(), /*niceX=*/false, /*niceY=*/true);
}

void IdTab::apply_result(const numina::TransferFunction& plant, double tau, const Series& experiment) {
    display_->setTransferFunction(plant, tau);
    show_overlay(experiment, tf_builder::sampleTransientAt(plant, experiment, tau));
}

void IdTab::run_static(const Series& experimental_h) {
    const auto settings         = read_id_settings();
    const std::size_t den_n     = static_cast<std::size_t>(std::clamp(settings.denOrder, 1, 6));
    const std::size_t num_m     = static_cast<std::size_t>(std::clamp(settings.numOrder, 0, static_cast<int>(den_n)));
    const std::size_t max_order = static_cast<std::size_t>(std::clamp(settings.maxAutoOrder, 2, 6));

    double dt    = 0.0;
    const auto h = numina::StepNormalizer::seriesToUniform(experimental_h, dt);
    if (h.size() < 2 || !(dt > 0.0)) {
        show_error(tr("Не удалось привести h(t) к равномерной сетке (точек: %1).").arg(experimental_h.size()));
        return;
    }

    const double tau = settings.estimateTau ? numina::DeadTimeEstimator::estimate(h, dt) : 0.0;
    const auto hs    = numina::DeadTimeEstimator::strip(h, dt, tau);
    numina::SimoyuIdentifier simoyu;
    simoyu.setMode(numina::SimoyuIdentifier::Mode::Refined);
    const auto plant =
        settings.autoOrder ? simoyu.identifyAuto(hs, dt, max_order, max_order) : simoyu.identify(hs, dt, den_n, num_m);

    if (plant.denominator().degree() < 1) {
        show_error(tr("Идентификация не дала модели (deg D < 1).\n"
                      "Точек h(t): %1. Проверьте данные и метод.")
                       .arg(experimental_h.size()));
        return;
    }

    if (chart_)
        chart_->setChartTitle(tr("h(t): эксперимент / модель"));
    apply_result(plant, tau > 0.0 ? tau : 0.0, experimental_h);
}

void IdTab::run_astatic(Method method) {
    numina::IntegratorIdentifier id;
    numina::TransferFunction plant;

    if (method == Method::StepResponse) {
        plant = id.identify(step_series_);
        if (plant.denominator().degree() < 1) {
            show_error(
                tr("Идентификация интегратора не дала модели k/p.\n"
                   "h(t) не должна быть константой."));
            return;
        }
        if (chart_)
            chart_->setChartTitle(tr("h(t): эксперимент / модель"));
        apply_result(plant, 0.0, step_series_);
        return;
    }

    plant = id.identify(valve_series_, signal_series_);
    if (plant.denominator().degree() < 1) {
        show_error(
            tr("Идентификация интегратора не дала модели k/p.\n"
               "Для (u, y) вход должен меняться."));
        return;
    }

    double dt_u  = 0.0;
    double dt_y  = 0.0;
    const auto u = numina::StepNormalizer::seriesToUniform(valve_series_, dt_u);
    const auto y = numina::StepNormalizer::seriesToUniform(signal_series_, dt_y);
    const auto g = (u.size() == y.size() && u.size() >= 3 && dt_u == dt_y && dt_u > 0.0)
                       ? numina::IntegratorIdentifier::fit(u, y, dt_u)
                       : numina::IntegratorIdentifier::Gain{};

    display_->setTransferFunction(plant);
    if (chart_)
        chart_->setChartTitle(tr("y(t): эксперимент / модель"));
    show_overlay(signal_series_, integrator_output(valve_series_, signal_series_, g.k, g.u_eq));
}
