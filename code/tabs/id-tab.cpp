#include "code/tabs/id-tab.h"

#include "code/charts/chart-panel.h"
#include "code/dialogs/tran-func-dialog.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/secondary-text.hxx"
#include "ui_id-tab.h"

#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QPoint>
#include <QPushButton>
#include <QRect>
#include <QSpinBox>
#include <QToolTip>

IdTab::IdTab(QWidget* parent) : QWidget(parent), ui(new Ui::IdTab) {
    ui->setupUi(this);
    install_custom_widgets();

    secondary_text::applyAll({ui->structHint, ui->delayHint, ui->fileLabel});

    connect(ui->openFileButton, &QPushButton::clicked, this, &IdTab::openFile);
    connect(ui->identifyButton, &QPushButton::clicked, this, &IdTab::runIdentification);
    connect(ui->clearButton, &QPushButton::clicked, this, &IdTab::clearAll);
    connect(ui->copyTfButton, &QPushButton::clicked, this, &IdTab::copyIdentifiedTf);
    connect(ui->equationsButton, &QPushButton::clicked, this, &IdTab::showEquations);
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

void IdTab::install_custom_widgets() {
    display_ = new TfDisplayWidget(QStringLiteral("W(p) = "), ui->formHost);
    tab_ui::mountInHost(ui->formHost, display_, Qt::AlignLeft | Qt::AlignVCenter);
    connect(display_, &TfDisplayWidget::contentsChanged, this, &IdTab::sync_tf_actions);
    sync_tf_actions();

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
    if (!display_ || !display_->isEmpty())
        return;
    const bool astatic = ui->plantKindCombo->currentIndex() == static_cast<int>(IdSettings::PlantKind::Astatic);
    if (astatic || ui->autoOrderCheck->isChecked()) {
        display_->clear();
        return;
    }
    display_->setStructureTemplate(ui->numOrderSpin->value(), ui->denOrderSpin->value());
}

void IdTab::sync_tf_actions() {
    const bool on = display_ && !display_->isEmpty();
    ui->copyTfButton->setEnabled(on);
    ui->equationsButton->setEnabled(on);
}

void IdTab::copyIdentifiedTf() {
    if (!display_ || display_->isEmpty())
        return;
    display_->copyToClipboard();
    QToolTip::showText(ui->copyTfButton->mapToGlobal(QPoint(0, ui->copyTfButton->height())), tr("ПФ скопирована"),
                       ui->copyTfButton, QRect(), 1500);
}

void IdTab::showEquations() {
    if (!display_ || display_->isEmpty())
        return;
    TranFuncDialog dialog(display_->transferFunction(), this, display_->delay());
    dialog.exec();
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
    display_->clear();
    maybe_show_structure_template();
    if (chart_)
        chart_->clearCurves();
    ui->fileLabel->setText(tr("Файл не выбран"));
    ui->fileLabel->setToolTip({});
}
