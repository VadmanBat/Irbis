#include "irbis/tabs/synthesis-tab.h"
#include "ui_synthesis-tab.h"

#include <cmath>

void SynthesisTab::sync_c0c1_selection_from_params() {
    const double kp = parameters_[0]->value();
    const double ti = parameters_[1]->value();
    const double td = parameters_[2]->value();
    if (is_pd_structure()) {
        if (kp > 0.0 && td > 0.0 && std::isfinite(kp) && std::isfinite(td))
            ui->c0c1Chart->setSelection(0.0, kp, kp * td);
        else
            ui->c0c1Chart->clearSelection();
        return;
    }
    if (is_pi_structure() && kp > 0.0 && ti > 0.0 && std::isfinite(kp) && std::isfinite(ti))
        ui->c0c1Chart->setSelection(kp / ti, kp);
    else
        ui->c0c1Chart->clearSelection();
}

void SynthesisTab::apply_controller_params(bool p_on, double kp, bool i_on, double ti, bool d_on, double td,
                                           bool replace_last) {
    block_param_signals(true);
    auto put = [this](const int i, const bool on, const double v) {
        if (on && std::isfinite(v)) {
            parameters_[i]->ensureValueInRange(v);
            parameters_[i]->setEnabled(true);
            parameters_[i]->setValue(v);
        }
        else {
            parameters_[i]->setEnabled(false);
        }
    };
    put(0, p_on, kp);
    put(1, i_on, ti);
    put(2, d_on, td);
    block_param_signals(false);
    apply_current_controller(replace_last);
}

void SynthesisTab::apply_pi_settings(double kp, double ti, bool replace_last) {
    if (!(kp > 0.0) || !(ti > 0.0) || !std::isfinite(kp) || !std::isfinite(ti))
        return;
    apply_controller_params(true, kp, true, ti, false, 0.0, replace_last);
}

void SynthesisTab::apply_pd_settings(double kp, double td, bool replace_last) {
    if (!(kp > 0.0) || !(td > 0.0) || !std::isfinite(kp) || !std::isfinite(td))
        return;
    apply_controller_params(true, kp, false, 0.0, true, td, replace_last);
}

void SynthesisTab::apply_design(const numina::ControllerDesigner::Design& d, bool replace_last) {
    if (!d.isOk())
        return;
    using L         = numina::ControllerDesigner::Law;
    const auto& s   = d.settings;
    const auto law  = d.law;
    const bool p_on = law == L::P || law == L::Pd || law == L::Pi || law == L::Pid;
    const bool i_on = law == L::I || law == L::Pi || law == L::Pid;
    const bool d_on = (law == L::Pd || law == L::Pid) && s.td > 0.0 && std::isfinite(s.td);
    apply_controller_params(p_on, s.kp, i_on, s.ti, d_on, s.td, replace_last);
}

void SynthesisTab::onSamplePicked(const C0C1Chart::Sample& sample) {
    const bool replace = !ui->charts->empty();
    if (is_pd_structure()) {
        if (sample.kp > 0.0 && sample.td > 0.0 && std::isfinite(sample.kp) && std::isfinite(sample.td))
            apply_pd_settings(sample.kp, sample.td, replace);
        return;
    }
    if (!(sample.c0 > 0.0) || !(sample.c1 > 0.0) || !(sample.kp > 0.0) || !(sample.ti > 0.0))
        return;
    apply_pi_settings(sample.kp, sample.ti, replace);
}
