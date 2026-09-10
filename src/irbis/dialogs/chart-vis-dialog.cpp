#include "irbis/dialogs/chart-vis-dialog.h"

#include "irbis/util/dialog-icons.hxx"
#include "irbis/util/secondary-text.hxx"
#include "ui_chart-vis-dialog.h"

#include <QCheckBox>
#include <QLayout>
#include <QPushButton>

ChartVisDialog::ChartVisDialog(ChartVisibility vis, QWidget* parent)
    : QDialog(parent), ui(new Ui::ChartVisDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::ChartProps);
    secondary_text::apply(ui->hintLabel);
    setMinimumWidth(300);
    if (auto* lay = layout())
        lay->setSizeConstraint(QLayout::SetFixedSize);

    apply_to_checks(vis);

    auto wire = [this](QCheckBox* box) {
        connect(box, &QCheckBox::toggled, this, &ChartVisDialog::keep_at_least_one);
    };
    wire(ui->transientCheck);
    wire(ui->impulseCheck);
    wire(ui->nyquistCheck);
    wire(ui->phaseCheck);
    wire(ui->amplitudeCheck);

    connect(ui->applyButton, &QPushButton::clicked, this, &ChartVisDialog::accept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &ChartVisDialog::reject);
}

ChartVisDialog::~ChartVisDialog() {
    delete ui;
}

void ChartVisDialog::apply_to_checks(ChartVisibility vis) {
    if (vis.count() == 0)
        vis.transient = true;
    ui->transientCheck->setChecked(vis.transient);
    ui->impulseCheck->setChecked(vis.impulse);
    ui->nyquistCheck->setChecked(vis.nyquist);
    ui->phaseCheck->setChecked(vis.phase);
    ui->amplitudeCheck->setChecked(vis.amplitude);
}

ChartVisibility ChartVisDialog::data() const {
    ChartVisibility vis;
    vis.transient = ui->transientCheck->isChecked();
    vis.impulse   = ui->impulseCheck->isChecked();
    vis.nyquist   = ui->nyquistCheck->isChecked();
    vis.phase     = ui->phaseCheck->isChecked();
    vis.amplitude = ui->amplitudeCheck->isChecked();
    if (vis.count() == 0)
        vis.transient = true;
    return vis;
}

void ChartVisDialog::keep_at_least_one() {
    if (data().count() > 0)
        return;
    auto* box = qobject_cast<QCheckBox*>(sender());
    if (!box)
        return;
    box->blockSignals(true);
    box->setChecked(true);
    box->blockSignals(false);
}

