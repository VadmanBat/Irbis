#include "code/dialogs/slider-settings-dialog.h"

#include "code/util/dialog-icons.hxx"
#include "code/util/format.hxx"
#include "code/util/secondary-text.hxx"
#include "ui_slider-settings-dialog.h"

#include <algorithm>
#include <QPushButton>
#include <QTextDocumentFragment>
#include <utility>

SliderSettingsDialog::SliderSettingsDialog(const QString& param_name, double hard_min, double hard_max,
                                           const SliderSettings& values, const SliderSettings& defaults,
                                           QWidget* parent)
    : QDialog(parent), ui(new Ui::SliderSettingsDialog), defaults_(defaults) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::SliderSettings);
    secondary_text::apply(ui->valueLabel);
    setMinimumWidth(320);
    resize(380, height());

    ui->paramTitle->setTextFormat(Qt::RichText);
    ui->paramTitle->setText(param_name.isEmpty() ? tr("Параметр") : param_name);
    const QString plain = QTextDocumentFragment::fromHtml(ui->paramTitle->text()).toPlainText();
    setWindowTitle(tr("Настройка ползунка %1").arg(plain));
    ui->valueLabel->setText(tr("Текущее значение: %1").arg(num_format::format(values.value)));

    const double lo = std::min(hard_min, hard_max);
    const double hi = std::max(hard_min, hard_max);
    ui->minSpin->setRange(lo, hi);
    ui->maxSpin->setRange(lo, hi);
    ui->minSpin->setValue(values.min);
    ui->maxSpin->setValue(values.max);
    ui->intervalsSpin->setValue(std::max(1, values.intervals));

    connect(ui->minSpin, &QDoubleSpinBox::valueChanged, this, [this](double) { sync_step_hint(); });
    connect(ui->maxSpin, &QDoubleSpinBox::valueChanged, this, [this](double) { sync_step_hint(); });
    connect(ui->intervalsSpin, &QSpinBox::valueChanged, this, [this](int) { sync_step_hint(); });
    connect(ui->defaultButton, &QPushButton::clicked, this, &SliderSettingsDialog::restore_defaults);
    connect(ui->applyButton, &QPushButton::clicked, this, &SliderSettingsDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &SliderSettingsDialog::reject);
    sync_step_hint();
}

SliderSettingsDialog::~SliderSettingsDialog() {
    delete ui;
}

SliderSettings SliderSettingsDialog::data() const {
    SliderSettings s;
    s.min       = ui->minSpin->value();
    s.max       = ui->maxSpin->value();
    s.intervals = ui->intervalsSpin->value();
    if (s.max < s.min)
        std::swap(s.min, s.max);
    if (!(s.max > s.min))
        s.max = s.min + 1.0;
    if (s.intervals < 1)
        s.intervals = 1;
    return s;
}

void SliderSettingsDialog::restore_defaults() {
    ui->minSpin->setValue(defaults_.min);
    ui->maxSpin->setValue(defaults_.max);
    ui->intervalsSpin->setValue(std::max(1, defaults_.intervals));
    sync_step_hint();
}

void SliderSettingsDialog::sync_step_hint() {
    const double lo   = ui->minSpin->value();
    const double hi   = ui->maxSpin->value();
    const int n       = ui->intervalsSpin->value();
    const double span = hi - lo;
    if (!(span > 0.0) || n < 1) {
        ui->stepHint->setText(tr("до должно быть больше, чем от"));
        return;
    }
    ui->stepHint->setText(num_format::format(span / static_cast<double>(n)));
}
