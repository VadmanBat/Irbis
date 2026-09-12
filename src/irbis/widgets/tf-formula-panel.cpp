#include "irbis/widgets/tf-formula-panel.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QPaintEvent>
#include <QPushButton>
#include <QRegion>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {
class FormulaClip : public QWidget {
public:
    explicit FormulaClip(QWidget* parent = nullptr) : QWidget(parent) {
        setAutoFillBackground(false);
        setAttribute(Qt::WA_StyledBackground, false);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        setMask(QRegion(rect()));
    }

    void paintEvent(QPaintEvent*) override {}
};
}

TfFormulaPanel::TfFormulaPanel(QWidget* parent) : QFrame(parent) {
    setObjectName(QStringLiteral("tfModelCard"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);
    setAttribute(Qt::WA_Hover, true);
    setContextMenuPolicy(Qt::DefaultContextMenu);

    clip_ = new FormulaClip(this);
    display_ = new TfDisplayWidget(clip_);
    display_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    display_->setAutoFillBackground(false);
    display_->installEventFilter(this);
    clip_->installEventFilter(this);

    name_label_ = new QLabel(this);
    name_label_->setObjectName(QStringLiteral("tfLinkName"));
    name_label_->setWordWrap(false);
    name_label_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    name_label_->setMinimumHeight(36);
    name_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto make_btn = [](QWidget* host, const QString& name, const QString& text, const QString& tip) {
        auto* btn = new QPushButton(text, host);
        btn->setObjectName(name);
        btn->setToolTip(tip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        btn->setMinimumHeight(22);
        btn->setFocusPolicy(Qt::TabFocus);
        return btn;
    };

    actions_host_ = new QWidget(this);
    actions_host_->setObjectName(QStringLiteral("tfHoverActions"));
    actions_host_->setAttribute(Qt::WA_StyledBackground, true);
    actions_host_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    edit_btn_    = make_btn(actions_host_, QStringLiteral("editTfButton"), tr("Изменить"),
                            tr("Задать коэффициенты передаточной функции"));
    copy_btn_    = make_btn(actions_host_, QStringLiteral("copyTfButton"), tr("Копировать"),
                            tr("Скопировать передаточную функцию в буфер обмена"));
    paste_btn_   = make_btn(actions_host_, QStringLiteral("pasteTfButton"), tr("Вставить"),
                            tr("Вставить передаточную функцию из буфера обмена"));
    details_btn_ = make_btn(actions_host_, QStringLiteral("equationsButton"), tr("Подробнее"),
                            tr("Полюса, h(t), w(t) и дифференциальные уравнения"));

    auto* actions = new QHBoxLayout(actions_host_);
    actions->setContentsMargins(4, 2, 4, 2);
    actions->setSpacing(4);
    actions->addWidget(edit_btn_);
    actions->addWidget(copy_btn_);
    actions->addWidget(paste_btn_);
    actions->addWidget(details_btn_);

    hbar_ = new QScrollBar(Qt::Horizontal, this);
    hbar_->setObjectName(QStringLiteral("tfFormulaBar"));
    hbar_->setFocusPolicy(Qt::NoFocus);
    hbar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hbar_->hide();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 10, 12, 10);
    root->setSpacing(4);
    root->addWidget(clip_, 0);
    root->addWidget(name_label_, 0);

    hide_timer_ = new QTimer(this);
    hide_timer_->setSingleShot(true);
    hide_timer_->setInterval(160);
    connect(hide_timer_, &QTimer::timeout, this, [this] { set_actions_open(false); });

    connect(edit_btn_, &QPushButton::clicked, this, &TfFormulaPanel::editRequested);
    connect(paste_btn_, &QPushButton::clicked, this, &TfFormulaPanel::pasteRequested);
    connect(copy_btn_, &QPushButton::clicked, this, &TfFormulaPanel::copy_clicked);
    connect(details_btn_, &QPushButton::clicked, this, &TfFormulaPanel::details_clicked);
    connect(display_, &TfDisplayWidget::contentsChanged, this, [this] {
        update_link_name();
        sync_actions();
        hbar_->setValue(0);
        updateGeometry();
        sync_scroll();
        QTimer::singleShot(0, this, [this] { sync_scroll(); });
        emit contentsChanged();
    });
    connect(hbar_, &QScrollBar::valueChanged, this, [this](int v) { display_->move(-v, 0); });

    sync_actions();
    sync_scroll();
    actions_host_->hide();
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
    paste_visible_ = on;
    paste_btn_->setVisible(on);
    if (actions_host_->isVisible())
        place_actions();
    updateGeometry();
}

void TfFormulaPanel::setEditVisible(bool on) {
    edit_visible_ = on;
    edit_btn_->setVisible(on);
    if (actions_host_->isVisible())
        place_actions();
    updateGeometry();
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
