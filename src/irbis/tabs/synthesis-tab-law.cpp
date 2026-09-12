#include "irbis/control/controller-design.hpp"
#include "irbis/tabs/synthesis-tab.h"
#include "ui_synthesis-tab.h"

#include <exception>
#include <QComboBox>
#include <QSignalBlocker>
#include <vector>

namespace {
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

int law_index_from_channels(const bool p_on, const bool i_on, const bool d_on) noexcept {
    const int id = static_cast<int>(p_on) + 2 * static_cast<int>(i_on) + 4 * static_cast<int>(d_on);
    switch (id) {
        case 1:
            return 0;
        case 2:
            return 1;
        case 5:
            return 2;
        case 3:
            return 3;
        case 7:
            return 4;
        default:
            return 5;
    }
}
}

void SynthesisTab::apply_law_channels() {
    using L        = numina::ControllerDesigner::Law;
    const auto law = selected_law();
    bool p_on      = false;
    bool i_on      = false;
    bool d_on      = false;
    switch (law) {
        case L::P:
            p_on = true;
            break;
        case L::I:
            i_on = true;
            break;
        case L::Pd:
            p_on = true;
            d_on = true;
            break;
        case L::Pi:
            p_on = true;
            i_on = true;
            break;
        case L::Pid:
            p_on = true;
            i_on = true;
            d_on = true;
            break;
        case L::Auto:
            return;
    }
    block_param_signals(true);
    parameters_[0]->setEnabled(p_on);
    parameters_[1]->setEnabled(i_on);
    parameters_[2]->setEnabled(d_on);
    block_param_signals(false);
}

void SynthesisTab::sync_law_from_params() {
    const int index =
        law_index_from_channels(parameters_[0]->enabled(), parameters_[1]->enabled(), parameters_[2]->enabled());
    const QSignalBlocker block(ui->lawCombo);
    ui->lawCombo->setCurrentIndex(index);
}

void SynthesisTab::update_stability_region() {
    using L        = numina::ControllerDesigner::Law;
    const auto law = selected_law();
    if (law != L::Pi && law != L::Pd) {
        return;
    }

    numina::TransferFunction plant;
    if (!build_plant(plant)) {
        ui->c0c1Chart->setLocus({}, {});
        ui->c0c1Chart->setOptima({}, {}, {});
        return;
    }

    try {
        const double phi = ui->phiSpin->value() / 100.0;
        numina::ControllerDesigner designer(plant, phi);
        ui->c0c1Chart->setPlane(law == L::Pd ? C0C1Chart::Plane::Pd : C0C1Chart::Plane::Pi);

        const auto loc = controller_design::locus(designer, law);
        std::vector<C0C1Chart::Sample> samples;
        samples.reserve(loc.size());
        for (const auto& s : loc)
            samples.push_back(to_sample(s));

        const QString loc_name = law == L::Pd ? tr("ЛРЗ (ПД)") : tr("ЛРЗ");
        ui->c0c1Chart->setLocus(std::move(samples), loc_name);
        ui->c0c1Chart->setOptima({}, {}, {});
        ui->c0c1Chart->requestRefit();
    }
    catch (const std::exception&) {
        ui->c0c1Chart->setLocus({}, {});
        ui->c0c1Chart->setOptima({}, {}, {});
    }
}

void SynthesisTab::on_law_changed() {
    using L        = numina::ControllerDesigner::Law;
    const auto law = selected_law();
    if (law != L::Auto) {
        apply_law_channels();
        update_c0c1_visibility();
        sync_c0c1_selection_from_params();
        refresh_closed_display();
        replaceTransferFunction();
    }
    if (law == L::Pi || law == L::Pd)
        update_stability_region();
}

void SynthesisTab::on_phi_changed() {
    update_stability_region();
}

void SynthesisTab::on_params_toggled() {
    sync_law_from_params();
    update_c0c1_visibility();
    sync_c0c1_selection_from_params();
    refresh_closed_display();
    replaceTransferFunction();
    update_stability_region();
}
