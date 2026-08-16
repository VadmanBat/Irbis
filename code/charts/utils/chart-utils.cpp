#include "code/charts/utils/chart-utils.hpp"

#include "code/charts/utils/chart-utils-detail.hpp"

#include <algorithm>
#include <QAbstractAxis>
#include <QAbstractSeries>
#include <QBrush>
#include <QFontMetrics>
#include <QGraphicsLayout>
#include <QLegend>
#include <QLineSeries>
#include <QMargins>
#include <QPainter>
#include <QScatterSeries>
#include <QValueAxis>

namespace chart_utils {
void applyAxisTheme(QValueAxis* axis, const ChartTheme& theme) {
    if (!axis)
        return;
    if (axis->labelsColor() != theme.text)
        axis->setLabelsColor(theme.text);
    const QBrush title(theme.text);
    if (axis->titleBrush() != title)
        axis->setTitleBrush(title);
    const QPen line(theme.axis_line, 1.0);
    if (axis->linePen() != line)
        axis->setLinePen(line);
    if (axis->gridLineColor() != theme.grid)
        axis->setGridLineColor(theme.grid);
    if (axis->minorGridLineColor() != theme.minor_grid)
        axis->setMinorGridLineColor(theme.minor_grid);
}

void applyChartTheme(QChart* chart, QChartView* view) {
    if (!chart)
        return;

    const ChartTheme theme = currentTheme();

    chart->setBackgroundVisible(true);
    chart->setBackgroundBrush(QBrush(theme.background));
    chart->setPlotAreaBackgroundVisible(true);
    chart->setPlotAreaBackgroundBrush(QBrush(theme.plot));
    chart->setTitleBrush(QBrush(theme.text));

    if (QLegend* legend = chart->legend()) {
        legend->setLabelColor(theme.text);
        legend->setBrush(QBrush(theme.legend_bg));
        legend->setBorderColor(theme.grid);
        legend->setColor(theme.text);
    }

    for (QAbstractAxis* ax : chart->axes()) {
        if (auto* v = qobject_cast<QValueAxis*>(ax))
            applyAxisTheme(v, theme);
    }

    // Guides follow theme. Data series keep pens (Properties / palette at add time).
    for (QAbstractSeries* s : chart->series()) {
        auto* line = qobject_cast<QLineSeries*>(s);
        if (!line)
            continue;
        if (detail::isGuideSeries(line->name()))
            line->setPen(guidePen(theme));
    }

    if (view)
        view->setBackgroundBrush(QBrush(theme.background));
}

QPen penForIndex(std::size_t index) {
    return penForIndexTheme(index, isDarkTheme());
}

void tightenChartFrame(QChart* chart) {
    if (!chart)
        return;
    if (QGraphicsLayout* lay = chart->layout())
        lay->setContentsMargins(0, 0, 0, 0);

    // Left/bottom already reserve a title band. The right side has no axis, so
    // match that band — otherwise the plot sits on the widget edge.
    int title_band = 0;
    for (QAbstractAxis* ax : chart->axes()) {
        if (ax->titleText().isEmpty() || !ax->isTitleVisible())
            continue;
        title_band = std::max(title_band, QFontMetrics(ax->titleFont()).height());
    }
    if (title_band == 0)
        title_band = QFontMetrics(chart->font()).height();

    constexpr int k = 2;
    chart->setMargins(QMargins(k, k, k + title_band, k));
}

QChartView* makeChartView(QChart* chart, QWidget* parent, const QString& title, const QString& titleX,
                          const QString& titleY) {
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->setVisible(true);
    createAxes(chart, titleX, titleY);

    updateAxes(chart, {-1.0, 1.0}, {-1.0, 1.0}, GridMode::Tab, /*snap_x=*/false, /*snap_y=*/false);

    auto* view = new QChartView(chart, parent);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    tightenChartFrame(chart);
    applyChartTheme(chart, view);
    createChartContextMenu(view);
    return view;
}

QChart* cloneChart(QChart* src) {
    if (!src)
        return new QChart;

    auto* dst = new QChart;
    dst->setTitle(src->title());
    dst->setAnimationOptions(QChart::NoAnimation);
    if (src->legend())
        dst->legend()->setVisible(src->legend()->isVisible());

    QString title_x;
    QString title_y;
    Pair range_x{-1.0, 1.0};
    Pair range_y{-1.0, 1.0};
    if (auto* ax = qobject_cast<QValueAxis*>(src->axes(Qt::Horizontal).value(0, nullptr))) {
        title_x        = ax->titleText();
        range_x.first  = ax->min();
        range_x.second = ax->max();
    }
    if (auto* ay = qobject_cast<QValueAxis*>(src->axes(Qt::Vertical).value(0, nullptr))) {
        title_y        = ay->titleText();
        range_y.first  = ay->min();
        range_y.second = ay->max();
    }

    createAxes(dst, title_x, title_y);

    for (auto* s : src->series()) {
        if (detail::isGuideSeries(s->name()))
            continue;
        QAbstractSeries* copy = nullptr;
        if (auto* line = qobject_cast<QLineSeries*>(s)) {
            auto* out = new QLineSeries;
            out->setName(line->name());
            out->setPen(line->pen());
            out->setPointsVisible(line->pointsVisible());
            out->replace(line->points());
            copy = out;
        }
        else if (auto* scatter = qobject_cast<QScatterSeries*>(s)) {
            auto* out = new QScatterSeries;
            out->setName(scatter->name());
            out->setMarkerShape(scatter->markerShape());
            out->setMarkerSize(scatter->markerSize());
            out->setColor(scatter->color());
            out->setBorderColor(scatter->borderColor());
            out->setPen(scatter->pen());
            out->setBrush(scatter->brush());
            out->setLightMarker(scatter->lightMarker());
            out->replace(scatter->points());
            copy = out;
        }
        if (!copy)
            continue;
        dst->addSeries(copy);
        detail::attachToAxes(dst, copy);
    }

    // Preserve source extents; viewer switches to GridMode::Viewer after open.
    updateAxes(dst, range_x, range_y, GridMode::Tab, /*snap_x=*/false, /*snap_y=*/false);
    tightenChartFrame(dst);
    applyChartTheme(dst, nullptr);
    return dst;
}
} // namespace chart_utils
