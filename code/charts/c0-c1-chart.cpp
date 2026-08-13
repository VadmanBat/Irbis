#include "code/charts/c0-c1-chart.h"

#include "code/charts/utils/chart-utils-theme.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QChart>
#include <QEvent>
#include <QLineSeries>
#include <QMargins>
#include <QMouseEvent>
#include <QScatterSeries>
#include <QVBoxLayout>
#include <QValueAxis>

namespace {
QScatterSeries* make_marker(const QString& name, const QColor& color, QScatterSeries::MarkerShape shape,
                            qreal size) {
    auto* s = new QScatterSeries;
    s->setName(name);
    s->setMarkerShape(shape);
    s->setMarkerSize(size);
    s->setColor(color);
    s->setBorderColor(color.darker(120));
    s->setPen(QPen(color.darker(120), 1.2));
    return s;
}

bool nearly_same(double a, double b, double eps) noexcept {
    if (!std::isfinite(a) || !std::isfinite(b))
        return false;
    return std::abs(a - b) <= eps;
}
}

C0C1Chart::C0C1Chart(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("C0C1Chart"));
    build_chart();
}

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
    // X = C₁, Y = C₀
    ax->setTitleText(QStringLiteral("C₁"));
    ay->setTitleText(QStringLiteral("C₀"));
    ax->setTitleVisible(true);
    ay->setTitleVisible(true);
}

void C0C1Chart::build_chart() {
    chart_ = new QChart;
    view_  = chart_utils::makeChartView(chart_, this, tr("РКЧХ: C₁–C₀"), QStringLiteral("C₁"), QStringLiteral("C₀"));
    view_->setMinimumWidth(220);
    view_->setMouseTracking(true);
    view_->viewport()->setMouseTracking(true);
    view_->viewport()->installEventFilter(this);
    chart_->setMargins(QMargins(12, 8, 12, 16));

    locus_series_ = new QLineSeries;
    locus_series_->setName(tr("C₁(ω), C₀(ω)"));
    locus_series_->setPen(chart_utils::penForIndex(0));

    const bool dark    = chart_utils::isDarkTheme();
    const QColor lik   = dark ? QColor(0x6c, 0xb6, 0xff) : QColor(0x1f, 0x77, 0xb4);
    const QColor ikk   = dark ? QColor(0xff, 0xb0, 0x4a) : QColor(0xff, 0x7f, 0x0e);
    const QColor sko   = dark ? QColor(0x5d, 0xdf, 0x5d) : QColor(0x2c, 0xa0, 0x2c);
    const QColor sel   = dark ? QColor(0xff, 0x6b, 0x6b) : QColor(0xd6, 0x27, 0x28);

    opt_lik_          = make_marker(tr("опт. ЛИК"), lik, QScatterSeries::MarkerShapeCircle, 12.0);
    opt_ikk_          = make_marker(tr("опт. ИКК"), ikk, QScatterSeries::MarkerShapeRectangle, 11.0);
    opt_sko_          = make_marker(tr("опт. СКО"), sko, QScatterSeries::MarkerShapeTriangle, 13.0);
    selection_series_ = make_marker(tr("выбор"), sel, QScatterSeries::MarkerShapeCircle, 16.0);
    selection_series_->setBorderColor(sel.lighter(140));

    chart_->addSeries(locus_series_);
    chart_->addSeries(opt_lik_);
    chart_->addSeries(opt_ikk_);
    chart_->addSeries(opt_sko_);
    chart_->addSeries(selection_series_);
    for (QAbstractSeries* s : chart_->series()) {
        for (QAbstractAxis* ax : chart_->axes())
            s->attachAxis(ax);
    }

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->addWidget(view_);
    apply_axis_titles();
    apply_theme();
}

void C0C1Chart::apply_theme() {
    chart_utils::applyChartTheme(chart_, view_);
    if (locus_series_)
        locus_series_->setPen(chart_utils::penForIndex(0));
    apply_axis_titles();
}

void C0C1Chart::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::ThemeChange)
        apply_theme();
    QWidget::changeEvent(event);
}

void C0C1Chart::clear() {
    locus_.clear();
    has_selection_ = false;
    dragging_      = false;
    last_pixel_    = {-1, -1};
    last_emit_c0_  = std::numeric_limits<double>::quiet_NaN();
    last_emit_c1_  = std::numeric_limits<double>::quiet_NaN();
    if (locus_series_)
        locus_series_->clear();
    if (opt_lik_)
        opt_lik_->clear();
    if (opt_ikk_)
        opt_ikk_->clear();
    if (opt_sko_)
        opt_sko_->clear();
    if (selection_series_)
        selection_series_->clear();
    refit_axes();
}

void C0C1Chart::setLocus(std::vector<Sample> samples, const QString& name) {
    locus_ = std::move(samples);
    if (!locus_series_)
        return;
    if (!name.isEmpty())
        locus_series_->setName(name);
    QList<QPointF> pts;
    pts.reserve(static_cast<int>(locus_.size()));
    for (const auto& s : locus_)
        pts.append(to_plot(s.c0, s.c1)); // X=C₁, Y=C₀
    locus_series_->replace(std::move(pts));
    refit_axes();
    update_selection_marker();
}

void C0C1Chart::setOptima(const Optimum& lik, const Optimum& ikk, const Optimum& sko) {
    auto put = [](QScatterSeries* series, const Optimum& o) {
        if (!series)
            return;
        series->clear();
        if (o.valid)
            series->append(to_plot(o.c0, o.c1));
        if (!o.label.isEmpty())
            series->setName(o.label);
    };
    put(opt_lik_, lik);
    put(opt_ikk_, ikk);
    put(opt_sko_, sko);
    refit_axes();
}

void C0C1Chart::setSelection(double c0, double c1) {
    if (has_selection_ && nearly_same(sel_c0_, c0, 0.0) && nearly_same(sel_c1_, c1, 0.0)) {
        // exact same — still refresh marker if empty
        if (selection_series_ && selection_series_->count() == 0)
            update_selection_marker();
        return;
    }
    has_selection_ = std::isfinite(c0) && std::isfinite(c1);
    sel_c0_        = c0;
    sel_c1_        = c1;
    update_selection_marker();
    // Never refit while dragging — axes jumps make pixel→value mapping feel sticky.
    if (!dragging_)
        ensure_selection_visible();
}

void C0C1Chart::clearSelection() {
    has_selection_ = false;
    if (selection_series_)
        selection_series_->clear();
}

void C0C1Chart::update_selection_marker() {
    if (!selection_series_)
        return;
    selection_series_->clear();
    if (has_selection_)
        selection_series_->append(to_plot(sel_c0_, sel_c1_));
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
    // Plot: X=C₁, Y=C₀
    const bool outside =
        sel_c1_ < ax->min() || sel_c1_ > ax->max() || sel_c0_ < ay->min() || sel_c0_ > ay->max();
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
        expand(s.c1, s.c0); // X=C₁, Y=C₀
    for (QScatterSeries* sc : {opt_lik_, opt_ikk_, opt_sko_, selection_series_}) {
        if (!sc)
            continue;
        for (const QPointF& p : sc->points())
            expand(p.x(), p.y());
    }
    if (!b.valid) {
        chart_utils::updateAxes(chart_, {0.0, 1.0}, {0.0, 1.0}, chart_utils::GridMode::Tab,
                                /*snap_x=*/false, /*snap_y=*/false);
        return;
    }
    const double hi_x  = std::max(0.0, b.max_x);
    const double hi_y  = std::max(0.0, b.max_y);
    const double pad_x = std::max(1e-9, 0.08 * (hi_x + 1e-12));
    const double pad_y = std::max(1e-9, 0.08 * (hi_y + 1e-12));
    // First quadrant only: lo = 0, hi as data max + pad. No lattice snap.
    const auto rx = chart_utils::paddedAxisRange(0.0, hi_x + pad_x, /*include_zero=*/true);
    const auto ry = chart_utils::paddedAxisRange(0.0, hi_y + pad_y, /*include_zero=*/true);
    chart_utils::updateAxes(chart_, rx, ry, chart_utils::GridMode::Tab, /*snap_x=*/false, /*snap_y=*/false);
}

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

    // Viewport → chart item coords → linear map in plotArea (stable, no snap).
    const QPointF scene_pt = view_->mapToScene(viewport_pos);
    const QPointF chart_pt = chart_->mapFromScene(scene_pt);
    const QRectF plot      = chart_->plotArea();
    if (!plot.isValid() || plot.width() < 1.0 || plot.height() < 1.0)
        return false;
    if (!plot.contains(chart_pt))
        return false;

    const double tx = (chart_pt.x() - plot.left()) / plot.width();
    const double ty = (plot.bottom() - chart_pt.y()) / plot.height(); // Y up in data
    const double x  = ax->min() + tx * (ax->max() - ax->min());       // C₁
    const double y  = ay->min() + ty * (ay->max() - ay->min());       // C₀
    if (!std::isfinite(x) || !std::isfinite(y))
        return false;

    c1 = x;
    c0 = y;
    return true;
}

void C0C1Chart::handle_pointer(const QPoint& viewport_pos, bool force_emit) {
    // One update per widget pixel — smooth free pick, no subpixel spam.
    if (!force_emit && viewport_pos == last_pixel_)
        return;
    last_pixel_ = viewport_pos;

    double c0 = 0.0;
    double c1 = 0.0;
    if (!value_at_pixel(viewport_pos, c0, c1))
        return;

    // Marker follows every new pixel.
    setSelection(c0, c1);

    // Emit / recalculate only when value coords actually changed (beyond ~½ pixel).
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
            last_pixel_ = {-1, -1}; // force first sample
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
            // Final sample at release (in case last move was filtered).
            handle_pointer(me->pos(), /*force_emit=*/false);
            return true;
        }
        case QEvent::Leave: {
            if (!dragging_)
                break;
            // Keep drag if mouse grabbed; Leave can fire during grab on some platforms.
            break;
        }
        default:
            break;
    }
    return QWidget::eventFilter(watched, event);
}
