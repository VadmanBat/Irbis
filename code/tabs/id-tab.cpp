#include "code/tabs/id-tab.h"

#include "code/dialogs/id-settings-dialog.h"
#include "code/tabs/tab-shell.hpp"
#include "ui_id-tab.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QPushButton>

IdTab::IdTab(QWidget* parent) : QWidget(parent), ui(new Ui::IdTab) {
    ui->setupUi(this);
    install_custom_widgets();

    charts_menu_ = new QMenu(this);
    tab_ui::wireChartsButton(ui->chartsButton, ui->charts, charts_menu_);

    connect(ui->openFileButton, &QPushButton::clicked, this, &IdTab::openFile);
    connect(ui->identifyButton, &QPushButton::clicked, this, &IdTab::runIdentification);
    connect(ui->clearButton, &QPushButton::clicked, this, &IdTab::clearAll);
    connect(ui->settingsButton, &QPushButton::clicked, this, &IdTab::openSettings);
    connect(ui->idSettingsButton, &QPushButton::clicked, this, &IdTab::openIdSettings);
    connect(ui->methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (has_data_)
            ui->fileLabel->setText(tr("%1 (нажмите ▶ для пересчёта)").arg(QFileInfo(file_path_).fileName()));
    });
}

IdTab::~IdTab() {
    delete ui;
}

void IdTab::install_custom_widgets() {
    display_ = new TfDisplayWidget(QStringLiteral("W(p) = "), ui->formHost);
    metrics_ = new RegulationWidget(3, 2, ui->metricsHost);
    tab_ui::mountInHost(ui->formHost, display_, Qt::AlignLeft | Qt::AlignVCenter);
    tab_ui::mountInHost(ui->metricsHost, metrics_, Qt::AlignRight | Qt::AlignVCenter);
    tab_ui::setupPlantQualityMetrics(metrics_);
}

void IdTab::show_error(const QString& message) {
    tab_ui::showError(this, tr("Ошибка"), message);
}

void IdTab::openSettings() {
    if (!tab_ui::editModelParam(this, model_param_))
        return;
    // Grid/Padé-sample settings only: recompute responses, keep identified plant.
    if (!ui->charts->empty()) {
        ui->charts->recomputeAll(model_param_);
        tab_ui::applySettledPlantMetrics(metrics_, ui->charts);
    }
}

void IdTab::openIdSettings() {
    IdSettingsDialog dialog(id_settings_, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    id_settings_ = dialog.data();
    if (has_data_)
        runIdentification();
}

void IdTab::openFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Открыть файл данных"), {},
                                                      tr("Файлы данных (*.txt *.csv *.dat);;Все файлы (*)"));
    if (path.isEmpty())
        return;
    file_path_ = path;
    has_data_  = false; // set true only after successful load in runIdentification
    ui->fileLabel->setText(QFileInfo(path).fileName());
    ui->fileLabel->setToolTip(path);
    runIdentification();
}

void IdTab::clearAll() {
    has_data_ = false;
    file_path_.clear();
    step_series_.clear();
    valve_series_.clear();
    signal_series_.clear();
    display_->clear();
    ui->charts->clearAll();
    metrics_->updateValues({});
    ui->fileLabel->setText(tr("Файл не выбран"));
    ui->fileLabel->setToolTip({});
}
