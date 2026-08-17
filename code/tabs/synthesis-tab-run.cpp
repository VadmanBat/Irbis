#include "code/tabs/synthesis-tab.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/tf-builder.hpp"
#include "ui_synthesis-tab.h"

#include <QCheckBox>
#include <QComboBox>
#include <sstream>
#include <utility>

namespace {
QString controller_title(bool p_on, bool i_on, bool d_on, double kp, double ti, double td) {
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
            stream << "I(" << ti << ")";
            break;
        case 3:
            stream << "PI(" << kp << ", " << ti << ")";
            break;
        case 4:
            stream << "D(" << td << ")";
            break;
        case 5:
            stream << "PD(" << kp << ", " << td << ")";
            break;
        case 6:
            stream << "ID(" << ti << ", " << td << ")";
            break;
        case 7:
            stream << "PID(" << kp << ", " << ti << ", " << td << ")";
            break;
        default:
            stream << "1";
            break;
    }
    return QString::fromStdString(stream.str());
}
}

void SynthesisTab::block_param_signals(bool block) {
    for (auto* p : parameters_)
        p->blockUiSignals(block);
}

void SynthesisTab::update_metrics_from_bank() {
    if (!ui->charts->hasLastQuality() || !ui->charts->lastQuality().is_settled) {
        metrics_->updateValues({});
        return;
    }
    const auto& q = ui->charts->lastQuality();
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
    if (!tab_ui::editModelParam(this, model_param_) || ui->charts->empty())
        return;
    try {
        // Padé order is baked into the delay factor of the closed loop, not into
        // the time/freq grid. Rebuild the last series from form + controller + τ.
        apply_current_controller(true);
        ui->charts->recomputeAll(model_param_);
        update_metrics_from_bank();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

numina::ControllerDesigner::Criterion SynthesisTab::selected_criterion() const noexcept {
    using C = numina::ControllerDesigner::Criterion;
    switch (ui->criterionCombo->currentIndex()) {
        case 1:
            return C::Ikk;
        case 2:
            return C::Sko;
        default:
            return C::Lik;
    }
}

numina::ControllerDesigner::Law SynthesisTab::selected_law() const noexcept {
    using L = numina::ControllerDesigner::Law;
    switch (ui->lawCombo->currentIndex()) {
        case 1:
            return L::Pid;
        case 2:
            return L::Auto;
        default:
            return L::Pi;
    }
}

numina::ControllerDesigner::Region SynthesisTab::selected_region() const noexcept {
    using R = numina::ControllerDesigner::Region;
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

void SynthesisTab::apply_current_controller(bool replace_last) {
    update_c0c1_visibility();
    auto plant_num = form_->numerator();
    auto plant_den = form_->denominator();
    if (!tf_builder::validInput(plant_num, plant_den))
        return;

    const bool p_on = parameters_[0]->enabled();
    const bool i_on = parameters_[1]->enabled();
    const bool d_on = parameters_[2]->enabled();
    const double kp = parameters_[0]->value();
    const double ti = parameters_[1]->value();
    const double td = parameters_[2]->value();
    auto [ctrl_num, ctrl_den] =
        numina::TransferFunction::makeController(p_on ? kp : -1.0, i_on ? ti : -1.0, d_on ? td : -1.0);

    try {
        const double tau = form_->delayTime();
        const int order  = model_param_.approxOrder;
        plant_tf_        = tf_builder::plant(plant_num, plant_den, tau, order);
        current_tf_ =
            tf_builder::closedLoop(std::move(plant_num), std::move(plant_den), std::move(ctrl_num).extractCoeffs(),
                                   std::move(ctrl_den).extractCoeffs(), tau, order);
        form_->setTransferFunction(&plant_tf_);

        const QString title  = controller_title(p_on, i_on, d_on, kp, ti, td);
        const bool had       = !ui->charts->empty();
        const bool appending = !(replace_last && had);
        if (!appending)
            ui->charts->replaceLastFromTf(current_tf_, model_param_, title);
        else
            ui->charts->appendFromTf(current_tf_, model_param_, title);

        if (appending && had)
            ui->c0c1Chart->pinSelection(title);
        const std::size_t n = ui->charts->seriesCount();
        ui->c0c1Chart->setLiveIndex(n == 0 ? 0 : n - 1);
        ui->c0c1Chart->trimPins(n == 0 ? 0 : n - 1);

        update_metrics_from_bank();
        sync_c0c1_selection_from_params();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}
