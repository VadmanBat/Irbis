#include "irbis/tabs/id-tab.h"

#include "irbis/charts/chart-panel.h"
#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/secondary-text.hxx"
#include "irbis/widgets/tf-display-widget.h"
#include "ui_id-tab.h"

#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QPushButton>
#include <QSpinBox>

IdTab::IdTab(QWidget* parent) : QWidget(parent), ui(new Ui::IdTab) {
    tab_ui::ensureFonts();
    ui->setupUi(this);
    install_custom_widgets();

    secondary_text::applyAll({ui->structHint, ui->delayHint, ui->fileLabel});

    connect(ui->openFileButton, &QPushButton::clicked, this, &IdTab::openFile);
    connect(ui->identifyButton, &QPushButton::clicked, this, &IdTab::runIdentification);
    connect(ui->clearButton, &QPushButton::clicked, this, &IdTab::clearAll);
    connect(ui->plantKindCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { sync_plant_kind_ui(); });
    connect(ui->autoOrderCheck, &QCheckBox::toggled, this, [this](bool) { sync_struct_ui(); });
    connect(ui->estimateTauCheck, &QCheckBox::toggled, this, [this](bool on) {
        ui->delayHint->setText(on ? tr("τ будет оценено по данным.") : tr("Модель без запаздывания (τ = 0)."));
        secondary_text::apply(ui->delayHint);
    });
    connect(ui->denOrderSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int n) {
        if (ui->numOrderSpin->value() > n)
            ui->numOrderSpin->setValue(n);
        ui->numOrderSpin->setMaximum(n);
        maybe_show_structure_template();
    });
    connect(ui->numOrderSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int) { maybe_show_structure_template(); });
    connect(ui->methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (file_path_.isEmpty())
            return;
        if (preview_loaded_file())
            ui->fileLabel->setText(QFileInfo(file_path_).fileName());
    });

    ui->numOrderSpin->setMaximum(ui->denOrderSpin->value());
    sync_plant_kind_ui();
    sync_struct_ui();
}

IdTab::~IdTab() {
    delete ui;
}

void IdTab::openHelp() {
    tab_ui::showHelp(this, HelpDialog::Topic::Identification);
}

void IdTab::install_custom_widgets() {
    panel_ = new TfFormulaPanel(ui->formHost);
    panel_->setCardFrame(false);
    panel_->setEditVisible(false);
    panel_->setPasteVisible(false);
    panel_->setExactDelaySolutions(true);
    tab_ui::mountInHost(ui->formHost, panel_, Qt::AlignLeft | Qt::AlignVCenter);

    chart_ = new ChartPanel(tr("h(t): эксперимент / модель"), tr("t, с"), tr("h(t)"), ui->chartHost);
    chart_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tab_ui::mountInHost(ui->chartHost, chart_, {});
    if (auto* box = qobject_cast<QBoxLayout*>(ui->chartHost->layout()))
        box->setStretchFactor(chart_, 1);
}

void IdTab::show_error(const QString& message) {
    tab_ui::showError(this, tr("Ошибка"), message);
}

IdSettings IdTab::read_id_settings() const {
    IdSettings s;
    s.plantKind   = static_cast<IdSettings::PlantKind>(ui->plantKindCombo->currentIndex());
    s.autoOrder   = ui->autoOrderCheck->isChecked();
    s.denOrder    = ui->denOrderSpin->value();
    s.numOrder    = std::min(ui->numOrderSpin->value(), s.denOrder);
    s.estimateTau = ui->estimateTauCheck->isChecked();
    return s;
}

void IdTab::sync_plant_kind_ui() {
    const bool astatic = ui->plantKindCombo->currentIndex() == static_cast<int>(IdSettings::PlantKind::Astatic);
    ui->structGroup->setVisible(!astatic);
    maybe_show_structure_template();
}

void IdTab::sync_struct_ui() {
    const bool auto_on = ui->autoOrderCheck->isChecked();
    ui->denOrderSpin->setEnabled(!auto_on);
    ui->numOrderSpin->setEnabled(!auto_on);
    ui->denOrderLabel->setEnabled(!auto_on);
    ui->numOrderLabel->setEnabled(!auto_on);
    ui->structHint->setText(auto_on ? tr("Структура подбирается автоматически.")
                                    : tr("Задайте порядки знаменателя и числителя"));
    secondary_text::apply(ui->structHint);
    maybe_show_structure_template();
}

void IdTab::maybe_show_structure_template() {
    if (!panel_->isEmpty())
        return;
    const bool astatic = ui->plantKindCombo->currentIndex() == static_cast<int>(IdSettings::PlantKind::Astatic);
    if (astatic || ui->autoOrderCheck->isChecked()) {
        panel_->clear();
        return;
    }
    panel_->display()->setStructureTemplate(ui->numOrderSpin->value(), ui->denOrderSpin->value());
}

void IdTab::openFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Открыть файл данных"), {},
                                                      tr("Файлы данных (*.txt *.csv *.dat);;Все файлы (*)"));
    if (path.isEmpty())
        return;
    file_path_ = path;
    ui->fileLabel->setToolTip(path);
    if (!preview_loaded_file()) {
        file_path_.clear();
        has_data_ = false;
        ui->fileLabel->setText(tr("Файл не выбран"));
        ui->fileLabel->setToolTip({});
        return;
    }
    ui->fileLabel->setText(QFileInfo(path).fileName());
}

void IdTab::clearAll() {
    has_data_ = false;
    file_path_.clear();
    step_series_.clear();
    valve_series_.clear();
    signal_series_.clear();
    panel_->clear();
    maybe_show_structure_template();
    if (chart_)
        chart_->clearCurves();
    ui->fileLabel->setText(tr("Файл не выбран"));
    ui->fileLabel->setToolTip({});
}
