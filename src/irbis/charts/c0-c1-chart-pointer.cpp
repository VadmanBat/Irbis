#include "irbis/charts/c0-c1-chart.h"

#include <algorithm>
#include <cmath>
#include <QChart>
#include <QChartView>
#include <QMouseEvent>
#include <QValueAxis>

bool C0C1Chart::value_at_pixel(const QPoint& viewport_pos, double& x, double& y) const {
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
    x               = ax->min() + tx * (ax->max() - ax->min());
    y               = ay->min() + ty * (ay->max() - ay->min());
    return std::isfinite(x) && std::isfinite(y);
}

void C0C1Chart::handle_pointer(const QPoint& viewport_pos, bool force_emit) {
    if (!force_emit && viewport_pos == last_pixel_)
        return;
    last_pixel_ = viewport_pos;

    double x = 0.0;
    double y = 0.0;
    if (!value_at_pixel(viewport_pos, x, y))
        return;

    const bool pd   = plane_ == Plane::Pd;
    const double c1 = pd ? y : x;
    const double c0 = pd ? 0.0 : y;
    const double c2 = pd ? x : 0.0;
    setSelection(c0, c1, c2);

    double eps_x = 0.0;
    double eps_y = 0.0;
    if (auto axes_x = chart_->axes(Qt::Horizontal); !axes_x.isEmpty()) {
        if (auto* ax = qobject_cast<QValueAxis*>(axes_x.constFirst())) {
            const double w = std::max(1.0, chart_->plotArea().width());
            eps_x          = 0.5 * (ax->max() - ax->min()) / w;
        }
    }
    if (auto axes_y = chart_->axes(Qt::Vertical); !axes_y.isEmpty()) {
        if (auto* ay = qobject_cast<QValueAxis*>(axes_y.constFirst())) {
            const double h = std::max(1.0, chart_->plotArea().height());
            eps_y          = 0.5 * (ay->max() - ay->min()) / h;
        }
    }
    if (!force_emit && nearly_same(x, last_emit_x_, eps_x) && nearly_same(y, last_emit_y_, eps_y))
        return;

    last_emit_x_ = x;
    last_emit_y_ = y;

    Sample s;
    s.c0    = c0;
    s.c1    = c1;
    s.c2    = c2;
    s.kp    = c1;
    s.omega = 0.0;
    if (pd)
        s.td = (c1 > 0.0) ? (c2 / c1) : 0.0;
    else
        s.ti = (c0 > 0.0 && c1 > 0.0) ? (c1 / c0) : 0.0;
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
            double x = 0.0, y = 0.0;
            if (!value_at_pixel(me->pos(), x, y))
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
