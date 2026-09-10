#include "irbis/dialogs/tf-input-dialog.h"

#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/dialog-icons.hxx"
#include "irbis/util/secondary-text.hxx"
#include "irbis/util/tf-builder.hpp"
#include "irbis/widgets/tf-display-widget.h"
#include "ui_tf-input-dialog.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QToolButton>
#include <utility>

bool TfInputDialog::high_first_pref_ = false;

TfInputDialog::TfInputDialog(QWidget* parent) : QDialog(parent), ui(new Ui::TfInputDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::TransferFunction);
    setMinimumWidth(480);
    resize(560, 560);

    secondary_text::applyAll({ui->hintLabel, ui->delayHint});
    ui->errorLabel->clear();
    ui->errorLabel->setVisible(false);

    auto* order_group = new QButtonGroup(this);
    order_group->setExclusive(true);
    order_group->addButton(ui->orderHighButton);
    order_group->addButton(ui->orderLowButton);

    connect(ui->orderHighButton, &QAbstractButton::toggled, this, [this](bool on) {
        if (on)
            set_order(true);
    });
    connect(ui->orderLowButton, &QAbstractButton::toggled, this, [this](bool on) {
        if (on)
            set_order(false);
    });
    connect(ui->numEdit, &QLineEdit::textChanged, this, &TfInputDialog::onFieldsChanged);
    connect(ui->denEdit, &QLineEdit::textChanged, this, &TfInputDialog::onFieldsChanged);
    connect(ui->delaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double) { onFieldsChanged(); });
    connect(ui->applyButton, &QPushButton::clicked, this, &TfInputDialog::tryAccept);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    high_first_ = high_first_pref_;
    {
        const QSignalBlocker b1(ui->orderHighButton);
        const QSignalBlocker b2(ui->orderLowButton);
        ui->orderHighButton->setChecked(high_first_);
        ui->orderLowButton->setChecked(!high_first_);
    }
    ui->preview->setTitle(QStringLiteral("W(p) = "));
    refresh_preview();
}

TfInputDialog::~TfInputDialog() {
    delete ui;
}

void TfInputDialog::setSymbolHtml(const QString& html) {
    QString symbol = html.trimmed();
    if (symbol.isEmpty())
        symbol = QStringLiteral("W(p)");
    ui->symbolLabel->setText(symbol);
    ui->preview->setTitle(symbol + QStringLiteral(" = "));
}

void TfInputDialog::setValue(const Vec& num, const Vec& den, double tau) {
    num_ = num;
    den_ = den;
    tau_ = tau < 0.0 ? 0.0 : tau;
    fill_fields_from_value();
    refresh_preview();
}

bool TfInputDialog::edit(QWidget* parent, Vec& num, Vec& den, double& tau, const QString& symbolHtml) {
    TfInputDialog dialog(parent);
    dialog.setSymbolHtml(symbolHtml);
    dialog.setValue(num, den, tau);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    num = dialog.numerator();
    den = dialog.denominator();
    tau = dialog.delay();
    return true;
}

void TfInputDialog::tryAccept() {
    Vec num, den;
    double tau = 0.0;
    QString error;
    if (!collect_valid(num, den, tau, &error)) {
        show_error(error);
        return;
    }
    num_              = std::move(num);
    den_              = std::move(den);
    tau_              = tau;
    high_first_pref_  = high_first_;
    accept();
}
