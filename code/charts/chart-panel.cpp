#include "code/charts/chart-panel.h"

#include <QChart>
#include <QChartView>
#include <QEvent>
#include <QVBoxLayout>

ChartPanel::ChartPanel(const QString& title, const QString& titleX, const QString& titleY, QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("ChartPanel"));
    chart_  = new QChart;
    view_   = chart_utils::makeChartView(chart_, this, title, titleX, titleY);
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 0);
    layout_->addWidget(view_);
}

void ChartPanel::apply_theme() {
    chart_utils::applyChartTheme(chart_, view_);
}

void ChartPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange ||
        event->type() == QEvent::ThemeChange)
        apply_theme();
    QWidget::changeEvent(event);
}

void ChartPanel::setChartTitle(const QString& title) {
    if (chart_)
        chart_->setTitle(title);
}

AxisBounds ChartPanel::addRealCurve(const VecPair& points, const QString& name) {
    const auto w = chart_utils::addRealSeries(chart_, points, name, curve_count_);
    if (w.wrote)
        ++curve_count_;
    return w.bounds;
}

AxisBounds ChartPanel::addComplexCurve(const VecComp& points, const QString& name) {
    const auto w = chart_utils::addComplexSeries(chart_, points, name, curve_count_);
    if (w.wrote)
        ++curve_count_;
    return w.bounds;
}

AxisBounds ChartPanel::addBand(const VecPair& lower, const VecPair& upper, const QString& name) {
    return chart_utils::addBandSeries(chart_, lower, upper, name).bounds;
}

AxisBounds ChartPanel::replaceLastRealCurve(const VecPair& points, const QString& name) {
    if (curve_count_ == 0)
        return addRealCurve(points, name);

    const auto w = chart_utils::replaceLastRealSeries(chart_, points, name);
    if (w.wrote)
        return w.bounds;

    // No data series on chart — add without bumping curve_count_ (already counted).
    return chart_utils::addRealSeries(chart_, points, name, curve_count_ - 1).bounds;
}

AxisBounds ChartPanel::replaceLastComplexCurve(const VecComp& points, const QString& name) {
    if (curve_count_ == 0)
        return addComplexCurve(points, name);

    const auto w = chart_utils::replaceLastComplexSeries(chart_, points, name);
    if (w.wrote)
        return w.bounds;

    return chart_utils::addComplexSeries(chart_, points, name, curve_count_ - 1).bounds;
}

void ChartPanel::clearDataSeries() {
    chart_utils::removeAllSeries(chart_);
    curve_count_ = 0;
}

void ChartPanel::clearCurves() {
    if (view_)
        view_->setUpdatesEnabled(false);
    clearDataSeries();
    if (chart_)
        chart_->zoomReset();
    constexpr chart_utils::Pair def{-1.0, 1.0};
    chart_utils::updateAxes(chart_, def, def, chart_utils::GridMode::Tab, false, false);
    if (view_) {
        view_->setUpdatesEnabled(true);
        view_->viewport()->update();
    }
}

void ChartPanel::fitAxes(double minX, double maxX, double minY, double maxY, bool niceX, bool niceY) {
    // Range: nice → 1–2–5; data → exact (t, ω); padded → +2%.
    // Ticks: 1–2–5 inside the window. t/ω keep the data span (no expand to 0 extra).
    const auto rx = niceX ? chart_utils::niceAxisRange(minX, maxX, true) : chart_utils::dataAxisRange(minX, maxX, true);
    const auto ry =
        niceY ? chart_utils::niceAxisRange(minY, maxY, true) : chart_utils::paddedAxisRange(minY, maxY, true);
    chart_utils::updateAxes(chart_, rx, ry, chart_utils::GridMode::Tab, /*snap_x=*/niceX, /*snap_y=*/niceY);
}
