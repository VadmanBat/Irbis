#include "code/tabs/synthesis-tab.h"

#include "code/dialogs/help-dialog.h"
#include "code/dialogs/mod-par-dialog.h"
#include "code/tabs/tab-charts-button.hpp"
#include "ui_synthesis-tab.h"

#include <QCheckBox>
#include <QMenu>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

SynthesisTab::SynthesisTab(QWidget* parent) : QWidget(parent), ui(new Ui::SynthesisTab) {
    ui->setupUi(this);
    install_custom_widgets();
    setup_metrics();

    charts_ = new ResponseChartBank(this);
    charts_->setTransientTitle(tr("Переходный процесс"));
    c0c1_chart_ = new C0C1Chart(this);
    ui->chartsLayout->addWidget(charts_, /*stretch=*/3);
    ui->chartsLayout->addWidget(c0c1_chart_, /*stretch=*/2);

    ui->verticalLayout->setStretch(0, 0);
    ui->verticalLayout->setStretch(1, 0);
    ui->verticalLayout->setStretch(2, 0);
    ui->verticalLayout->setStretch(3, 1);

    charts_menu_     = new QMenu(this);
    auto* charts_btn = tab_ui::makeChartsButton(this, charts_, charts_menu_);
    ui->buttonLayout->insertWidget(ui->buttonLayout->indexOf(ui->settingsButton), charts_btn);

    form_->setTransferFunction(&plant_tf_);

    connect(ui->helpButton, &QPushButton::clicked, this, &SynthesisTab::openHelp);
    connect(ui->settingsButton, &QPushButton::clicked, this, &SynthesisTab::openSettings);
    connect(ui->autoSynthButton, &QPushButton::clicked, this, &SynthesisTab::autoSynthesize);
    connect(ui->addButton, &QPushButton::clicked, this, &SynthesisTab::addTransferFunction);
    connect(ui->clearButton, &QPushButton::clicked, this, &SynthesisTab::clearCharts);
    connect(c0c1_chart_, &C0C1Chart::samplePicked, this, &SynthesisTab::on_c0c1_sample_picked);
}

SynthesisTab::~SynthesisTab() {
    delete ui;
}

void SynthesisTab::install_custom_widgets() {
    form_             = new TranFuncForm(6, 6, QStringLiteral("W<sub>ОУ</sub>(p) = "), ui->formHost);
    auto* form_layout = new QVBoxLayout(ui->formHost);
    form_layout->setContentsMargins(0, 0, 0, 0);
    form_layout->addWidget(form_, 0, Qt::AlignLeft | Qt::AlignVCenter);

    metrics_             = new RegulationWidget(3, 4, ui->metricsHost);
    auto* metrics_layout = new QVBoxLayout(ui->metricsHost);
    metrics_layout->setContentsMargins(0, 0, 0, 0);
    metrics_layout->addWidget(metrics_, 0, Qt::AlignRight | Qt::AlignVCenter);
    ui->topLayout->insertStretch(1, 1);

    parameters_ = {
        new RegParameter(QStringLiteral("K<sub>p</sub>"), 0.05, 2000, 0.05, 5, this),
        new RegParameter(QStringLiteral("T<sub>u</sub>"), 0.05, 2000, 1, 120, this),
        new RegParameter(QStringLiteral("T<sub>d</sub>"), 0.05, 2000, 1, 60, this),
    };
    ui->paramsLayout->setSpacing(0);
    ui->paramsLayout->setContentsMargins(0, 0, 0, 0);
    ui->paramsLayout->setSizeConstraint(QLayout::SetMinimumSize);
    for (auto* p : parameters_) {
        ui->paramsLayout->addLayout(p->layout());
        connect(p->checkBox(), &QCheckBox::toggled, this, [this](bool) {
            sync_c0c1_selection_from_params();
            replaceTransferFunction();
        });
        connect(p->slider(), &DoubleSlider::doubleValueChanged, this, [this](double) {
            sync_c0c1_selection_from_params();
            replaceTransferFunction();
        });
    }
}

void SynthesisTab::setup_metrics() {
    metrics_->setLabels(
        {
            QStringLiteral("t<sub>р</sub>:"),
            QStringLiteral("ω<sub>n</sub>:"),
            QStringLiteral("h<sub>уст</sub>:"),
            QStringLiteral("ЛИК:"),
            QStringLiteral("t<sub>н</sub>:"),
            QStringLiteral("ω<sub>c</sub>:"),
            QStringLiteral("σ<sub>ст</sub>:"),
            QStringLiteral("ИКК:"),
            QStringLiteral("t<sub>п</sub>:"),
            QStringLiteral("ζ:"),
            QStringLiteral("σ<sub>пр</sub>:"),
            QStringLiteral("СКО:"),
        },
        {
            tr("Время регулирования, с"),
            tr("Собственная частота, рад/с"),
            tr("Установившееся значение"),
            tr("Линейный интегральный критерий"),
            tr("Время нарастания, с"),
            tr("Частота среза, рад/с"),
            tr("Статическая ошибка"),
            tr("Интегральный квадратичный критерий (ИКК)"),
            tr("Время пика, с"),
            tr("Коэффициент демпфирования, %"),
            tr("Перерегулирование, %"),
            tr("RMS ошибки разгона на [0, tр] (не стохастическое СКО настройки)"),
        });
    metrics_->setColors(
        {{1, 2}, {0, 0}, {0, 0}, {1, 2}, {1, 2}, {0, 0}, {0, 0}, {1, 2}, {1, 2}, {2, 1}, {1, 2}, {1, 2}});
}

void SynthesisTab::show_error(const QString& message) {
    QMessageBox::critical(this, tr("Ошибка ввода"), message);
}

void SynthesisTab::openHelp() {
    HelpDialog dialog(this);
    dialog.exec();
}

void SynthesisTab::addTransferFunction() {
    apply_current_regulator(false);
}

void SynthesisTab::replaceTransferFunction() {
    if (charts_->empty())
        return;
    apply_current_regulator(true);
}

void SynthesisTab::clearCharts() {
    charts_->clearAll();
    metrics_->updateValues({});
    if (c0c1_chart_)
        c0c1_chart_->clear();
}
