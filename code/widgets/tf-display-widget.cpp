#include "code/widgets/tf-display-widget.h"

#include "code/util/format.hxx"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStringList>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>

TfDisplayWidget::TfDisplayWidget(const QString& title, QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("TfDisplayWidget"));

    titleLabel_ = new QLabel(title, this);
    titleLabel_->setObjectName(QStringLiteral("tfTitle"));
    titleLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    numLabel_ = new QLabel(QStringLiteral("—"), this);
    numLabel_->setObjectName(QStringLiteral("tfPolyText"));
    numLabel_->setAlignment(Qt::AlignCenter);
    numLabel_->setTextFormat(Qt::RichText);
    numLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    denLabel_ = new QLabel(QStringLiteral("—"), this);
    denLabel_->setObjectName(QStringLiteral("tfPolyText"));
    denLabel_->setAlignment(Qt::AlignCenter);
    denLabel_->setTextFormat(Qt::RichText);
    denLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("tfDivider"));
    divider->setFrameShape(QFrame::NoFrame);
    divider->setFixedHeight(2);
    divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    divider->setMinimumWidth(120);
    divider->setAttribute(Qt::WA_StyledBackground, true);

    delayGroup_ = new QWidget(this);
    delayGroup_->setObjectName(QStringLiteral("tfDelayGroup"));
    auto* delay_root = new QHBoxLayout(delayGroup_);
    delay_root->setContentsMargins(0, 0, 0, 0);
    delay_root->setSpacing(2);

    delayLabel_ = new QLabel(delayGroup_);
    delayLabel_->setObjectName(QStringLiteral("tfDelayText"));
    delayLabel_->setTextFormat(Qt::RichText);
    delayLabel_->setVisible(false);
    delay_root->addWidget(delayLabel_);

    copyBtn_ = new QPushButton(QStringLiteral("📋"), this);
    copyBtn_->setObjectName(QStringLiteral("tfCopyButton"));
    copyBtn_->setToolTip(tr("Копировать ПФ"));
    copyBtn_->setFixedSize(28, 28);
    copyBtn_->setFocusPolicy(Qt::NoFocus);
    copyBtn_->setEnabled(false);
    copyBtn_->raise();
    connect(copyBtn_, &QPushButton::clicked, this, &TfDisplayWidget::copyToClipboard);

    auto* frac = new QGridLayout;
    frac->setContentsMargins(0, 0, 0, 0);
    frac->setHorizontalSpacing(8);
    frac->setVerticalSpacing(4);
    frac->addWidget(titleLabel_, 0, 0, 3, 1, Qt::AlignRight | Qt::AlignVCenter);
    frac->addWidget(numLabel_, 0, 1);
    frac->addWidget(divider, 1, 1);
    frac->addWidget(denLabel_, 2, 1);
    frac->addWidget(delayGroup_, 0, 2, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);

    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addLayout(frac, 0);
    row->addStretch(1);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 12); // room for overlay copy
    root->addLayout(row);
}

void TfDisplayWidget::set_polys(const Vec& num, const Vec& den, double tau) {
    num_   = num;
    den_   = den;
    tau_   = tau;
    empty_ = num.empty() && den.empty();

    numLabel_->setText(empty_ ? QStringLiteral("—")
                              : num_format::polyHtmlLowFirst(num_, num_format::SIGNIFICANT_DIGITS));
    denLabel_->setText(empty_ ? QStringLiteral("—")
                              : num_format::polyHtmlLowFirst(den_, num_format::SIGNIFICANT_DIGITS));

    if (!empty_ && tau_ > 0.0) {
        delayLabel_->setText(
            QStringLiteral("· e<sup>−%1 p</sup>").arg(num_format::format(tau_, num_format::SIGNIFICANT_DIGITS)));
        delayLabel_->setVisible(true);
        delayGroup_->setVisible(true);
    }
    else {
        delayLabel_->clear();
        delayLabel_->setVisible(false);
        delayGroup_->setVisible(false);
    }
    copyBtn_->setEnabled(!empty_);
    QTimer::singleShot(0, this, &TfDisplayWidget::reposition_copy_button);
}

void TfDisplayWidget::setTransferFunction(const numina::TransferFunction& tf, double tau) {
    set_polys(tf.numerator().coeffs(), tf.denominator().coeffs(), tau);
}

void TfDisplayWidget::clear() {
    set_polys({}, {}, 0.0);
}

QString TfDisplayWidget::human_text() const {
    if (empty_)
        return {};
    QString human = QStringLiteral("W(p) = (%1) / (%2)")
                        .arg(num_format::polyPlainLowFirst(num_, num_format::SIGNIFICANT_DIGITS),
                             num_format::polyPlainLowFirst(den_, num_format::SIGNIFICANT_DIGITS));
    if (tau_ > 0.0)
        human += QStringLiteral(" · e^(-%1 p)").arg(num_format::format(tau_, num_format::SIGNIFICANT_DIGITS));
    return human;
}

QString TfDisplayWidget::export_text() const {
    if (empty_)
        return {};
    QStringList num_parts, den_parts;
    for (double v : num_)
        num_parts << num_format::formatFull(v);
    for (double v : den_)
        den_parts << num_format::formatFull(v);
    return QStringLiteral(
               "RegValve-TF-v1\n"
               "num: %1\n"
               "den: %2\n"
               "tau: %3\n"
               "\n"
               "%4\n")
        .arg(num_parts.join(QLatin1Char(' ')), den_parts.join(QLatin1Char(' ')), num_format::formatFull(tau_),
             human_text());
}

void TfDisplayWidget::copyToClipboard() {
    if (empty_)
        return;
    QApplication::clipboard()->setText(export_text());
    QToolTip::showText(copyBtn_->mapToGlobal(QPoint(0, copyBtn_->height())), tr("ПФ скопирована"), copyBtn_, QRect(),
                       1500);
}

void TfDisplayWidget::reposition_copy_button() {
    if (!copyBtn_)
        return;
    constexpr int btn = 28;

    int x = 0;
    int y = 0;

    if (tau_ > 0.0 && delayGroup_->isVisible() && delayLabel_->isVisible()) {
        // With e^{−τp}: under the delay block (same as before).
        const QRect g = delayGroup_->geometry();
        x             = g.right() - btn;
        y             = g.bottom() + 2;
    }
    else {
        // No delay: well to the right of N/D so the button never sits on the fraction.
        constexpr int gap = 32;
        const QRect den   = denLabel_->geometry();
        x                 = den.right() + gap;
        y                 = den.bottom() - btn;
        if (y < den.top())
            y = den.bottom() + 2;
        // Ensure the widget is wide enough; do not clamp back onto the poly text.
        const int need_w = x + btn + 8;
        if (need_w > width())
            setMinimumWidth(need_w);
    }

    if (y + btn > height())
        y = qMax(0, height() - btn);
    copyBtn_->move(qMax(0, x), y);
    copyBtn_->raise();
}

void TfDisplayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    reposition_copy_button();
}

void TfDisplayWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &TfDisplayWidget::reposition_copy_button);
}
