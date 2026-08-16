#include "code/charts/c0-c1-chart.h"
#include "code/charts/utils/chart-utils-theme.hpp"

#include <algorithm>
#include <QBrush>
#include <QChart>
#include <QFont>
#include <QGraphicsSimpleTextItem>
#include <QResizeEvent>
#include <QScatterSeries>
#include <QTimer>
#include <QValueAxis>

void C0C1Chart::apply_axis_titles() {
    if (!chart_)
        return;
    auto axes_x = chart_->axes(Qt::Horizontal);
    auto axes_y = chart_->axes(Qt::Vertical);
    if (axes_x.isEmpty() || axes_y.isEmpty())
        return;
    auto* ax = qobject_cast<QValueAxis*>(axes_x.constFirst());
    auto* ay = qobject_cast<QValueAxis*>(axes_y.constFirst());
    if (!ax || !ay)
        return;
    // Keep title text for the detached viewer; hide them here — they steal plot area.
    ax->setTitleText(QStringLiteral("C₁"));
    ay->setTitleText(QStringLiteral("C₀"));
    ax->setTitleVisible(false);
    ay->setTitleVisible(false);

    if (!tag_c1_) {
        tag_c1_ = new QGraphicsSimpleTextItem(QStringLiteral("C₁"), chart_);
        tag_c1_->setZValue(20);
    }
    if (!tag_c0_) {
        tag_c0_ = new QGraphicsSimpleTextItem(QStringLiteral("C₀"), chart_);
        tag_c0_->setZValue(20);
    }
    QFont f = chart_->font();
    f.setPointSize(9);
    f.setBold(true);
    tag_c1_->setFont(f);
    tag_c0_->setFont(f);
    const QBrush ink(chart_utils::currentTheme().text);
    tag_c1_->setBrush(ink);
    tag_c0_->setBrush(ink);
    place_axis_tags();
}

void C0C1Chart::place_axis_tags() {
    if (!chart_ || !tag_c1_ || !tag_c0_)
        return;
    const QRectF plot = chart_->plotArea();
    if (!plot.isValid() || plot.width() < 8.0 || plot.height() < 8.0)
        return;
    constexpr qreal pad = 3.0;
    const QRectF r1     = tag_c1_->boundingRect();
    const QRectF r0     = tag_c0_->boundingRect();
    tag_c1_->setPos(plot.right() - r1.width() - pad, plot.bottom() - r1.height() - pad);
    tag_c0_->setPos(plot.left() + pad, plot.top() + pad);
}

void C0C1Chart::ensure_selection_visible() {
    if (!has_selection_ || !chart_ || dragging_)
        return;
    auto axes_x = chart_->axes(Qt::Horizontal);
    auto axes_y = chart_->axes(Qt::Vertical);
    if (axes_x.isEmpty() || axes_y.isEmpty())
        return;
    auto* ax = qobject_cast<QValueAxis*>(axes_x.constFirst());
    auto* ay = qobject_cast<QValueAxis*>(axes_y.constFirst());
    if (!ax || !ay)
        return;
    const bool outside = sel_c1_ < ax->min() || sel_c1_ > ax->max() || sel_c0_ < ay->min() || sel_c0_ > ay->max();
    if (outside)
        refit_axes();
}

void C0C1Chart::refit_axes() {
    if (!chart_)
        return;
    AxisBounds b;
    auto expand = [&](double x, double y) {
        if (!b.valid) {
            b.min_x = b.max_x = x;
            b.min_y = b.max_y = y;
            b.valid           = true;
            return;
        }
        b.min_x = std::min(b.min_x, x);
        b.max_x = std::max(b.max_x, x);
        b.min_y = std::min(b.min_y, y);
        b.max_y = std::max(b.max_y, y);
    };
    for (const auto& s : locus_)
        expand(s.c1, s.c0);
    for (QScatterSeries* sc : {opt_lik_, opt_ikk_, opt_sko_, selection_series_}) {
        if (!sc)
            continue;
        for (const QPointF& p : sc->points())
            expand(p.x(), p.y());
    }
    for (QScatterSeries* sc : pin_series_) {
        if (!sc)
            continue;
        for (const QPointF& p : sc->points())
            expand(p.x(), p.y());
    }
    if (!b.valid) {
        chart_utils::updateAxes(chart_, {0.0, 1.0}, {0.0, 1.0}, chart_utils::GridMode::Tab, true, true);
        apply_axis_titles();
        return;
    }
    const double hi_x  = std::max(0.0, b.max_x);
    const double hi_y  = std::max(0.0, b.max_y);
    const double pad_x = std::max(1e-9, 0.08 * (hi_x + 1e-12));
    const double pad_y = std::max(1e-9, 0.08 * (hi_y + 1e-12));
    const auto rx      = chart_utils::paddedAxisRange(0.0, hi_x + pad_x, true);
    const auto ry      = chart_utils::paddedAxisRange(0.0, hi_y + pad_y, true);
    chart_utils::updateAxes(chart_, rx, ry, chart_utils::GridMode::Tab, true, true);
    apply_axis_titles();
}

void C0C1Chart::requestRefit() {
    schedule_refit();
}

void C0C1Chart::schedule_refit() {
    if (refit_pending_)
        return;
    refit_pending_ = true;
    QTimer::singleShot(0, this, [this] {
        refit_pending_ = false;
        refit_axes();
    });
}

void C0C1Chart::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    place_axis_tags();
    if (hasLocus() || hasSelection())
        schedule_refit();
}
