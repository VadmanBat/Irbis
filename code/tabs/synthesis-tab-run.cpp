#include "code/dialogs/mod-par-dialog.h"
#include "code/tabs/synthesis-tab.h"
#include "code/util/format.hxx"
#include "code/util/tf-builder.hpp"
#include "numina/classes/control/regulator-designer.h"
#include "numina/classes/control/transfer-function.h"
#include "ui_synthesis-tab.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>
#include <vector>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>

namespace {
/// UI title for chart legend; coeffs come from numina::TransferFunction::makeRegulator.
QString regulator_title(bool p_on, bool i_on, bool d_on, double kp, double tu, double td) {
    const int id = static_cast<int>(p_on) + 2 * static_cast<int>(i_on) + 4 * static_cast<int>(d_on);
    std::ostringstream stream;
    stream << std::fixed;
    stream.precision(2);
    switch (id) {
        case 0:
            stream << "1";
            break;
        case 1:
            stream << "P(" << kp << ")";
            break;
        case 2:
            stream << "I(" << tu << ")";
            break;
        case 3:
            stream << "PI(" << kp << ", " << tu << ")";
            break;
        case 4:
            stream << "D(" << td << ")";
            break;
        case 5:
            stream << "PD(" << kp << ", " << td << ")";
            break;
        case 6:
            stream << "ID(" << tu << ", " << td << ")";
            break;
        case 7:
            stream << "PID(" << kp << ", " << tu << ", " << td << ")";
            break;
        default:
            stream << "1";
            break;
    }
    return QString::fromStdString(stream.str());
}

C0C1Chart::Optimum to_optimum(const numina::RegulatorDesigner::Design& d, const QString& label) {
    C0C1Chart::Optimum o;
    o.valid  = d.isOk();
    o.c0     = d.settings.c0;
    o.c1     = d.settings.c1;
    o.kp     = d.settings.kp;
    o.tu     = d.settings.tu;
    o.omega  = d.settings.omega;
    o.label  = label;
    return o;
}

bool uses_pid_face(const numina::RegulatorDesigner& des, numina::RegulatorDesigner::Law law,
                   numina::RegulatorDesigner::Type w_hi_hint = 1e3) {
    using L = numina::RegulatorDesigner::Law;
    if (law == L::Pid)
        return true;
    return law == L::Auto && des.needsPid(w_hi_hint);
}

std::vector<C0C1Chart::Sample> build_locus(const numina::RegulatorDesigner& des, numina::RegulatorDesigner::Law law,
                                           std::size_t n_points = 160) {
    std::vector<C0C1Chart::Sample> out;
    const bool face = uses_pid_face(des, law);
    double w_hi     = des.maxFrequency(/*w_hi_hint=*/1e3);
    if (!(w_hi > 1e-9) || !std::isfinite(w_hi)) {
        if (!face)
            return out;
        w_hi = 1e3;
    }

    if (n_points < 2)
        n_points = 2;
    out.reserve(n_points);
    for (std::size_t i = 0; i < n_points; ++i) {
        const double t     = static_cast<double>(i) / static_cast<double>(n_points - 1);
        const auto s       = face ? des.settingsOnFace(t * w_hi) : des.settingsAt(t * w_hi);
        if (!std::isfinite(s.c0) || !std::isfinite(s.c1))
            continue;
        C0C1Chart::Sample sample;
        sample.omega = s.omega;
        sample.c0    = s.c0;
        sample.c1    = s.c1;
        sample.c2    = s.c2;
        sample.kp    = s.kp;
        sample.tu    = s.tu;
        sample.td    = s.td;
        out.push_back(sample);
    }
    return out;
}

numina::RegulatorDesigner::Design run_design(const numina::RegulatorDesigner& des,
                                             numina::RegulatorDesigner::Spec spec,
                                             numina::RegulatorDesigner::Criterion criterion) {
    using L = numina::RegulatorDesigner::Law;
    using R = numina::RegulatorDesigner::Region;
    spec.criterion = criterion;
    const bool gamma = spec.region == R::Gamma;
    switch (spec.law) {
        case L::Pid:
            return des.designPid(spec);
        case L::Auto:
            if (des.needsPid(spec.w_hi_hint))
                return des.designPid(spec);
            return gamma ? des.designByGamma(spec) : des.designPi(spec);
        case L::Pi:
        default:
            return gamma ? des.designByGamma(spec) : des.designPi(spec);
    }
}
}

void SynthesisTab::block_param_signals(bool block) {
    for (auto* p : parameters_) {
        p->checkBox()->blockSignals(block);
        p->slider()->blockSignals(block);
    }
}

void SynthesisTab::update_metrics_from_bank() {
    if (!charts_->hasLastQuality() || !charts_->lastQuality().is_settled) {
        metrics_->updateValues({});
        return;
    }
    const auto& q = charts_->lastQuality();
    metrics_->updateValues({
        q.settling_time,
        q.natural_frequency,
        q.steady_state,
        q.iae,
        q.rise_time,
        q.cut_frequency,
        1.0 - q.steady_state,
        q.ise,
        q.peak_time,
        q.damping_ratio,
        q.overshoot_percent,
        q.sigma,
    });
}

void SynthesisTab::openSettings() {
    ModParDialog dialog(model_param_, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    model_param_ = dialog.data();
    if (charts_->empty())
        return;
    try {
        charts_->recomputeAll(model_param_);
        update_metrics_from_bank();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

numina::RegulatorDesigner::Criterion SynthesisTab::selected_criterion() const noexcept {
    using C = numina::RegulatorDesigner::Criterion;
    switch (ui->criterionCombo->currentIndex()) {
        case 1:
            return C::Ikk;
        case 2:
            return C::Sko;
        default:
            return C::Lik;
    }
}

numina::RegulatorDesigner::Law SynthesisTab::selected_law() const noexcept {
    using L = numina::RegulatorDesigner::Law;
    switch (ui->lawCombo->currentIndex()) {
        case 1:
            return L::Pid;
        case 2:
            return L::Auto;
        default:
            return L::Pi;
    }
}

numina::RegulatorDesigner::Region SynthesisTab::selected_region() const noexcept {
    using R = numina::RegulatorDesigner::Region;
    return ui->regionCombo->currentIndex() == 1 ? R::Gamma : R::Rkch;
}

bool SynthesisTab::build_plant(numina::TransferFunction& out) {
    auto plant_num = form_->numerator();
    auto plant_den = form_->denominator();
    if (!tf_builder::validInput(plant_num, plant_den))
        return false;
    out = tf_builder::plant(std::move(plant_num), std::move(plant_den), form_->delayTime(), model_param_.approxOrder);
    plant_tf_ = out;
    form_->setTransferFunction(&plant_tf_);
    return true;
}

void SynthesisTab::sync_c0c1_selection_from_params() {
    if (!c0c1_chart_)
        return;
    // C₀ = Kp/Tu, C₁ = Kp — follows sliders regardless of locus / channel checkboxes.
    const double kp = parameters_[0]->value();
    const double tu = parameters_[1]->value();
    if (kp > 0.0 && tu > 0.0 && std::isfinite(kp) && std::isfinite(tu))
        c0c1_chart_->setSelection(kp / tu, kp);
    else
        c0c1_chart_->clearSelection();
}

void SynthesisTab::apply_pi_settings(double kp, double tu, bool replace_last) {
    if (!(kp > 0.0) || !(tu > 0.0) || !std::isfinite(kp) || !std::isfinite(tu)) {
        // Invalid point on plane — marker already set by chart; do not spam errors while dragging.
        return;
    }
    // Sliders stay fully usable: only expand range if value is outside; never shrink or force D off.
    block_param_signals(true);
    parameters_[0]->ensureValueInRange(kp);
    parameters_[1]->ensureValueInRange(tu);
    if (!parameters_[0]->enabled())
        parameters_[0]->setEnabled(true);
    if (!parameters_[1]->enabled())
        parameters_[1]->setEnabled(true);
    parameters_[0]->setValue(kp);
    parameters_[1]->setValue(tu);
    block_param_signals(false);

    // Selection marker is owned by the chart during drag; avoid setSelection→refit loop.
    apply_current_regulator(replace_last);
}

void SynthesisTab::apply_design(const numina::RegulatorDesigner::Settings& s, bool replace_last) {
    if (!s.isOk())
        return;
    block_param_signals(true);
    parameters_[0]->ensureValueInRange(s.kp);
    parameters_[1]->ensureValueInRange(s.tu);
    parameters_[0]->setEnabled(true);
    parameters_[1]->setEnabled(true);
    parameters_[0]->setValue(s.kp);
    parameters_[1]->setValue(s.tu);
    if (s.td > 0.0 && std::isfinite(s.td)) {
        parameters_[2]->ensureValueInRange(s.td);
        parameters_[2]->setEnabled(true);
        parameters_[2]->setValue(s.td);
    }
    block_param_signals(false);
    apply_current_regulator(replace_last);
}

void SynthesisTab::on_c0c1_sample_picked(const C0C1Chart::Sample& sample) {
    // Chart already moved the marker; apply PI only for positive C₀, C₁.
    if (!(sample.c0 > 0.0) || !(sample.c1 > 0.0) || !(sample.kp > 0.0) || !(sample.tu > 0.0))
        return;
    apply_pi_settings(sample.kp, sample.tu, !charts_->empty());
}

void SynthesisTab::autoSynthesize() {
    numina::TransferFunction plant;
    if (!build_plant(plant)) {
        show_error(tr("Задайте корректную ПФ объекта управления."));
        return;
    }

    try {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        const double phi = ui->phiSpin->value();
        numina::RegulatorDesigner designer(plant, phi);

        numina::RegulatorDesigner::Spec spec;
        spec.phi    = phi;
        spec.law    = selected_law();
        spec.region = selected_region();

        using C = numina::RegulatorDesigner::Criterion;
        using L = numina::RegulatorDesigner::Law;
        using R = numina::RegulatorDesigner::Region;
        // Γ + ПИ: ЛИК numina сводит к ИКК. На грани ПИД ЛИК считается отдельно.
        const bool gamma_pi   = spec.region == R::Gamma && !uses_pid_face(designer, spec.law);
        const auto design_ikk = run_design(designer, spec, C::Ikk);
        const auto design_sko = run_design(designer, spec, C::Sko);
        const auto design_lik = gamma_pi ? numina::RegulatorDesigner::Design{} : run_design(designer, spec, C::Lik);

        C crit = selected_criterion();
        if (gamma_pi && crit == C::Lik)
            crit = C::Ikk;
        const auto* chosen = (crit == C::Sko) ? &design_sko : (crit == C::Ikk) ? &design_ikk : &design_lik;

        const bool face = uses_pid_face(designer, spec.law);
        auto locus      = build_locus(designer, spec.law);
        if (locus.empty() && !face) {
            QApplication::restoreOverrideCursor();
            show_error(tr("Нет области C₀>0, C₁>0 при φ = %1.\n"
                          "ПИ недоступен (Ω_доп пуста). Выберите ПИД или Авто.")
                           .arg(num_format::format(phi, 3)));
            return;
        }

        c0c1_chart_->setLocus(std::move(locus), face ? tr("грань C₂*(ω)") : tr("C₁(ω), C₀(ω)"));
        c0c1_chart_->setOptima(gamma_pi ? C0C1Chart::Optimum{} : to_optimum(design_lik, tr("опт. ЛИК")),
                               to_optimum(design_ikk, tr("опт. ИКК")), to_optimum(design_sko, tr("опт. СКО")));

        QApplication::restoreOverrideCursor();

        if (!chosen->isOk()) {
            if (spec.law == L::Pid)
                show_error(tr("ПИД недоступен: нет настроек с C₂>0.\n"
                              "Если ПИ уже обеспечивает φ, выберите ПИ или Авто."));
            else if (spec.law == L::Pi)
                show_error(tr("ПИ недоступен (C₀, C₁ > 0 и доминирование / сектор Γ).\n"
                              "При пустой Ω_доп выберите ПИД или Авто."));
            else
                show_error(tr("Автонастройка не нашла ни ПИ, ни ПИД."));
            return;
        }

        apply_design(chosen->settings, !charts_->empty());
    }
    catch (const std::exception& ex) {
        QApplication::restoreOverrideCursor();
        show_error(tr("Автосинтез: %1").arg(QString::fromUtf8(ex.what())));
    }
}

void SynthesisTab::apply_current_regulator(bool replace_last) {
    auto plant_num = form_->numerator();
    auto plant_den = form_->denominator();
    if (!tf_builder::validInput(plant_num, plant_den))
        return;

    const bool p_on = parameters_[0]->enabled();
    const bool i_on = parameters_[1]->enabled();
    const bool d_on = parameters_[2]->enabled();
    const double kp = parameters_[0]->value();
    const double tu = parameters_[1]->value();
    const double td = parameters_[2]->value();
    // makeRegulator: channel on only if param > 0; disabled UI channels → ≤ 0.
    auto [reg_num, reg_den] = numina::TransferFunction::makeRegulator(p_on ? kp : -1.0, i_on ? tu : -1.0,
                                                                      d_on ? td : -1.0);

    try {
        const double tau = form_->delayTime();
        const int order  = model_param_.approxOrder;
        plant_tf_        = tf_builder::plant(plant_num, plant_den, tau, order);
        current_tf_ =
            tf_builder::closedLoop(std::move(plant_num), std::move(plant_den), std::move(reg_num).extractCoeffs(),
                                   std::move(reg_den).extractCoeffs(), tau, order);
        form_->setTransferFunction(&plant_tf_);

        const QString title = regulator_title(p_on, i_on, d_on, kp, tu, td);
        if (replace_last && !charts_->empty())
            charts_->replaceLastFromTf(current_tf_, model_param_, title);
        else
            charts_->appendFromTf(current_tf_, model_param_, title);

        update_metrics_from_bank();
        sync_c0c1_selection_from_params();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}
