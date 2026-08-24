#include "code/widgets/tf-form/tran-func-form.h"

#include "code/widgets/tf-form/tran-func-form-line-edit.hpp"

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QTimer>

TranFuncForm::TranFuncForm(std::size_t n, std::size_t m, const QString& title, QWidget* parent)
    : QWidget(parent), numerator_(n), denominator_(m) {
    setObjectName(QStringLiteral("TranFuncForm"));

    auto* title_label = new QLabel(title, this);
    title_label->setObjectName(QStringLiteral("tfTitle"));
    title_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    create_label_context_menu(title_label);

    auto* divider = new QFrame(this);
    divider->setObjectName(QStringLiteral("tfDivider"));
    divider->setFrameShape(QFrame::NoFrame);
    divider->setFixedHeight(2);
    divider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    divider->setMinimumWidth(120);
    divider->setAttribute(Qt::WA_StyledBackground, true);

    auto* frac_grid = new QGridLayout;
    frac_grid->setHorizontalSpacing(8);
    frac_grid->setVerticalSpacing(6);
    frac_grid->setContentsMargins(0, 0, 0, 0);
    frac_grid->addWidget(title_label, 0, 0, 3, 1, Qt::AlignRight | Qt::AlignVCenter);
    frac_grid->addLayout(create_coeff_row(numerator_, 1), 0, 1);
    frac_grid->addWidget(divider, 1, 1);
    frac_grid->addLayout(create_coeff_row(denominator_, 1 << static_cast<int>(n)), 2, 1);
    frac_grid->setColumnStretch(1, 1);
    frac_grid->setRowMinimumHeight(1, 2);

    auto* root = new QHBoxLayout(this);
    root->setSpacing(8);
    root->setAlignment(Qt::AlignVCenter);
    root->addLayout(frac_grid, 1);

    auto* mul_dot = new QLabel(QString::fromUtf8("·"), this);
    mul_dot->setObjectName(QStringLiteral("tfMulDot"));
    mul_dot->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    root->addWidget(mul_dot, 0, Qt::AlignVCenter);

    delay_group_ = new QWidget(this);
    delay_group_->setObjectName(QStringLiteral("tfDelayGroup"));
    delay_group_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto* delay_root = new QHBoxLayout(delay_group_);
    delay_root->setContentsMargins(0, 0, 0, 0);
    delay_root->setSpacing(2);

    auto* e_base = new QLabel(QStringLiteral("e"), delay_group_);
    e_base->setObjectName(QStringLiteral("tfDelayBase"));
    e_base->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    auto* exp_group = new QWidget(delay_group_);
    exp_group->setObjectName(QStringLiteral("tfDelayExp"));
    auto* exp_layout = new QHBoxLayout(exp_group);
    exp_layout->setContentsMargins(0, 0, 0, 8);
    exp_layout->setSpacing(2);

    auto* minus_lab = new QLabel(QString::fromUtf8("−"), exp_group);
    minus_lab->setObjectName(QStringLiteral("tfDelayMinus"));
    minus_lab->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    delay_element_ = new QDoubleSpinBox(exp_group);
    delay_element_->setObjectName(QStringLiteral("tfDelay"));
    delay_element_->setDecimals(2);
    delay_element_->setSingleStep(0.01);
    delay_element_->setRange(0.0, 30.0);
    delay_element_->setValue(0.0);
    delay_element_->setKeyboardTracking(false);
    delay_element_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    delay_element_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    delay_element_->setToolTip(tr("Постоянная запаздывания τ, с (0…30). Ввод с клавиатуры."));
    delay_element_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    {
        QStyleOptionSpinBox opt;
        opt.initFrom(delay_element_);
        opt.frame         = true;
        opt.buttonSymbols = QAbstractSpinBox::NoButtons;
        opt.stepEnabled   = QAbstractSpinBox::StepNone;
        const QFontMetrics fm(delay_element_->font());
        const int extra  = 2 * fm.horizontalAdvance(QLatin1Char('0'));
        const int text_w = fm.horizontalAdvance(QStringLiteral("30.00")) + extra;
        const QSize text_sz(text_w + 4, fm.height());
        const QSize sz = delay_element_->style()->sizeFromContents(QStyle::CT_SpinBox, &opt, text_sz, delay_element_);
        delay_element_->setFixedSize(sz.expandedTo(text_sz + QSize(14, 6)));
    }

    auto* p_lab = new QLabel(QStringLiteral("p"), exp_group);
    p_lab->setObjectName(QStringLiteral("tfDelayP"));
    p_lab->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    exp_layout->addWidget(minus_lab);
    exp_layout->addWidget(delay_element_);
    exp_layout->addWidget(p_lab);
    delay_root->addWidget(e_base, 0, Qt::AlignBottom);
    delay_root->addWidget(exp_group, 0, Qt::AlignTop);
    root->addWidget(delay_group_, 0, Qt::AlignVCenter);

    copy_btn_ = new QPushButton(QStringLiteral("📋"), this);
    copy_btn_->setObjectName(QStringLiteral("tfCopyButton"));
    copy_btn_->setToolTip(tr("Копировать ПФ (для отчёта или вставки на другую вкладку)"));
    copy_btn_->setFixedSize(24, 24);
    copy_btn_->setFocusPolicy(Qt::NoFocus);
    copy_btn_->raise();

    paste_btn_ = new QPushButton(QStringLiteral("📌"), this);
    paste_btn_->setObjectName(QStringLiteral("tfPasteButton"));
    paste_btn_->setToolTip(tr("Вставить ПФ из буфера обмена"));
    paste_btn_->setFixedSize(24, 24);
    paste_btn_->setFocusPolicy(Qt::NoFocus);
    paste_btn_->raise();

    connect(copy_btn_, &QPushButton::clicked, this, &TranFuncForm::copyToClipboard);
    connect(paste_btn_, &QPushButton::clicked, this, &TranFuncForm::pasteFromClipboard);
}

void TranFuncForm::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    reposition_clip_buttons();
}

void TranFuncForm::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, [this] {
        for (auto* le : numerator_)
            adjust_line_edit_width(le);
        for (auto* le : denominator_)
            adjust_line_edit_width(le);
        reposition_clip_buttons();
    });
}

void TranFuncForm::setTransferFunction(const numina::TransferFunction* tf) {
    tf_ = tf;
}

void TranFuncForm::setExactDelaySolutions(bool on) noexcept {
    exact_delay_solutions_ = on;
}

std::vector<double> TranFuncForm::numerator() const {
    return get_line_edit_data(numerator_);
}

std::vector<double> TranFuncForm::denominator() const {
    return get_line_edit_data(denominator_);
}

bool TranFuncForm::hasDelay() const {
    return delay_element_->value() != 0;
}

double TranFuncForm::delayTime() const {
    return delay_element_->value();
}
