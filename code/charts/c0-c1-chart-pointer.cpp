#include "code/charts/c0-c1-chart.h"

#include <algorithm>
#include <cmath>
#include <QChart>
#include <QChartView>
#include <QMouseEvent>
#include <QValueAxis>

bool C0C1Chart::value_at_pixel(const QPoint& viewport_pos, double& c0, double& c1) const {
    if (!chart_ || !view_)
        return false;

    auto axes_x = chart_->axes(Qt::Horizontal);
    auto axes_y = chart_->axes(Qt::Vertical);
    if (axes_x.isEmpty() || axes_y.isEmpty())
        return false;
    auto* ax = qobject_cast<QValueAxis*>(axes_x.constFirst());
    auto* ay = qobject_cast<QValueAxis*>(axes_y.constFirst());
    if (!ax || !ay)
        return false;

    const QPointF scene_pt = view_->mapToScene(viewport_pos);
    const QPointF chart_pt = chart_->mapFromScene(scene_pt);
    const QRectF plot      = chart_->plotArea();
    if (!plot.isValid() || plot.width() < 1.0 || plot.height() < 1.0)
        return false;
    if (!plot.contains(chart_pt))
        return false;

    const double tx = (chart_pt.x() - plot.left()) / plot.width();
    const double ty = (plot.bottom() - chart_pt.y()) / plot.height();
    const double x  = ax->min() + tx * (ax->max() - ax->min());
    const double y  = ay->min() + ty * (ay->max() - ay->min());
    if (!std::isfinite(x) || !std::isfinite(y))
        return false;

    c1 = x;
    c0 = y;
    return true;
}

void C0C1Chart::handle_pointer(const QPoint& viewport_pos, bool force_emit) {
    if (!force_emit && viewport_pos == last_pixel_)
        return;
    last_pixel_ = viewport_pos;

    double c0 = 0.0;
    double c1 = 0.0;
    if (!value_at_pixel(viewport_pos, c0, c1))
        return;

    setSelection(c0, c1);

    double eps0 = 0.0;
    double eps1 = 0.0;
    if (auto axes_x = chart_->axes(Qt::Horizontal); !axes_x.isEmpty()) {
        if (auto* ax = qobject_cast<QValueAxis*>(axes_x.constFirst())) {
            const double w = std::max(1.0, chart_->plotArea().width());
            eps1           = 0.5 * (ax->max() - ax->min()) / w;
        }
    }
    if (auto axes_y = chart_->axes(Qt::Vertical); !axes_y.isEmpty()) {
        if (auto* ay = qobject_cast<QValueAxis*>(axes_y.constFirst())) {
            const double h = std::max(1.0, chart_->plotArea().height());
            eps0           = 0.5 * (ay->max() - ay->min()) / h;
        }
    }
    if (!force_emit && nearly_same(c0, last_emit_c0_, eps0) && nearly_same(c1, last_emit_c1_, eps1))
        return;

    last_emit_c0_ = c0;
    last_emit_c1_ = c1;

    Sample s;
    s.c0    = c0;
    s.c1    = c1;
    s.kp    = c1;
    s.tu    = (c0 > 0.0 && c1 > 0.0) ? (c1 / c0) : 0.0;
    s.omega = 0.0;
    emit samplePicked(s);
}

bool C0C1Chart::eventFilter(QObject* watched, QEvent* event) {
    if (watched != view_->viewport())
        return QWidget::eventFilter(watched, event);

    switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton)
                break;
            double c0 = 0.0, c1 = 0.0;
            if (!value_at_pixel(me->pos(), c0, c1))
                return false;
            dragging_   = true;
            last_pixel_ = {-1, -1};
            view_->viewport()->setCursor(Qt::CrossCursor);
            view_->viewport()->grabMouse();
            handle_pointer(me->pos(), /*force_emit=*/true);
            return true;
        }
        case QEvent::MouseMove: {
            if (!dragging_)
                break;
            auto* me = static_cast<QMouseEvent*>(event);
            if (!(me->buttons() & Qt::LeftButton)) {
                dragging_ = false;
                view_->viewport()->releaseMouse();
                view_->viewport()->unsetCursor();
                break;
            }
            handle_pointer(me->pos(), /*force_emit=*/false);
            return true;
        }
        case QEvent::MouseButtonRelease: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton || !dragging_)
                break;
            dragging_ = false;
            view_->viewport()->releaseMouse();
            view_->viewport()->unsetCursor();
            handle_pointer(me->pos(), /*force_emit=*/false);
            return true;
        }
        case QEvent::MouseButtonDblClick: {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton)
                return true;
            break;
        }
        case QEvent::Leave:
            break;
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}
