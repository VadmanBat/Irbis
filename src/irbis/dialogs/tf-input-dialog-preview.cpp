#include "irbis/dialogs/tf-input-dialog.h"
#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/style-core.hpp"
#include "irbis/util/tf-builder.hpp"
#include "irbis/util/tf-clipboard.hpp"
#include "irbis/widgets/tf-display-widget.h"
#include "ui_tf-input-dialog.h"

#include <QFrame>
#include <QLineEdit>
#include <QPalette>
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

void TfInputDialog::style_error_state(bool has_error, bool num_bad, bool den_bad) {
    const bool dark       = palette().color(QPalette::Window).lightness() < 128;
    const QString dark_ui = dark ? QStringLiteral("true") : QStringLiteral("false");
    const QString on      = has_error ? QStringLiteral("true") : QStringLiteral("false");
    const QString num_on  = num_bad ? QStringLiteral("true") : QStringLiteral("false");
    const QString den_on  = den_bad ? QStringLiteral("true") : QStringLiteral("false");
    style_util::setProperty(ui->errorBanner, "darkUi", dark_ui);
    style_util::setProperty(ui->errorBanner, "hasError", on);
    style_util::setProperty(ui->errorLabel, "darkUi", dark_ui);
    style_util::setProperty(ui->errorLabel, "hasError", on);
    style_util::setProperty(ui->numEdit, "darkUi", dark_ui);
    style_util::setProperty(ui->numEdit, "hasError", num_on);
    style_util::setProperty(ui->denEdit, "darkUi", dark_ui);
    style_util::setProperty(ui->denEdit, "hasError", den_on);
}

void TfInputDialog::show_error(const QString& message, bool num_bad, bool den_bad) {
    ui->errorLabel->setText(message);
    const bool on = !message.isEmpty();
    style_error_state(on, on && num_bad, on && den_bad);
}

void TfInputDialog::clear_error() {
    ui->errorLabel->clear();
    style_error_state(false, false, false);
}

bool TfInputDialog::collect_valid(Vec& num, Vec& den, double& tau, QString* error, bool* num_bad, bool* den_bad) const {
    if (num_bad)
        *num_bad = false;
    if (den_bad)
        *den_bad = false;

    if (!parse_poly(ui->numEdit->text(), num, error)) {
        if (num_bad)
            *num_bad = true;
        return false;
    }
    if (!parse_poly(ui->denEdit->text(), den, error)) {
        if (den_bad)
            *den_bad = true;
        return false;
    }
    tau = ui->delaySpin->value();
    if (tau < 0.0)
        tau = 0.0;
    const QString plant_err = tab_ui::plantInputError(num, den);
    if (plant_err.isEmpty())
        return true;
    if (error)
        *error = plant_err;

    bool num_ok = false;
    for (double c : num) {
        if (c != 0.0) {
            num_ok = true;
            break;
        }
    }
    if (!num_ok) {
        if (num_bad)
            *num_bad = true;
    }
    else if (den.size() < 2) {
        if (den_bad)
            *den_bad = true;
    }
    else {
        if (num_bad)
            *num_bad = true;
        if (den_bad)
            *den_bad = true;
    }
    return false;
}

void TfInputDialog::refresh_preview() {
    ui->delayHint->setVisible(!(ui->delaySpin->value() > 0.0));

    Vec num, den;
    double tau = 0.0;
    QString error;
    bool num_bad = false;
    bool den_bad = false;
    if (!collect_valid(num, den, tau, &error, &num_bad, &den_bad)) {
        ui->preview->clear();
        ui->applyButton->setEnabled(false);
        if (!ui->numEdit->text().trimmed().isEmpty() || !ui->denEdit->text().trimmed().isEmpty())
            show_error(error, num_bad, den_bad);
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
