#include "code/tabs/analysis-tab.h"

#include "code/tabs/tab-shell.hpp"
#include "code/util/tf-builder.hpp"
#include "ui_analysis-tab.h"

#include <QMenu>
#include <QPushButton>

AnalysisTab::AnalysisTab(QWidget* parent) : QWidget(parent), ui(new Ui::AnalysisTab) {
    ui->setupUi(this);
    model_param_.usePadeApprox = false;
    install_custom_widgets();

    charts_menu_ = new QMenu(this);
    tab_ui::wireChartsButton(ui->chartsButton, ui->charts, charts_menu_);

    form_->bindNameLabel(ui->nameLabel);
    form_->setTransferFunction(&current_tf_);

    connect(ui->settingsButton, &QPushButton::clicked, this, &AnalysisTab::openSettings);
    connect(ui->addButton, &QPushButton::clicked, this, &AnalysisTab::addTransferFunction);
    connect(ui->replaceButton, &QPushButton::clicked, this, &AnalysisTab::replaceTransferFunction);
    connect(ui->clearButton, &QPushButton::clicked, this, &AnalysisTab::clearCharts);
}

AnalysisTab::~AnalysisTab() {
    delete ui;
}

void AnalysisTab::install_custom_widgets() {
    form_ = new TranFuncForm(6, 6, QStringLiteral("W(p) = "), ui->formHost);
    form_->setExactDelaySolutions(true);
    metrics_ = new RegulationWidget(3, 2, ui->metricsHost);
    tab_ui::mountInHost(ui->formHost, form_, Qt::AlignLeft | Qt::AlignVCenter);
    tab_ui::mountInHost(ui->metricsHost, metrics_, Qt::AlignRight | Qt::AlignVCenter);
    tab_ui::setupPlantQualityMetrics(metrics_);
}

void AnalysisTab::show_error(const QString& message) {
    tab_ui::showError(this, tr("Ошибка ввода"), message);
}

void AnalysisTab::update_metrics() {
    tab_ui::applySettledPlantMetrics(metrics_, ui->charts);
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

void AnalysisTab::addTransferFunction() {
    auto num = form_->numerator();
    auto den = form_->denominator();
    if (const QString err = tab_ui::plantInputError(num, den); !err.isEmpty()) {
        show_error(err);
        return;
    }

    try {
        current_tf_ = tf_builder::plant(std::move(num), std::move(den));
        form_->setTransferFunction(&current_tf_);
        ui->charts->appendFromTf(current_tf_, model_param_, form_->linkName(), form_->delayTime());
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
    auto num = form_->numerator();
    auto den = form_->denominator();
    if (const QString err = tab_ui::plantInputError(num, den); !err.isEmpty()) {
        show_error(err);
        return;
    }
    try {
        current_tf_ = tf_builder::plant(std::move(num), std::move(den));
        form_->setTransferFunction(&current_tf_);
        ui->charts->replaceLastFromTf(current_tf_, model_param_, form_->linkName(), form_->delayTime());
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
