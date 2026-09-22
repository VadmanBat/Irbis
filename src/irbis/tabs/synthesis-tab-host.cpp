#include "irbis/tabs/synthesis-tab.h"

#include "ui_synthesis-tab.h"

#include <QComboBox>
#include <QSignalBlocker>

namespace {
int law_index(const numina::ControllerLaw law) noexcept {
    using L = numina::ControllerLaw;
    switch (law) {
        case L::P:
            return 0;
        case L::I:
            return 1;
        case L::Pd:
            return 2;
        case L::Pi:
            return 3;
        case L::Pid:
            return 4;
        case L::Auto:
            return 5;
    }
    return 3;
}

void channels(const numina::ControllerLaw law, bool& p, bool& i, bool& d) noexcept {
    using L = numina::ControllerLaw;
    p = law == L::P || law == L::Pd || law == L::Pi || law == L::Pid;
    i = law == L::I || law == L::Pi || law == L::Pid;
    d = law == L::Pd || law == L::Pid;
}
}

bool SynthesisTab::showPlant(std::vector<double> num, std::vector<double> den, const double tau) {
    return apply_plant(std::move(num), std::move(den), tau);
}

void SynthesisTab::showController(const numina::ControllerLaw law, const double kp, const double ti, const double td) {
    {
        const QSignalBlocker block(ui->lawCombo);
        ui->lawCombo->setCurrentIndex(law_index(law));
    }
    bool p = false;
    bool i = false;
    bool d = false;
    if (law == numina::ControllerLaw::Auto) {
        p = kp > 0.0;
        i = ti > 0.0;
        d = td > 0.0;
    }
    else {
        channels(law, p, i, d);
    }
    apply_controller_params(p, kp, i, ti, d, td, true);
    update_stability_region();
}

bool SynthesisTab::hasPlant() const noexcept {
    return has_plant_;
}

SynthesisTab::ControllerReading SynthesisTab::controllerReading() const {
    ControllerReading reading;
    reading.law = selected_law();
    reading.p   = parameters_[0]->enabled();
    reading.i   = parameters_[1]->enabled();
    reading.d   = parameters_[2]->enabled();
    reading.kp  = reading.p ? parameters_[0]->value() : 0.0;
    reading.ti  = reading.i ? parameters_[1]->value() : 0.0;
    reading.td  = reading.d ? parameters_[2]->value() : 0.0;
    return reading;
}

void SynthesisTab::readPlant(std::vector<double>& num, std::vector<double>& den, double& tau) const {
    num = plant_num_;
    den = plant_den_;
    tau = plant_tau_;
}
