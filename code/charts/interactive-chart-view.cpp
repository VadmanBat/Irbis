#include "code/charts/interactive-chart-view.h"

#include <algorithm>
#include <cmath>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QValueAxis>
#include <QWheelEvent>

namespace {
void apply_axis_grid(QChart* chart, bool on) {
    if (!chart)
        return;
    if (auto* ax = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0, nullptr))) {
        ax->setGridLineVisible(on);
        ax->setMinorGridLineVisible(on);
    }
    if (auto* ay = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0, nullptr))) {
        ay->setGridLineVisible(on);
        ay->setMinorGridLineVisible(on);
    }
}
} // namespace

namespace chart_viewer {
QValueAxis* InteractiveChartView::axis_x() const {
    return chart() ? qobject_cast<QValueAxis*>(chart()->axes(Qt::Horizontal).value(0, nullptr)) : nullptr;
}

QValueAxis* InteractiveChartView::axis_y() const {
    return chart() ? qobject_cast<QValueAxis*>(chart()->axes(Qt::Vertical).value(0, nullptr)) : nullptr;
}

InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent) : QChartView(chart, parent) {
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setRubberBand(QChartView::RectangleRubberBand);
    apply_tool_cursor();
    apply_axis_grid(chart, grid_on_);
    if (chart)
        chart_utils::applyViewerGrid(chart);
    apply_theme();
}

void InteractiveChartView::apply_theme() {
    chart_utils::applyChartTheme(chart(), this);
    // Theme only restyles; keep toolbar grid toggle.
    apply_axis_grid(chart(), grid_on_);
}

void InteractiveChartView::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::ThemeChange)
        apply_theme();
    QChartView::changeEvent(event);
}

void InteractiveChartView::apply_tool_cursor() {
    switch (tool_) {
        case Tool::Pan:
            setCursor(Qt::OpenHandCursor);
            setRubberBand(QChartView::NoRubberBand);
            break;
        case Tool::ZoomRect:
            setCursor(Qt::CrossCursor);
            setRubberBand(QChartView::RectangleRubberBand);
            break;
    }
}

void InteractiveChartView::setTool(Tool tool) {
    tool_    = tool;
    panning_ = false;
    apply_tool_cursor();
}

void InteractiveChartView::clamp_axis_spans() {
    auto clamp = [](QValueAxis* a) {
        if (!a)
            return;
        const double lo = a->min();
        const double hi = a->max();
        if (!std::isfinite(lo) || !std::isfinite(hi))
            return;
        const double c     = 0.5 * (lo + hi);
        const double min_s = std::max(1e-15, 1e-12 * std::max(1.0, std::abs(c)));
        if (std::isfinite(c) && hi - lo >= min_s)
            return;
        const double mid = std::isfinite(c) ? c : 0.0;
        a->setRange(mid - 0.5 * min_s, mid + 0.5 * min_s);
    };
    clamp(axis_x());
    clamp(axis_y());
}

void InteractiveChartView::zoom_about(const QPointF& value, double factor) {
    auto zoom_one = [&](QValueAxis* a, double center) {
        if (!a || !(factor > 0.0) || !std::isfinite(center))
            return;
        const double lo = a->min();
        const double hi = a->max();
        if (!std::isfinite(lo) || !std::isfinite(hi) || !(hi > lo))
            return;
        const double span  = hi - lo;
        const double min_s = std::max(1e-15, 1e-12 * std::max(1.0, std::abs(center)));
        double new_span    = span / factor;
        if (new_span < min_s)
            new_span = min_s;
        const double t      = (center - lo) / span;
        const double new_lo = center - t * new_span;
        const double new_hi = new_lo + new_span;
        if (std::isfinite(new_lo) && std::isfinite(new_hi) && new_hi > new_lo) {
            chart_utils::applyViewerGrid(a, new_lo, new_hi);
            a->setRange(new_lo, new_hi);
        }
    };
    zoom_one(axis_x(), value.x());
    zoom_one(axis_y(), value.y());
}

void InteractiveChartView::sync_axes_after_view_change() {
    auto* ax = axis_x();
    auto* ay = axis_y();
    if (!ax || !ay)
        return;
    clamp_axis_spans();
    // Tick lattice only — must not re-enable grid if user turned it off.
    chart_utils::applyViewerGrid(ax);
    chart_utils::applyViewerGrid(ay);
    apply_axis_grid(chart(), grid_on_);
    chart_utils::updateOriginGuides(chart(), {ax->min(), ax->max()}, {ay->min(), ay->max()});
}

void InteractiveChartView::pan_by_pixels(int dx_px, int dy_px) {
    auto* ax = axis_x();
    auto* ay = axis_y();
    if (!ax || !ay || !chart())
        return;
    const QRectF area = chart()->plotArea();
    if (!(area.width() > 1.0) || !(area.height() > 1.0))
        return;
    const double dx = -dx_px * (ax->max() - ax->min()) / area.width();
    const double dy = dy_px * (ay->max() - ay->min()) / area.height();
    ax->setRange(ax->min() + dx, ax->max() + dx);
    ay->setRange(ay->min() + dy, ay->max() + dy);
}

void InteractiveChartView::zoomInStep() {
    if (!chart())
        return;
    zoom_about(chart()->mapToValue(chart()->plotArea().center()), 1.25);
    sync_axes_after_view_change();
    emit viewChanged();
}

void InteractiveChartView::zoomOutStep() {
    if (!chart())
        return;
    zoom_about(chart()->mapToValue(chart()->plotArea().center()), 0.8);
    sync_axes_after_view_change();
    emit viewChanged();
}

void InteractiveChartView::resetView(const chart_utils::Pair& home_x, const chart_utils::Pair& home_y) {
    if (!chart())
        return;
    // Ticks for the home window first — zoomReset/setRange on a leftover tiny
    // interval would paint 10^n grid lines and freeze.
    if (auto* ax = axis_x())
        chart_utils::applyViewerGrid(ax, home_x.first, home_x.second);
    if (auto* ay = axis_y())
        chart_utils::applyViewerGrid(ay, home_y.first, home_y.second);
    chart()->zoomReset();
    chart_utils::updateAxes(chart(), home_x, home_y, chart_utils::GridMode::Viewer, false, false);
    apply_axis_grid(chart(), grid_on_);
    emit viewChanged();
}

void InteractiveChartView::setGridVisible(bool on) {
    grid_on_ = on;
    apply_axis_grid(chart(), on);
}

void InteractiveChartView::mousePressEvent(QMouseEvent* event) {
    if ((tool_ == Tool::Pan && event->button() == Qt::LeftButton) || event->button() == Qt::MiddleButton) {
        panning_  = true;
        last_pos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPoint delta = event->pos() - last_pos_;
        last_pos_          = event->pos();
        pan_by_pixels(delta.x(), delta.y());
        sync_axes_after_view_change();
        emit viewChanged();
        event->accept();
    }
    else {
        QChartView::mouseMoveEvent(event);
    }

    if (chart()) {
        // event pos is view coords; mapToValue expects chart item coords.
        const QPointF chart_pos = chart()->mapFromScene(mapToScene(event->pos()));
        const QPointF v         = chart()->mapToValue(chart_pos);
        const bool inside       = chart()->plotArea().contains(chart_pos);
        emit cursorMoved(v.x(), v.y(), inside);
    }
}

void InteractiveChartView::mouseReleaseEvent(QMouseEvent* event) {
    if (panning_ && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)) {
        panning_ = false;
        apply_tool_cursor();
        sync_axes_after_view_change();
        emit viewChanged();
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
    sync_axes_after_view_change();
    emit viewChanged();
}

void InteractiveChartView::wheelEvent(QWheelEvent* event) {
    if (!chart() || event->angleDelta().y() == 0) {
        QChartView::wheelEvent(event);
        return;
    }
    const QPointF chart_pos = chart()->mapFromScene(mapToScene(event->position().toPoint()));
    const QPointF focus     = chart()->mapToValue(chart_pos);
    if (!std::isfinite(focus.x()) || !std::isfinite(focus.y())) {
        event->accept();
        return;
    }
    // Wheel up → zoom in (factor > 1); wheel down → zoom out.
    constexpr double k_step = 1.25;
    zoom_about(focus, event->angleDelta().y() > 0 ? k_step : 1.0 / k_step);
    sync_axes_after_view_change();
    emit viewChanged();
    event->accept();
}

void InteractiveChartView::mouseDoubleClickEvent(QMouseEvent* event) {
    QChartView::mouseDoubleClickEvent(event);
    emit viewChanged();
}

void InteractiveChartView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        zoomInStep();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Minus) {
        zoomOutStep();
        event->accept();
        return;
    }
    QChartView::keyPressEvent(event);
}
} // namespace chart_viewer
