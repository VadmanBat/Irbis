#include "irbis/widgets/tf-h-scroll.h"

#include "irbis/widgets/tf-display-widget.h"

#include <QBoxLayout>
#include <QEvent>
#include <QPaintEvent>
#include <QRegion>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace {
constexpr int BAR_H = 12;

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

TfHScroll::TfHScroll(QWidget* content, QWidget* parent) : QWidget(parent), content_(content) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    setMinimumWidth(0);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::NoFocus);

    clip_ = new FormulaClip(this);
    content_->setParent(clip_);
    content_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    content_->setAutoFillBackground(false);
    content_->setMinimumWidth(0);
    content_->installEventFilter(this);
    clip_->installEventFilter(this);

    hbar_ = new QScrollBar(Qt::Horizontal, this);
    hbar_->setObjectName(QStringLiteral("tfFormulaBar"));
    hbar_->setFocusPolicy(Qt::NoFocus);
    hbar_->hide();

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, BAR_H);
    root->setSpacing(0);
    root->addWidget(clip_);

    connect(hbar_, &QScrollBar::valueChanged, this, [this](int v) { content_->move(-v, 0); });
    if (auto* tf = qobject_cast<TfDisplayWidget*>(content_)) {
        connect(tf, &TfDisplayWidget::contentsChanged, this, [this] {
            hbar_->setValue(0);
            updateGeometry();
            sync_scroll();
            QTimer::singleShot(0, this, [this] { sync_scroll(); });
        });
    }
    sync_scroll();
}

TfHScroll* TfHScroll::wrapInLayout(QWidget* content, QBoxLayout* layout) {
    layout->removeWidget(content);
    auto* scroll = new TfHScroll(content, layout->parentWidget());
    layout->addWidget(scroll);
    return scroll;
}

QSize TfHScroll::content_hint() const {
    return content_->sizeHint().expandedTo(content_->minimumSizeHint());
}

void TfHScroll::place_hbar() {
    hbar_->setGeometry(0, height() - BAR_H, width(), BAR_H);
    hbar_->raise();
}

void TfHScroll::sync_scroll() {
    const QSize sh = content_hint();
    content_->resize(sh);
    const int h = qMax(1, sh.height());
    if (clip_->minimumHeight() != h || clip_->maximumHeight() != h)
        clip_->setFixedHeight(h);

    const int need     = qMax(sh.width(), content_->width());
    const int avail    = clip_->width();
    const int overflow = avail > 0 ? qMax(0, need - avail) : 0;
    const bool show    = overflow > 2;
    hbar_->setRange(0, show ? overflow : 0);
    hbar_->setPageStep(qMax(1, avail));
    hbar_->setSingleStep(16);
    if (hbar_->isVisible() != show)
        hbar_->setVisible(show);
    if (!show)
        hbar_->setValue(0);
    content_->move(-hbar_->value(), 0);
    if (show)
        place_hbar();
}

QSize TfHScroll::sizeHint() const {
    QSize s = content_hint();
    s.rheight() += BAR_H;
    return s;
}

QSize TfHScroll::minimumSizeHint() const {
    QSize s = sizeHint();
    s.setWidth(0);
    return s;
}

void TfHScroll::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    sync_scroll();
}

bool TfHScroll::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::Wheel)
        return QWidget::eventFilter(watched, event);
    if (watched != clip_ && watched != content_)
        return QWidget::eventFilter(watched, event);
    if (hbar_->maximum() <= hbar_->minimum())
        return QWidget::eventFilter(watched, event);

    const auto* wheel = static_cast<const QWheelEvent*>(event);
    const QPoint pixel = wheel->pixelDelta();
    const QPoint angle = wheel->angleDelta();
    const QPoint delta = pixel.isNull() ? angle / 8 : pixel;
    const int step     = delta.x() != 0 ? delta.x() : delta.y();
    if (step == 0)
        return QWidget::eventFilter(watched, event);

    hbar_->setValue(hbar_->value() - step);
    return true;
}

