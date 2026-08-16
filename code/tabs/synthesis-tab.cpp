#include "code/tabs/synthesis-tab.h"

#include "code/dialogs/help-dialog.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/dialog-icons.hxx"
#include "ui_synthesis-tab.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>

SynthesisTab::SynthesisTab(QWidget* parent) : QWidget(parent), ui(new Ui::SynthesisTab) {
    ui->setupUi(this);
    ui->regulatorFaceLayout->setAlignment(ui->faceCoeffGrid, Qt::AlignLeft);
    install_custom_widgets();
    setup_metrics();

    ui->charts->setTransientTitle(tr("Переходный процесс"));

    charts_menu_ = new QMenu(this);
    tab_ui::wireChartsButton(ui->chartsButton, ui->charts, charts_menu_);

    form_->setTransferFunction(&plant_tf_);

    connect(ui->helpButton, &QPushButton::clicked, this, &SynthesisTab::openHelp);
    connect(ui->settingsButton, &QPushButton::clicked, this, &SynthesisTab::openSettings);
    connect(ui->autoSynthButton, &QPushButton::clicked, this, &SynthesisTab::autoSynthesize);
    connect(ui->addButton, &QPushButton::clicked, this, &SynthesisTab::addTransferFunction);
    connect(ui->clearButton, &QPushButton::clicked, this, &SynthesisTab::clearCharts);
    connect(ui->c0c1Chart, &C0C1Chart::samplePicked, this, &SynthesisTab::onSamplePicked);

    auto setup_view_btn = [](QToolButton* btn, QChar glyph) {
        dialog_icons::applyGlyph(btn, glyph);
        btn->setFixedSize(22, 22);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        btn->setFocusPolicy(Qt::TabFocus);
        btn->setAutoRaise(false);
    };
    setup_view_btn(ui->viewPlaneButton, QChar(0xf201)); // chart-line
    setup_view_btn(ui->viewFaceButton, QChar(0xf52c));  // equals

    auto* view_group = new QButtonGroup(this);
    view_group->setExclusive(true);
    view_group->addButton(ui->viewPlaneButton);
    view_group->addButton(ui->viewFaceButton);
    connect(ui->viewPlaneButton, &QAbstractButton::toggled, this, [this](bool on) {
        if (!on)
            return;
        show_plane_ = true;
        update_c0c1_visibility();
    });
    connect(ui->viewFaceButton, &QAbstractButton::toggled, this, [this](bool on) {
        if (!on)
            return;
        show_plane_ = false;
        update_c0c1_visibility();
    });
    update_c0c1_visibility();
}

SynthesisTab::~SynthesisTab() {
    delete ui;
}

void SynthesisTab::install_custom_widgets() {
    form_    = new TranFuncForm(6, 6, QStringLiteral("W<sub>ОУ</sub>(p) = "), ui->formHost);
    metrics_ = new RegulationWidget(3, 4, ui->metricsHost);
    tab_ui::mountInHost(ui->formHost, form_, Qt::AlignLeft | Qt::AlignVCenter);
    tab_ui::mountInHost(ui->metricsHost, metrics_, Qt::AlignRight | Qt::AlignVCenter);

    parameters_ = {
        new RegParameter(QStringLiteral("K<sub>p</sub>"), 0.01, 2000, 0.01, 3, this),
        new RegParameter(QStringLiteral("T<sub>i</sub>"), 0.01, 2000, 1, 120, this),
        new RegParameter(QStringLiteral("T<sub>d</sub>"), 0.01, 2000, 1, 60, this),
    };
    parameters_[0]->setValue(1);
    parameters_[1]->setValue(30);
    int row = 0;
    for (auto* p : parameters_) {
        p->placeIn(ui->paramsLayout, row++);
        connect(p->checkBox(), &QCheckBox::toggled, this, [this](bool) {
            update_c0c1_visibility();
            sync_c0c1_selection_from_params();
            replaceTransferFunction();
        });
        connect(p, &RegParameter::valueChanged, this, [this](double) {
            update_regulator_face();
            sync_c0c1_selection_from_params();
            replaceTransferFunction();
        });
    }
    ui->paramsLayout->setColumnStretch(3, 1);
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
    tab_ui::showError(this, tr("Ошибка ввода"), message);
}

void SynthesisTab::openHelp() {
    HelpDialog dialog(this);
    dialog.exec();
}

void SynthesisTab::addTransferFunction() {
    apply_current_controller(false);
}

void SynthesisTab::replaceTransferFunction() {
    if (ui->charts->empty())
        return;
    apply_current_controller(true);
}

void SynthesisTab::clearCharts() {
    ui->charts->clearAll();
    metrics_->updateValues({});
    ui->c0c1Chart->clear();
}
