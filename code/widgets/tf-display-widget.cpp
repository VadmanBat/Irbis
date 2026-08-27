#include "code/widgets/tf-display-widget.h"

#include "code/util/format.hxx"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
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
    root->setContentsMargins(0, 0, 0, 0);
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
    emit contentsChanged();
}

void TfDisplayWidget::setTransferFunction(const numina::TransferFunction& tf, double tau) {
    tf_ = tf;
    set_polys(tf.numerator().coeffs(), tf.denominator().coeffs(), tau);
}

void TfDisplayWidget::setStructureTemplate(int numDegree, int denDegree) {
    if (denDegree < 1)
        denDegree = 1;
    if (numDegree < 0)
        numDegree = 0;
    if (numDegree > denDegree)
        numDegree = denDegree;

    auto term = [](QChar letter, int power) -> QString {
        const QString c = QStringLiteral("%1<sub>%2</sub>").arg(letter).arg(power);
        if (power == 0)
            return c;
        if (power == 1)
            return c + QStringLiteral(" p");
        return c + QStringLiteral(" p<sup>%1</sup>").arg(power);
    };

    QString num = term(QLatin1Char('b'), 0);
    for (int k = 1; k <= numDegree; ++k)
        num += QStringLiteral(" + ") + term(QLatin1Char('b'), k);

    QString den = QStringLiteral("1");
    for (int k = 1; k <= denDegree; ++k)
        den += QStringLiteral(" + ") + term(QLatin1Char('a'), k);

    tf_    = {};
    empty_ = true;
    num_.clear();
    den_.clear();
    tau_ = 0.0;
    numLabel_->setText(num);
    denLabel_->setText(den);
    delayLabel_->clear();
    delayLabel_->setVisible(false);
    delayGroup_->setVisible(false);
    emit contentsChanged();
}

void TfDisplayWidget::clear() {
    tf_ = {};
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
               "Irbis-TF-v1\n"
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
}
