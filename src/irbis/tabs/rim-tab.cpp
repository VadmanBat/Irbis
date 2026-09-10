#include "irbis/tabs/rim-tab.h"

#include "irbis/charts/chart-panel.h"
#include "irbis/dialogs/tf-input-dialog.h"
#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/secondary-text.hxx"
#include "irbis/util/tf-builder.hpp"
#include "irbis/util/tf-clipboard.hpp"
#include "ui_rim-tab.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QMessageBox>
#include <QPushButton>
#include <utility>

RimTab::RimTab(QWidget* parent) : QWidget(parent), ui(new Ui::RimTab) {
    tab_ui::ensureFonts();
    ui->setupUi(this);
    install_custom_widgets();
    secondary_text::apply(ui->statusLabel);
    sync_law_ui();

    connect(ui->lawCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { sync_law_ui(); });
    connect(ui->runButton, &QPushButton::clicked, this, &RimTab::runSimulation);
    connect(ui->continueButton, &QPushButton::clicked, this, &RimTab::continueSimulation);
    connect(ui->clearButton, &QPushButton::clicked, this, &RimTab::resetSimulation);
    connect(panel_, &TfFormulaPanel::editRequested, this, &RimTab::editPlant);
    connect(panel_, &TfFormulaPanel::pasteRequested, this, &RimTab::pastePlant);
}

RimTab::~RimTab() {
    delete ui;
}

void RimTab::install_custom_widgets() {
    panel_ = new TfFormulaPanel(ui->formHost);
    panel_->setTitle(QStringLiteral("W<sub>ОУ</sub>(p) = "));
    panel_->setExactDelaySolutions(true);
    panel_->setPasteVisible(true);
    panel_->setCardFrame(false);
    tab_ui::mountInHost(ui->formHost, panel_, Qt::AlignLeft | Qt::AlignVCenter);

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

bool RimTab::apply_plant(std::vector<double> num, std::vector<double> den, double tau) {
    if (const QString err = tab_ui::plantInputError(num, den); !err.isEmpty()) {
        show_error(err);
        return false;
    }
    plant_num_ = std::move(num);
    plant_den_ = std::move(den);
    plant_tau_ = tau < 0.0 ? 0.0 : tau;
    has_plant_ = true;
    panel_->setTransferFunction(tf_builder::plant(plant_num_, plant_den_), plant_tau_);
    return true;
}

void RimTab::editPlant() {
    auto num   = plant_num_;
    auto den   = plant_den_;
    double tau = plant_tau_;
    if (!TfInputDialog::edit(this, num, den, tau, QStringLiteral("W<sub>ОУ</sub>(p)")))
        return;
    apply_plant(std::move(num), std::move(den), tau);
}

void RimTab::pastePlant() {
    const auto data = tf_clipboard::parse(QApplication::clipboard()->text());
    if (!data.ok) {
        QMessageBox::information(this, tr("Вставка ПФ"),
                                 tr("В буфере нет данных формата Irbis-TF-v1.\n"
                                    "Скопируйте ПФ кнопкой «Копировать»."));
        return;
    }
    apply_plant(data.num, data.den, data.tau);
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
