#include "irbis/tabs/analysis-tab.h"

#include "irbis/dialogs/tf-input-dialog.h"
#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/tf-builder.hpp"
#include "irbis/util/tf-clipboard.hpp"
#include "ui_analysis-tab.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <utility>

AnalysisTab::AnalysisTab(QWidget* parent) : QWidget(parent), ui(new Ui::AnalysisTab) {
    tab_ui::ensureFonts();
    ui->setupUi(this);
    model_param_.usePadeApprox = false;
    install_custom_widgets();

    connect(ui->addButton, &QPushButton::clicked, this, &AnalysisTab::addTransferFunction);
    connect(ui->replaceButton, &QPushButton::clicked, this, &AnalysisTab::replaceTransferFunction);
    connect(ui->clearButton, &QPushButton::clicked, this, &AnalysisTab::clearCharts);
}

AnalysisTab::~AnalysisTab() {
    delete ui;
}

void AnalysisTab::install_custom_widgets() {
    panel_   = new TfFormulaPanel(ui->formHost);
    metrics_ = new RegulationWidget(3, 2, ui->metricsHost);
    panel_->setTitle(QStringLiteral("W(p) = "));
    panel_->setExactDelaySolutions(true);
    panel_->setPasteVisible(true);
    tab_ui::mountInHost(ui->formHost, panel_, Qt::AlignLeft, 1);
    tab_ui::mountInHost(ui->metricsHost, metrics_, Qt::AlignRight, 1);
    for (QPushButton* btn : {ui->addButton, ui->replaceButton, ui->clearButton})
        btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    tab_ui::setupPlantQualityMetrics(metrics_);
    connect(panel_, &TfFormulaPanel::editRequested, this, &AnalysisTab::editPlant);
    connect(panel_, &TfFormulaPanel::pasteRequested, this, &AnalysisTab::pastePlant);
}

void AnalysisTab::show_error(const QString& message) {
    tab_ui::showError(this, tr("Ошибка ввода"), message);
}

void AnalysisTab::update_metrics() {
    tab_ui::applySettledPlantMetrics(metrics_, ui->charts);
}

bool AnalysisTab::apply_plant(std::vector<double> num, std::vector<double> den, double tau) {
    if (const QString err = tab_ui::plantInputError(num, den); !err.isEmpty()) {
        show_error(err);
        return false;
    }
    current_tf_ = tf_builder::plant(num, den);
    delay_      = tau < 0.0 ? 0.0 : tau;
    panel_->setTransferFunction(current_tf_, delay_);
    return true;
}

void AnalysisTab::openHelp() {
    tab_ui::showHelp(this, HelpDialog::Topic::Analysis);
}

void AnalysisTab::openSettings() {
    if (!tab_ui::editModelParam(this, model_param_, /*allowIdealDelay=*/true) || ui->charts->empty())
        return;
    try {
        ui->charts->recomputeAll(model_param_);
        update_metrics();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

void AnalysisTab::openChartSettings() {
    tab_ui::editChartVisibility(this, ui->charts);
}

void AnalysisTab::editPlant() {
    auto num    = panel_->display()->numerator();
    auto den    = panel_->display()->denominator();
    double tau  = panel_->display()->delay();
    if (!TfInputDialog::edit(this, num, den, tau, QStringLiteral("W(p)")))
        return;
    apply_plant(std::move(num), std::move(den), tau);
}

void AnalysisTab::pastePlant() {
    const auto data = tf_clipboard::parse(QApplication::clipboard()->text());
    if (!data.ok) {
        QMessageBox::information(this, tr("Вставка ПФ"),
                                 tr("В буфере нет данных формата Irbis-TF-v1.\n"
                                    "Скопируйте ПФ кнопкой «Копировать»."));
        return;
    }
    apply_plant(data.num, data.den, data.tau);
}

void AnalysisTab::addTransferFunction() {
    if (panel_->isEmpty()) {
        show_error(tr("Задайте передаточную функцию (кнопка «Изменить»)."));
        return;
    }
    try {
        ui->charts->appendFromTf(current_tf_, model_param_, panel_->linkName(), delay_);
        update_metrics();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

void AnalysisTab::replaceTransferFunction() {
    if (ui->charts->empty()) {
        addTransferFunction();
        return;
    }
    if (panel_->isEmpty()) {
        show_error(tr("Задайте передаточную функцию (кнопка «Изменить»)."));
        return;
    }
    try {
        ui->charts->replaceLastFromTf(current_tf_, model_param_, panel_->linkName(), delay_);
        update_metrics();
    }
    catch (const std::exception& ex) {
        show_error(QString::fromUtf8(ex.what()));
    }
}

void AnalysisTab::clearCharts() {
    ui->charts->clearAll();
    metrics_->updateValues({});
}
