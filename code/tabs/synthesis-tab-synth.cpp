#include "code/control/controller-design.hpp"
#include "code/tabs/synthesis-tab.h"
#include "ui_synthesis-tab.h"

#include <cmath>
#include <QApplication>
#include <vector>

namespace {
C0C1Chart::Optimum to_optimum(const numina::ControllerDesigner::Design& d, const QString& label) {
    C0C1Chart::Optimum o;
    o.valid = d.isOk();
    o.c0    = d.settings.c0;
    o.c1    = d.settings.c1;
    o.kp    = d.settings.kp;
    o.ti    = d.settings.ti;
    o.omega = d.settings.omega;
    o.label = label;
    return o;
}

C0C1Chart::Sample to_sample(const numina::ControllerDesigner::Settings& s) {
    C0C1Chart::Sample sample;
    sample.omega = s.omega;
    sample.c0    = s.c0;
    sample.c1    = s.c1;
    sample.c2    = s.c2;
    sample.kp    = s.kp;
    sample.ti    = s.ti;
    sample.td    = s.td;
    return sample;
}
}

void SynthesisTab::sync_c0c1_selection_from_params() {
    const double kp = parameters_[0]->value();
    const double ti = parameters_[1]->value();
    if (kp > 0.0 && ti > 0.0 && std::isfinite(kp) && std::isfinite(ti))
        ui->c0c1Chart->setSelection(kp / ti, kp);
    else
        ui->c0c1Chart->clearSelection();
}

void SynthesisTab::apply_pi_settings(double kp, double ti, bool replace_last) {
    if (!(kp > 0.0) || !(ti > 0.0) || !std::isfinite(kp) || !std::isfinite(ti))
        return;
    block_param_signals(true);
    parameters_[0]->ensureValueInRange(kp);
    parameters_[1]->ensureValueInRange(ti);
    if (!parameters_[0]->enabled())
        parameters_[0]->setEnabled(true);
    if (!parameters_[1]->enabled())
        parameters_[1]->setEnabled(true);
    parameters_[0]->setValue(kp);
    parameters_[1]->setValue(ti);
    block_param_signals(false);
    apply_current_controller(replace_last);
}

void SynthesisTab::apply_design(const numina::ControllerDesigner::Design& d, bool replace_last) {
    if (!d.isOk())
        return;
    const auto& s    = d.settings;
    const bool use_d = d.law == numina::ControllerDesigner::Law::Pid && s.td > 0.0 && std::isfinite(s.td);
    block_param_signals(true);
    parameters_[0]->ensureValueInRange(s.kp);
    parameters_[1]->ensureValueInRange(s.ti);
    parameters_[0]->setEnabled(true);
    parameters_[1]->setEnabled(true);
    parameters_[0]->setValue(s.kp);
    parameters_[1]->setValue(s.ti);
    if (use_d) {
        parameters_[2]->ensureValueInRange(s.td);
        parameters_[2]->setEnabled(true);
        parameters_[2]->setValue(s.td);
    }
    else {
        parameters_[2]->setEnabled(false);
    }
    block_param_signals(false);
    apply_current_controller(replace_last);
}

void SynthesisTab::onSamplePicked(const C0C1Chart::Sample& sample) {
    if (!(sample.c0 > 0.0) || !(sample.c1 > 0.0) || !(sample.kp > 0.0) || !(sample.ti > 0.0))
        return;
    apply_pi_settings(sample.kp, sample.ti, !ui->charts->empty());
}

void SynthesisTab::autoSynthesize() {
    numina::TransferFunction plant;
    if (!build_plant(plant)) {
        show_error(tr("Задайте корректную ПФ объекта управления."));
        return;
    }

    try {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        const double phi = static_cast<double>(ui->phiSpin->value()) / 100.0;
        numina::ControllerDesigner designer(plant, phi);

        numina::ControllerDesigner::Spec spec;
        spec.phi    = phi;
        spec.law    = selected_law();
        spec.region = selected_region();

        using L           = numina::ControllerDesigner::Law;
        const auto bundle = controller_design::synthesize(designer, spec, selected_criterion());
        const auto loc    = controller_design::locus(designer, spec.law);
        if (loc.empty() && !bundle.face) {
            QApplication::restoreOverrideCursor();
            show_error(tr("Нет области C₀>0, C₁>0 при φ = %1 %.\n"
                          "ПИ недоступен (Ω_доп пуста). Выберите ПИД или Авто.")
                           .arg(ui->phiSpin->value()));
            return;
        }

        std::vector<C0C1Chart::Sample> samples;
        samples.reserve(loc.size());
        for (const auto& s : loc)
            samples.push_back(to_sample(s));
        ui->c0c1Chart->setLocus(std::move(samples), bundle.face ? tr("ЛРЗ (грань C₂*)") : tr("ЛРЗ"));
        ui->c0c1Chart->setOptima(bundle.gamma_pi ? C0C1Chart::Optimum{} : to_optimum(bundle.lik, tr("опт. ЛИК")),
                                 to_optimum(bundle.ikk, tr("опт. ИКК")), to_optimum(bundle.sko, tr("опт. СКО")));

        QApplication::restoreOverrideCursor();

        if (!bundle.selected().isOk()) {
            if (spec.law == L::Pid)
                show_error(
                    tr("ПИД недоступен: нет настроек с C₂>0.\n"
                       "Если ПИ уже обеспечивает φ, выберите ПИ или Авто."));
            else if (spec.law == L::Pi)
                show_error(
                    tr("ПИ недоступен (C₀, C₁ > 0 и доминирование / сектор Γ).\n"
                       "При пустой Ω_доп выберите ПИД или Авто."));
            else
                show_error(tr("Автонастройка не нашла ни ПИ, ни ПИД."));
            return;
        }

        apply_design(bundle.selected(), !ui->charts->empty());
        ui->c0c1Chart->requestRefit();
    }
    catch (const std::exception& ex) {
        QApplication::restoreOverrideCursor();
        show_error(tr("Автосинтез: %1").arg(QString::fromUtf8(ex.what())));
    }
}
