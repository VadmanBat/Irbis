#include "code/dialogs/mod-par-dialog.h"

#include "code/util/dialog-icons.hxx"
#include "code/util/secondary-text.hxx"
#include "ui_mod-par-dialog.h"

#include <QCheckBox>
#include <utility>

ModParDialog::ModParDialog(const ModelParam& values, QWidget* parent, bool allowIdealDelay)
    : QDialog(parent), ui(new Ui::ModParDialog), allow_ideal_delay_(allowIdealDelay) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::ModelParams);
    secondary_text::applyAll({ui->hintLabel, ui->approxHint});
    // Compact default size; user may freely resize.
    setMinimumWidth(320);
    resize(400, height());
    setSizeGripEnabled(true);

    ui->autoTimeRangeCheckBox->setChecked(values.autoTimeRange);
    ui->autoTimeIntervalsCheckBox->setChecked(values.autoTimeIntervals);
    ui->autoFreqRangeCheckBox->setChecked(values.autoFreqRange);
    ui->autoFreqIntervalsCheckBox->setChecked(values.autoFreqIntervals);
    ui->timeMinSpinBox->setValue(values.timeMin);
    ui->timeMaxSpinBox->setValue(values.timeMax);
    ui->timeIntervalsSpinBox->setValue(values.timeIntervals);
    ui->freqMinSpinBox->setValue(values.freqMin);
    ui->freqMaxSpinBox->setValue(values.freqMax);
    ui->freqIntervalsSpinBox->setValue(values.freqIntervals);
    ui->approxOrderSpinBox->setValue(values.approxOrder);

    ui->usePadeCheckBox->setVisible(allow_ideal_delay_);
    if (allow_ideal_delay_) {
        ui->usePadeCheckBox->setChecked(values.usePadeApprox);
        connect(ui->usePadeCheckBox, &QCheckBox::toggled, this, &ModParDialog::on_use_pade_toggled);
        on_use_pade_toggled(values.usePadeApprox);
    }

    connect(ui->autoTimeRangeCheckBox, &QCheckBox::toggled, this, &ModParDialog::on_auto_time_range_toggled);
    connect(ui->autoTimeIntervalsCheckBox, &QCheckBox::toggled, this, &ModParDialog::on_auto_time_intervals_toggled);
    connect(ui->autoFreqRangeCheckBox, &QCheckBox::toggled, this, &ModParDialog::on_auto_freq_range_toggled);
    connect(ui->autoFreqIntervalsCheckBox, &QCheckBox::toggled, this, &ModParDialog::on_auto_freq_intervals_toggled);
    connect(ui->applyButton, &QPushButton::clicked, this, &ModParDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ModParDialog::reject);

    on_auto_time_range_toggled(values.autoTimeRange);
    on_auto_time_intervals_toggled(values.autoTimeIntervals);
    on_auto_freq_range_toggled(values.autoFreqRange);
    on_auto_freq_intervals_toggled(values.autoFreqIntervals);
}

ModParDialog::~ModParDialog() {
    delete ui;
}

ModelParam ModParDialog::data() const {
    ModelParam p;
    p.autoTimeRange     = ui->autoTimeRangeCheckBox->isChecked();
    p.autoTimeIntervals = ui->autoTimeIntervalsCheckBox->isChecked();
    p.autoFreqRange     = ui->autoFreqRangeCheckBox->isChecked();
    p.autoFreqIntervals = ui->autoFreqIntervalsCheckBox->isChecked();
    p.timeMin           = ui->timeMinSpinBox->value();
    p.timeMax           = ui->timeMaxSpinBox->value();
    p.timeIntervals     = ui->timeIntervalsSpinBox->value();
    p.freqMin           = ui->freqMinSpinBox->value();
    p.freqMax           = ui->freqMaxSpinBox->value();
    p.freqIntervals     = ui->freqIntervalsSpinBox->value();
    p.approxOrder       = ui->approxOrderSpinBox->value();
    p.usePadeApprox     = !allow_ideal_delay_ || ui->usePadeCheckBox->isChecked();
    if (p.timeMax < p.timeMin)
        std::swap(p.timeMin, p.timeMax);
    if (p.freqMax < p.freqMin)
        std::swap(p.freqMin, p.freqMax);
    return p;
}

void ModParDialog::on_auto_time_range_toggled(bool checked) {
    ui->timeMinSpinBox->setEnabled(!checked);
    ui->timeMaxSpinBox->setEnabled(!checked);
    ui->autoTimeIntervalsCheckBox->setEnabled(!checked);
    if (checked)
        ui->autoTimeIntervalsCheckBox->setChecked(true);
}

void ModParDialog::on_auto_time_intervals_toggled(bool checked) {
    ui->timeIntervalsSpinBox->setEnabled(!checked);
}

void ModParDialog::on_auto_freq_range_toggled(bool checked) {
    ui->freqMinSpinBox->setEnabled(!checked);
    ui->freqMaxSpinBox->setEnabled(!checked);
    // Same as time: intervals «Авто» only when range is manual.
    ui->autoFreqIntervalsCheckBox->setEnabled(!checked);
    if (checked)
        ui->autoFreqIntervalsCheckBox->setChecked(true);
}

void ModParDialog::on_auto_freq_intervals_toggled(bool checked) {
    ui->freqIntervalsSpinBox->setEnabled(!checked);
}

void ModParDialog::on_use_pade_toggled(bool checked) {
    ui->approxOrderLabel->setEnabled(checked);
    ui->approxOrderSpinBox->setEnabled(checked);
    ui->approxHint->setText(checked ? tr("1…6, обычно 6") : tr("Точное e^{−τp}: сдвиг h(t), фаза −ωτ"));
    secondary_text::apply(ui->approxHint);
}
