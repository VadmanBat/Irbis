#include "irbis/widgets/tf-formula-panel.h"

#include "irbis/dialogs/tran-func-dialog.h"
#include "irbis/util/secondary-text.hxx"
#include "irbis/util/tf-link-name.hpp"

#include <QCoreApplication>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPoint>
#include <QPushButton>
#include <QRect>
#include <QStyle>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

TfFormulaPanel::TfFormulaPanel(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("tfModelCard"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);

    display_ = new TfDisplayWidget(this);

    name_label_ = new QLabel(this);
    name_label_->setObjectName(QStringLiteral("tfLinkName"));
    name_label_->setWordWrap(false);
    name_label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    name_label_->setMinimumHeight(QFontMetrics(name_label_->font()).height() + 6);
    name_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto make_btn = [this](const QString& name, const QString& text, const QString& tip) {
        auto* btn = new QPushButton(text, this);
        btn->setObjectName(name);
        btn->setToolTip(tip);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        btn->setMinimumHeight(0);
        return btn;
    };

    edit_btn_    = make_btn(QStringLiteral("editTfButton"), tr("Изменить"),
                            tr("Задать коэффициенты передаточной функции"));
    copy_btn_    = make_btn(QStringLiteral("copyTfButton"), tr("Копировать"),
                            tr("Скопировать передаточную функцию в буфер обмена"));
    paste_btn_   = make_btn(QStringLiteral("pasteTfButton"), tr("Вставить"),
                            tr("Вставить передаточную функцию из буфера обмена"));
    details_btn_ = make_btn(QStringLiteral("equationsButton"), tr("Подробнее"),
                            tr("Полюса, h(t), w(t) и дифференциальные уравнения"));

    actions_host_ = new QWidget(this);
    actions_host_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* actions = new QVBoxLayout(actions_host_);
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(2);
    actions->addWidget(edit_btn_, 1);
    actions->addWidget(copy_btn_, 1);
    actions->addWidget(paste_btn_, 1);
    actions->addWidget(details_btn_, 1);

    auto* left = new QVBoxLayout;
    left->setContentsMargins(0, 0, 0, 0);
    left->setSpacing(4);
    left->addWidget(display_, 0, Qt::AlignLeft | Qt::AlignTop);
    left->addWidget(name_label_, 0);

    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(12);
    body->addLayout(left, 1);
    body->addWidget(actions_host_, 0);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(6);
    root->addLayout(body);

    connect(edit_btn_, &QPushButton::clicked, this, &TfFormulaPanel::editRequested);
    connect(paste_btn_, &QPushButton::clicked, this, &TfFormulaPanel::pasteRequested);
    connect(copy_btn_, &QPushButton::clicked, this, &TfFormulaPanel::copy_clicked);
    connect(details_btn_, &QPushButton::clicked, this, &TfFormulaPanel::details_clicked);
    connect(display_, &TfDisplayWidget::contentsChanged, this, [this] {
        update_link_name();
        sync_actions();
        emit contentsChanged();
    });
    sync_actions();
    sync_action_widths();
}

void TfFormulaPanel::sync_actions() {
    const bool on = !display_->isEmpty();
    copy_btn_->setEnabled(on);
    details_btn_->setEnabled(on);
}

void TfFormulaPanel::sync_action_widths() {
    const bool extra = edit_btn_->isVisibleTo(this) || paste_btn_->isVisibleTo(this);
    const int w      = extra ? 120 : 180;
    edit_btn_->setMinimumWidth(w);
    copy_btn_->setMinimumWidth(w);
    paste_btn_->setMinimumWidth(w);
    details_btn_->setMinimumWidth(w);
}

void TfFormulaPanel::update_link_name() {
    if (display_->isEmpty()) {
        link_name_.clear();
        name_label_->clear();
        return;
    }
    link_name_ = tf_link_name::describe(display_->numerator(), display_->denominator(), display_->delay());
    const QString unknown = QCoreApplication::translate("tf_link_name", "Неизвестно");
    if (link_name_.startsWith(unknown))
        name_label_->clear();
    else
        name_label_->setText(link_name_);
    secondary_text::apply(name_label_);
}

void TfFormulaPanel::copy_clicked() {
    if (display_->isEmpty())
        return;
    display_->copyToClipboard();
    QToolTip::showText(copy_btn_->mapToGlobal(QPoint(0, copy_btn_->height())), tr("ПФ скопирована"), copy_btn_,
                       QRect(), 1500);
}

void TfFormulaPanel::details_clicked() {
    if (display_->isEmpty())
        return;
    const double tau = exact_delay_ ? display_->delay() : 0.0;
    TranFuncDialog dialog(display_->transferFunction(), this, tau);
    dialog.exec();
}

void TfFormulaPanel::setCardFrame(bool on) {
    card_frame_ = on;
    setObjectName(on ? QStringLiteral("tfModelCard") : QStringLiteral("tfFormulaPanel"));
    setFrameShape(on ? QFrame::StyledPanel : QFrame::NoFrame);
    if (layout())
        layout()->setContentsMargins(on ? 12 : 0, on ? 10 : 0, on ? 12 : 0, on ? 10 : 0);
    style()->unpolish(this);
    style()->polish(this);
}

void TfFormulaPanel::setPasteVisible(bool on) {
    paste_btn_->setVisible(on);
    sync_action_widths();
}

void TfFormulaPanel::setEditVisible(bool on) {
    edit_btn_->setVisible(on);
    sync_action_widths();
}

void TfFormulaPanel::setTitle(const QString& html) {
    display_->setTitle(html);
}

void TfFormulaPanel::setTransferFunction(const numina::TransferFunction& tf, double tau) {
    display_->setTransferFunction(tf, tau);
}

void TfFormulaPanel::clear() {
    display_->clear();
}

bool TfFormulaPanel::isEmpty() const noexcept {
    return display_->isEmpty();
}
