#include "irbis/dialogs/tf-input-dialog.h"

#include "irbis/tabs/tab-shell.hpp"
#include "irbis/util/dialog-icons.hxx"
#include "irbis/util/secondary-text.hxx"
#include "irbis/util/tf-builder.hpp"
#include "irbis/widgets/tf-display-widget.h"
#include "irbis/widgets/tf-h-scroll.h"
#include "ui_tf-input-dialog.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QToolButton>
#include <utility>

bool TfInputDialog::high_first_pref_ = false;

TfInputDialog::TfInputDialog(QWidget* parent) : QDialog(parent), ui(new Ui::TfInputDialog) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::TransferFunction);
    setMinimumSize(480, 540);
    resize(560, 620);

    secondary_text::applyAll({ui->hintLabel, ui->delayHint});
    ui->errorBanner->setAttribute(Qt::WA_StyledBackground, true);
    ui->errorLabel->setTextFormat(Qt::PlainText);
    ui->errorLabel->clear();
    style_error_state(false, false, false);

    auto* coeff_validator =
        new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[0-9 .,+eE-]*$")), this);
    ui->numEdit->setValidator(coeff_validator);
    ui->denEdit->setValidator(coeff_validator);

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
    if (auto* box = qobject_cast<QBoxLayout*>(ui->previewGroup->layout()))
        TfHScroll::wrapInLayout(ui->preview, box);
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
    bool num_bad = false;
    bool den_bad = false;
    if (!collect_valid(num, den, tau, &error, &num_bad, &den_bad)) {
        show_error(error, num_bad, den_bad);
        return;
    }
    num_             = std::move(num);
    den_             = std::move(den);
    tau_             = tau;
    high_first_pref_ = high_first_;
    accept();
}
