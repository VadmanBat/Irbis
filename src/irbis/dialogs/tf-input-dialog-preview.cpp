#include "irbis/dialogs/tf-input-dialog.h"

#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/tf-builder.hpp"
#include "irbis/util/tf-clipboard.hpp"
#include "irbis/widgets/tf-display-widget.h"
#include "ui_tf-input-dialog.h"

#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>

bool TfInputDialog::parse_poly(const QString& text, Vec& high_to_low, QString* error) const {
    return tf_clipboard::parsePolyLine(text, high_first_, high_to_low, error);
}

void TfInputDialog::set_order(bool high_first) {
    if (high_first_ == high_first)
        return;
    high_first_ = high_first;
    refresh_preview();
}

void TfInputDialog::fill_fields_from_value() {
    const QSignalBlocker b1(ui->numEdit);
    const QSignalBlocker b2(ui->denEdit);
    const QSignalBlocker b3(ui->delaySpin);
    ui->numEdit->setText(tf_clipboard::formatCoeffLine(num_, high_first_));
    ui->denEdit->setText(tf_clipboard::formatCoeffLine(den_, high_first_));
    ui->delaySpin->setValue(tau_);
    ui->delayHint->setVisible(!(tau_ > 0.0));
}

void TfInputDialog::show_error(const QString& message) {
    ui->errorLabel->setText(message);
    ui->errorLabel->setVisible(!message.isEmpty());
}

void TfInputDialog::clear_error() {
    ui->errorLabel->clear();
    ui->errorLabel->setVisible(false);
}

bool TfInputDialog::collect_valid(Vec& num, Vec& den, double& tau, QString* error) const {
    if (!parse_poly(ui->numEdit->text(), num, error))
        return false;
    if (!parse_poly(ui->denEdit->text(), den, error))
        return false;
    tau = ui->delaySpin->value();
    if (tau < 0.0)
        tau = 0.0;
    const QString plant_err = tab_ui::plantInputError(num, den);
    if (!plant_err.isEmpty()) {
        if (error)
            *error = plant_err;
        return false;
    }
    return true;
}

void TfInputDialog::refresh_preview() {
    ui->delayHint->setVisible(!(ui->delaySpin->value() > 0.0));

    Vec num, den;
    double tau = 0.0;
    QString error;
    if (!collect_valid(num, den, tau, &error)) {
        ui->preview->clear();
        ui->applyButton->setEnabled(false);
        if (!ui->numEdit->text().trimmed().isEmpty() || !ui->denEdit->text().trimmed().isEmpty())
            show_error(error);
        else
            clear_error();
        return;
    }

    clear_error();
    ui->applyButton->setEnabled(true);
    ui->preview->setTransferFunction(tf_builder::plant(num, den), tau);
}

void TfInputDialog::onFieldsChanged() {
    refresh_preview();
}
