#include "code/charts/c0-c1-chart.h"

#include "code/charts/utils/chart-utils-detail.hpp"
#include "code/charts/utils/chart-utils-theme.hpp"

#include <algorithm>
#include <cmath>
#include <QAbstractSeries>
#include <QApplication>
#include <QChart>
#include <QChartView>
#include <QEvent>
#include <QFont>
#include <QGraphicsLayout>
#include <QImage>
#include <QLegend>
#include <QLineSeries>
#include <QMargins>
#include <QPainter>
#include <QScatterSeries>
#include <QValueAxis>
#include <QVBoxLayout>

namespace {
constexpr int kCrossMark = 16;
constexpr int kRingMark  = 22;
constexpr int kSelMark   = 16;
constexpr qreal kMergePx = 12.0;
const QColor kLikGreen{0x2e, 0xc8, 0x54};
const QColor kIkkRed{0xe5, 0x39, 0x35};
const QColor kSkoBlue{0x1e, 0x88, 0xe5};

enum class OptGlyph { Plus, Cross, Ring };

QImage opt_glyph(OptGlyph glyph, const QColor& color) {
    const int size    = glyph == OptGlyph::Ring ? kRingMark : kCrossMark;
    const qreal pen_w = glyph == OptGlyph::Ring ? 2.2 : 2.0;
    const qreal dpr   = qApp ? qApp->devicePixelRatio() : 1.0;
    QImage img(qRound(size * dpr), qRound(size * dpr), QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(dpr);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, pen_w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal mid = size * 0.5;
    const qreal r   = size * (glyph == OptGlyph::Ring ? 0.38 : 0.36);
    switch (glyph) {
        case OptGlyph::Plus:
            p.drawLine(QPointF(mid, mid - r), QPointF(mid, mid + r));
            p.drawLine(QPointF(mid - r, mid), QPointF(mid + r, mid));
            break;
        case OptGlyph::Cross: {
            const qreal d = r * 0.70710678118;
            p.drawLine(QPointF(mid - d, mid - d), QPointF(mid + d, mid + d));
            p.drawLine(QPointF(mid + d, mid - d), QPointF(mid - d, mid + d));
            break;
        }
        case OptGlyph::Ring:
            p.drawEllipse(QPointF(mid, mid), r, r);
            break;
    }
    return img;
}

void paint_legend_swatch(QScatterSeries* series, const QColor& color) {
    series->setColor(color);
    series->setBorderColor(color);
    series->setBrush(QBrush(color));
    series->setPen(QPen(color, 1.0));
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
}

void style_opt_glyph(QScatterSeries* series, OptGlyph glyph, const QColor& color) {
    if (!series)
        return;
    const int size = glyph == OptGlyph::Ring ? kRingMark : kCrossMark;
    paint_legend_swatch(series, color);
    series->setMarkerSize(size);
    series->setLightMarker(opt_glyph(glyph, color));
}

QImage sel_glyph(const QColor& color) {
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QImage img(qRound(kSelMark * dpr), qRound(kSelMark * dpr), QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(dpr);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    const qreal mid = kSelMark * 0.5;
    const qreal r   = kSelMark * 0.32;
    p.setPen(QPen(color.lighter(140), 1.4));
    p.setBrush(color);
    p.drawEllipse(QPointF(mid, mid), r, r);
    return img;
}

qreal pixel_dist(QChart* chart, const QPointF& a, const QPointF& b) {
    if (!chart)
        return 1e300;
    auto* ax          = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).value(0, nullptr));
    auto* ay          = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).value(0, nullptr));
    const QRectF plot = chart->plotArea();
    if (!ax || !ay || !plot.isValid())
        return 1e300;
    const double sx = ax->max() - ax->min();
    const double sy = ay->max() - ay->min();
    if (!(sx > 0.0) || !(sy > 0.0))
        return 1e300;
    const double dx = (a.x() - b.x()) / sx * plot.width();
    const double dy = (a.y() - b.y()) / sy * plot.height();
    return std::hypot(dx, dy);
}

void merge_close(QPointF& a, QPointF& b) {
    a = b = QPointF(0.5 * (a.x() + b.x()), 0.5 * (a.y() + b.y()));
}

void nest_optima(QChart* chart, QPointF* lik, QPointF* ikk, QPointF* sko) {
    const bool hl = lik != nullptr;
    const bool hi = ikk != nullptr;
    const bool hs = sko != nullptr;
    auto close    = [&](const QPointF& a, const QPointF& b) { return pixel_dist(chart, a, b) < kMergePx; };

    if (hl && hi && hs && close(*lik, *ikk) && close(*ikk, *sko) && close(*lik, *sko)) {
        const QPointF c((lik->x() + ikk->x() + sko->x()) / 3.0, (lik->y() + ikk->y() + sko->y()) / 3.0);
        *lik = *ikk = *sko = c;
        return;
    }
    if (hl && hi && close(*lik, *ikk))
        merge_close(*lik, *ikk);
    if (hl && hs && close(*lik, *sko))
        merge_close(*lik, *sko);
    if (hi && hs && close(*ikk, *sko))
        merge_close(*ikk, *sko);
}

void style_selection_marker(QScatterSeries* series, const QColor& color) {
    if (!series)
        return;
    paint_legend_swatch(series, color);
    series->setMarkerSize(kSelMark);
    series->setLightMarker(sel_glyph(color));
}

QPen locus_pen() {
    const QColor c = chart_utils::isDarkTheme() ? QColor(0x66, 0xbb, 0x6a) : QColor(0x2e, 0x7d, 0x32);
    QPen pen(c, 2.0, Qt::DashLine);
    pen.setCapStyle(Qt::FlatCap);
    pen.setCosmetic(true);
    return pen;
}

QScatterSeries* make_selection_marker(const QString& name, const QColor& color) {
    auto* s = new QScatterSeries;
    s->setName(name);
    style_selection_marker(s, color);
    return s;
}
}

C0C1Chart::C0C1Chart(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("C0C1Chart"));
    build_chart();
}

void C0C1Chart::build_chart() {
    chart_ = new QChart;
    view_  = chart_utils::makeChartView(chart_, this, tr("Допустимые настройки"), QStringLiteral("C₁"),
                                        QStringLiteral("C₀"));
    view_->setMinimumSize(0, 0);
    view_->setMouseTracking(true);
    view_->viewport()->setMouseTracking(true);
    view_->viewport()->installEventFilter(this);
    tighten_plot();

    locus_series_ = new QLineSeries;
    locus_series_->setName(tr("ЛРЗ"));
    locus_series_->setPen(locus_pen());

    const bool dark  = chart_utils::isDarkTheme();
    const QColor sel = dark ? QColor(0xff, 0x6b, 0x6b) : QColor(0xd6, 0x27, 0x28);

    opt_lik_ = new QScatterSeries;
    opt_lik_->setName(tr("опт. ЛИК"));
    opt_ikk_ = new QScatterSeries;
    opt_ikk_->setName(tr("опт. ИКК"));
    opt_sko_ = new QScatterSeries;
    opt_sko_->setName(tr("опт. СКО"));
    style_opt_glyph(opt_lik_, OptGlyph::Plus, kLikGreen);
    style_opt_glyph(opt_ikk_, OptGlyph::Cross, kIkkRed);
    style_opt_glyph(opt_sko_, OptGlyph::Ring, kSkoBlue);
    selection_series_ = make_selection_marker(tr("выбор"), sel);

    chart_->addSeries(locus_series_);
    chart_->addSeries(opt_sko_);
    chart_->addSeries(opt_lik_);
    chart_->addSeries(opt_ikk_);
    chart_->addSeries(selection_series_);
    chart_utils::detail::attachToAxes(chart_, locus_series_);
    chart_utils::detail::attachToAxes(chart_, opt_sko_);
    chart_utils::detail::attachToAxes(chart_, opt_lik_);
    chart_utils::detail::attachToAxes(chart_, opt_ikk_);
    chart_utils::detail::attachToAxes(chart_, selection_series_);

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->addWidget(view_);
    apply_axis_titles();
    apply_theme();
    QObject::connect(chart_, &QChart::plotAreaChanged, this, [this](const QRectF&) { place_axis_tags(); });
}

void C0C1Chart::tighten_plot() {
    if (!chart_)
        return;
    if (QLegend* lg = chart_->legend())
        lg->setVisible(false);
    if (QGraphicsLayout* lay = chart_->layout())
        lay->setContentsMargins(0, 0, 0, 0);
    chart_->setBackgroundRoundness(0);
    chart_->setMargins(QMargins(2, 2, 2, 2));
    QFont title_font = chart_->titleFont();
    title_font.setPointSize(10);
    chart_->setTitleFont(title_font);
}

void C0C1Chart::apply_theme() {
    chart_utils::applyChartTheme(chart_, view_);
    if (locus_series_)
        locus_series_->setPen(locus_pen());
    style_opt_glyph(opt_lik_, OptGlyph::Plus, kLikGreen);
    style_opt_glyph(opt_ikk_, OptGlyph::Cross, kIkkRed);
    style_opt_glyph(opt_sko_, OptGlyph::Ring, kSkoBlue);
    style_live_marker();
    restyle_pins();
    apply_axis_titles();
    tighten_plot();
}

void C0C1Chart::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::ThemeChange)
        apply_theme();
    QWidget::changeEvent(event);
}

void C0C1Chart::setSquareSide(int side) {
    const int s = std::max(160, side);
    setFixedSize(s, s);
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
    clear_pins();
    live_index_ = 0;
    style_live_marker();
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
        pts.append(to_plot(s.c0, s.c1));
    locus_series_->replace(std::move(pts));
    refit_axes();
    update_selection_marker();
    schedule_refit();
}

void C0C1Chart::setOptima(const Optimum& lik, const Optimum& ikk, const Optimum& sko) {
    QPointF p_lik = to_plot(lik.c0, lik.c1);
    QPointF p_ikk = to_plot(ikk.c0, ikk.c1);
    QPointF p_sko = to_plot(sko.c0, sko.c1);

    auto put = [](QScatterSeries* series, bool valid, const QPointF& pt, const QString& label) {
        if (!series)
            return;
        series->clear();
        if (valid)
            series->append(pt);
        if (!label.isEmpty())
            series->setName(label);
    };
    put(opt_lik_, lik.valid, p_lik, lik.label);
    put(opt_ikk_, ikk.valid, p_ikk, ikk.label);
    put(opt_sko_, sko.valid, p_sko, sko.label);
    refit_axes();

    nest_optima(chart_, lik.valid ? &p_lik : nullptr, ikk.valid ? &p_ikk : nullptr, sko.valid ? &p_sko : nullptr);
    put(opt_lik_, lik.valid, p_lik, {});
    put(opt_ikk_, ikk.valid, p_ikk, {});
    put(opt_sko_, sko.valid, p_sko, {});
    schedule_refit();
}

void C0C1Chart::setSelection(double c0, double c1) {
    if (has_selection_ && nearly_same(sel_c0_, c0, 0.0) && nearly_same(sel_c1_, c1, 0.0)) {
        if (selection_series_ && selection_series_->count() == 0)
            update_selection_marker();
        return;
    }
    has_selection_ = std::isfinite(c0) && std::isfinite(c1);
    sel_c0_        = c0;
    sel_c1_        = c1;
    update_selection_marker();
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

void C0C1Chart::style_live_marker() {
    style_selection_marker(selection_series_, chart_utils::penForIndex(live_index_).color());
}

void C0C1Chart::style_pin_marker(QScatterSeries* series, std::size_t color_index) {
    style_selection_marker(series, chart_utils::penForIndex(color_index).color());
}

void C0C1Chart::restyle_pins() {
    for (std::size_t i = 0; i < pin_series_.size(); ++i)
        style_pin_marker(pin_series_[i], i);
}

void C0C1Chart::clear_pins() {
    if (!chart_) {
        pin_series_.clear();
        return;
    }
    for (QScatterSeries* s : pin_series_) {
        if (!s)
            continue;
        chart_->removeSeries(s);
        delete s;
    }
    pin_series_.clear();
}

void C0C1Chart::setLiveIndex(std::size_t index) {
    live_index_ = index;
    style_live_marker();
}

void C0C1Chart::pinSelection(const QString& name) {
    if (!chart_ || !has_selection_) {
        ++live_index_;
        style_live_marker();
        return;
    }

    auto* pin = new QScatterSeries;
    pin->setName(name.isEmpty() ? tr("фикс. %1").arg(pin_series_.size() + 1) : name);
    pin->append(to_plot(sel_c0_, sel_c1_));
    style_pin_marker(pin, live_index_);

    if (selection_series_)
        chart_->removeSeries(selection_series_);
    chart_->addSeries(pin);
    chart_utils::detail::attachToAxes(chart_, pin);
    if (selection_series_) {
        chart_->addSeries(selection_series_);
        chart_utils::detail::attachToAxes(chart_, selection_series_);
    }
    pin_series_.push_back(pin);

    ++live_index_;
    style_live_marker();
    schedule_refit();
}

void C0C1Chart::trimPins(std::size_t max_count) {
    if (!chart_)
        return;
    while (pin_series_.size() > max_count) {
        QScatterSeries* s = pin_series_.front();
        pin_series_.erase(pin_series_.begin());
        if (!s)
            continue;
        chart_->removeSeries(s);
        delete s;
    }
    restyle_pins();
}
