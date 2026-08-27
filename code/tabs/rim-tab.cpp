#include "code/tabs/rim-tab.h"

#include "code/charts/chart-panel.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/secondary-text.hxx"
#include "ui_rim-tab.h"

#include <QComboBox>
#include <QPushButton>

RimTab::RimTab(QWidget* parent) : QWidget(parent), ui(new Ui::RimTab) {
    ui->setupUi(this);
    install_custom_widgets();
    secondary_text::apply(ui->statusLabel);
    sync_law_ui();

    connect(ui->lawCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { sync_law_ui(); });
    connect(ui->runButton, &QPushButton::clicked, this, &RimTab::runSimulation);
    connect(ui->continueButton, &QPushButton::clicked, this, &RimTab::continueSimulation);
    connect(ui->clearButton, &QPushButton::clicked, this, &RimTab::resetSimulation);
}

RimTab::~RimTab() {
    delete ui;
}

void RimTab::install_custom_widgets() {
    form_ = new TranFuncForm(6, 6, QStringLiteral("W<sub>ОУ</sub>(p) = "), ui->formHost);
    form_->setExactDelaySolutions(true);
    tab_ui::mountInHost(ui->formHost, form_, Qt::AlignLeft | Qt::AlignVCenter);

    y_chart_ = new ChartPanel(tr("Регулирование y(t)"), tr("t, с"), tr("y(t)"), ui->yChartHost);
    y_chart_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tab_ui::mountInHost(ui->yChartHost, y_chart_, {});
    if (auto* box = qobject_cast<QBoxLayout*>(ui->yChartHost->layout()))
        box->setStretchFactor(y_chart_, 1);

    mu_chart_ = new ChartPanel(tr("Положение ОР μ(t)"), tr("t, с"), tr("μ"), ui->muChartHost);
    mu_chart_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tab_ui::mountInHost(ui->muChartHost, mu_chart_, {});
    if (auto* box = qobject_cast<QBoxLayout*>(ui->muChartHost->layout()))
        box->setStretchFactor(mu_chart_, 1);
}

void RimTab::show_error(const QString& message) {
    tab_ui::showError(this, tr("Ошибка"), message);
}

void RimTab::sync_law_ui() {
    if (session_)
        return;
    const auto law = selected_law();
    ui->kpSpin->setEnabled(rim::hasP(law));
    ui->tiSpin->setEnabled(rim::hasI(law));
    ui->tdSpin->setEnabled(rim::hasD(law));
    ui->kpLabel->setEnabled(rim::hasP(law));
    ui->tiLabel->setEnabled(rim::hasI(law));
    ui->tdLabel->setEnabled(rim::hasD(law));
}

void RimTab::set_run_locked(const bool on) {
    const bool edit = !on;
    ui->lawCombo->setEnabled(edit);
    ui->travelSpin->setEnabled(edit);
    ui->pulseSpin->setEnabled(edit);
    ui->deadzoneSpin->setEnabled(edit);
    ui->filterSpin->setEnabled(edit);
    ui->diffSpin->setEnabled(edit);
    ui->valve0Spin->setEnabled(edit);
    ui->dtSpin->setEnabled(edit);
    if (edit)
        sync_law_ui();
    else {
        ui->kpSpin->setEnabled(false);
        ui->tiSpin->setEnabled(false);
        ui->tdSpin->setEnabled(false);
        ui->kpLabel->setEnabled(false);
        ui->tiLabel->setEnabled(false);
        ui->tdLabel->setEnabled(false);
    }
    ui->continueButton->setEnabled(on && session_ != nullptr);
}

numina::ControlLaw RimTab::selected_law() const noexcept {
    switch (ui->lawCombo->currentIndex()) {
        case 0:
            return numina::ControlLaw::P;
        case 1:
            return numina::ControlLaw::Pd;
        case 2:
            return numina::ControlLaw::Pi;
        case 4:
            return numina::ControlLaw::I;
        case 3:
        default:
            return numina::ControlLaw::Pid;
    }
}

numina::PidSettings RimTab::read_pid_settings() const {
    numina::PidSettings s;
    s.kp          = ui->kpSpin->value();
    s.ti          = ui->tiSpin->value();
    s.td          = ui->tdSpin->value();
    s.pulse_time  = ui->pulseSpin->value();
    s.travel_time = ui->travelSpin->value();
    s.deadzone    = ui->deadzoneSpin->value();
    s.filter_time = ui->filterSpin->value();
    s.valve0      = ui->valve0Spin->value();
    s.diff_time   = ui->diffSpin->value();
    return s;
}
