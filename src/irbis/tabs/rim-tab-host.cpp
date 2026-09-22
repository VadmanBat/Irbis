#include "irbis/tabs/rim-tab.h"

#include "ui_rim-tab.h"

#include <algorithm>
#include <QComboBox>
#include <QSignalBlocker>

namespace {
int law_index(const numina::ControlLaw law) noexcept {
    switch (law) {
        case numina::ControlLaw::P:
            return 0;
        case numina::ControlLaw::Pd:
            return 1;
        case numina::ControlLaw::Pi:
            return 2;
        case numina::ControlLaw::Pid:
            return 3;
        case numina::ControlLaw::I:
            return 4;
    }
    return 2;
}
}

bool RimTab::showPlant(std::vector<double> num, std::vector<double> den, const double tau) {
    return apply_plant(std::move(num), std::move(den), tau);
}

void RimTab::showRegulator(const numina::ControlLaw law, const numina::PidSettings& settings, const double setpoint,
                           const double horizon, const double dt) {
    {
        const QSignalBlocker block(ui->lawCombo);
        ui->lawCombo->setCurrentIndex(law_index(law));
    }
    ui->kpSpin->setValue(settings.kp);
    ui->tiSpin->setValue(settings.ti);
    ui->tdSpin->setValue(settings.td);
    ui->travelSpin->setValue(settings.travel_time);
    ui->pulseSpin->setValue(settings.pulse_time);
    ui->filterSpin->setValue(settings.filter_time);
    ui->deadzoneSpin->setValue(settings.deadzone);
    ui->diffSpin->setValue(settings.diff_time);
    ui->valve0Spin->setValue(std::clamp(settings.valve0, -0.5, 0.5));
    ui->setpointSpin->setValue(setpoint);
    if (horizon > 0.0)
        ui->durationSpin->setValue(horizon);
    if (dt > 0.0)
        ui->dtSpin->setValue(dt);
    sync_law_ui();
}

bool RimTab::hasPlant() const noexcept {
    return has_plant_;
}

numina::ControlLaw RimTab::regulatorLaw() const noexcept {
    return selected_law();
}

numina::PidSettings RimTab::regulator() const {
    return read_pid_settings();
}

double RimTab::setpoint() const {
    return ui->setpointSpin->value();
}

double RimTab::horizon() const {
    return ui->durationSpin->value();
}

double RimTab::sampleStep() const {
    return ui->dtSpin->value();
}

void RimTab::readPlant(std::vector<double>& num, std::vector<double>& den, double& tau) const {
    num = plant_num_;
    den = plant_den_;
    tau = plant_tau_;
}
