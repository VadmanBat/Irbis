#include "irbis/widgets/tf-formula-panel.h"

#include "irbis/dialogs/tran-func-dialog.h"
#include "irbis/util/secondary-text.hxx"
#include "irbis/util/tf-link-name.hpp"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QLayout>
#include <QMenu>
#include <QPoint>
#include <QPushButton>
#include <QRect>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QToolTip>
#include <QWheelEvent>
#include <QWidget>

int TfFormulaPanel::border_x() const {
    if (width() > 0) {
        const int b = width() - contentsRect().width();
        if (b >= 0)
            return b;
    }
    return 2 * frameWidth();
}

int TfFormulaPanel::chrome_width() const {
    int w = border_x();
    if (QLayout* lay = layout()) {
        const QMargins m = lay->contentsMargins();
        w += m.left() + m.right();
    }
    return w;
}

void TfFormulaPanel::sync_scroll() {
    const QSize sh = display_->sizeHint().expandedTo(display_->minimumSizeHint());
    display_->resize(sh);
    const int h = qMax(1, sh.height());
    if (clip_->minimumHeight() != h || clip_->maximumHeight() != h)
        clip_->setFixedHeight(h);

    const int need     = qMax(sh.width(), display_->width());
    const int avail    = clip_->width();
    const int overflow = avail > 0 ? qMax(0, need - avail) : 0;
    const int slack    = qMax(2, border_x());
    const bool show    = overflow > slack;
    hbar_->setRange(0, show ? overflow : 0);
    hbar_->setPageStep(qMax(1, avail));
    hbar_->setSingleStep(16);
    if (hbar_->isVisible() != show)
        hbar_->setVisible(show);
    if (!show)
        hbar_->setValue(0);
    display_->move(-hbar_->value(), 0);
    if (show)
        place_hbar();
}

void TfFormulaPanel::place_hbar() {
    const QRect inner = contentsRect();
    QMargins m;
    if (layout())
        m = layout()->contentsMargins();
    const int bh = qMax(hbar_->sizeHint().height(), 12);
    const int x  = inner.left() + m.left();
    const int w  = qMax(0, inner.width() - m.left() - m.right());
    const int y  = inner.bottom() - bh + 1;
    hbar_->setGeometry(x, y, w, bh);
    hbar_->raise();
    if (actions_host_->isVisible())
        actions_host_->raise();
}

QSize TfFormulaPanel::sizeHint() const {
    const int extra = chrome_width();
    const int min_w = actions_host_->sizeHint().width() + extra;
    int w           = display_->sizeHint().expandedTo(display_->minimumSizeHint()).width() + extra;
    if (w < min_w)
        w = min_w;

    int h = display_->sizeHint().height() + name_label_->minimumHeight();
    if (QLayout* lay = layout()) {
        const QMargins m = lay->contentsMargins();
        h += m.top() + m.bottom() + lay->spacing() + 2 * frameWidth();
    }
    return QSize(w, h);
}

QSize TfFormulaPanel::minimumSizeHint() const {
    QSize s = sizeHint();
    s.setWidth(actions_host_->sizeHint().width() + chrome_width());
    return s;
}

void TfFormulaPanel::place_actions() {
    actions_host_->adjustSize();
    const QRect slot = name_label_->geometry();
    int x            = slot.x();
    int y            = slot.y();
    const int max_x  = width() - actions_host_->width() - 4;
    const int max_y  = height() - actions_host_->height() - 4;
    if (x > max_x)
        x = qMax(4, max_x);
    if (y > max_y)
        y = qMax(4, max_y);
    actions_host_->move(x, y);
    actions_host_->raise();
}

void TfFormulaPanel::set_actions_open(bool on) {
    if (on) {
        place_actions();
        actions_host_->show();
        actions_host_->raise();
        return;
    }
    actions_host_->hide();
}

void TfFormulaPanel::sync_actions() {
    const bool on = !display_->isEmpty();
    copy_btn_->setEnabled(on);
    details_btn_->setEnabled(on);
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
    hide_timer_->stop();
    set_actions_open(underMouse());
}

void TfFormulaPanel::enterEvent(QEnterEvent* event) {
    hide_timer_->stop();
    set_actions_open(true);
    QFrame::enterEvent(event);
}

void TfFormulaPanel::leaveEvent(QEvent* event) {
    hide_timer_->start();
    QFrame::leaveEvent(event);
}

void TfFormulaPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    sync_scroll();
    if (actions_host_->isVisible())
        place_actions();
}

void TfFormulaPanel::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    if (edit_visible_)
        menu.addAction(tr("Изменить"), this, &TfFormulaPanel::editRequested);
    if (paste_visible_)
        menu.addAction(tr("Вставить"), this, &TfFormulaPanel::pasteRequested);
    auto* copy = menu.addAction(tr("Копировать"), this, [this] { copy_clicked(); });
    copy->setEnabled(!display_->isEmpty());
    auto* details = menu.addAction(tr("Подробнее"), this, [this] { details_clicked(); });
    details->setEnabled(!display_->isEmpty());
    menu.exec(event->globalPos());
    hide_timer_->stop();
    set_actions_open(underMouse());
}

bool TfFormulaPanel::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::Wheel)
        return QFrame::eventFilter(watched, event);
    if (watched != clip_ && watched != display_)
        return QFrame::eventFilter(watched, event);

    if (hbar_->maximum() <= hbar_->minimum())
        return QFrame::eventFilter(watched, event);

    const auto* wheel = static_cast<const QWheelEvent*>(event);
    const QPoint pixel = wheel->pixelDelta();
    const QPoint angle = wheel->angleDelta();
    const QPoint delta = pixel.isNull() ? angle / 8 : pixel;
    const int step     = delta.x() != 0 ? delta.x() : delta.y();
    if (step == 0)
        return QFrame::eventFilter(watched, event);

    hbar_->setValue(hbar_->value() - step);
    return true;
}
