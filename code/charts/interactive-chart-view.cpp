#include "code/charts/interactive-chart-view.h"

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

void InteractiveChartView::sync_axes_after_view_change() {
    auto* ax = axis_x();
    auto* ay = axis_y();
    if (!ax || !ay)
        return;
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
    // QChart::zoom(f): f > 1 zooms in, 0 < f < 1 zooms out.
    chart()->zoom(1.25);
    sync_axes_after_view_change();
    emit viewChanged();
}

void InteractiveChartView::zoomOutStep() {
    if (!chart())
        return;
    chart()->zoom(0.8);
    sync_axes_after_view_change();
    emit viewChanged();
}

void InteractiveChartView::resetView(const chart_utils::Pair& home_x, const chart_utils::Pair& home_y) {
    if (!chart())
        return;
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
    const QPointF before    = chart()->mapToValue(chart_pos);
    // Wheel up → zoom in (factor > 1); wheel down → zoom out.
    constexpr double k_step = 1.25;
    chart()->zoom(event->angleDelta().y() > 0 ? k_step : 1.0 / k_step);
    const QPointF after = chart()->mapToValue(chart_pos);
    auto* ax            = axis_x();
    auto* ay            = axis_y();
    if (ax && ay) {
        ax->setRange(ax->min() + (before.x() - after.x()), ax->max() + (before.x() - after.x()));
        ay->setRange(ay->min() + (before.y() - after.y()), ay->max() + (before.y() - after.y()));
    }
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
