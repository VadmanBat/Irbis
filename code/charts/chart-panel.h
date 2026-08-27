#pragma once

#include "code/charts/utils/chart-utils.hpp"

#include <QWidget>

class QChart;
class QChartView;
class QEvent;
class QVBoxLayout;

/// Owns one QChart + QChartView for a tab panel (series, fit, clear).
/// Low-level QChart ops stay in chart_utils; this class is the UI boundary.
class ChartPanel : public QWidget {
    Q_OBJECT

private:
    QChart* chart_{nullptr};
    QChartView* view_{nullptr};
    QVBoxLayout* layout_{nullptr};
    std::size_t curve_count_{0};

    void apply_theme();

protected:
    void changeEvent(QEvent* event) override;

public:
    using Pair    = chart_utils::Pair;
    using VecPair = chart_utils::VecPair;
    using VecComp = chart_utils::VecComp;

    explicit ChartPanel(const QString& title, const QString& titleX, const QString& titleY, QWidget* parent = nullptr);

    [[nodiscard]] QChart* chart() const { return chart_; }
    [[nodiscard]] QChartView* view() const { return view_; }
    [[nodiscard]] std::size_t curveCount() const { return curve_count_; }

    void setChartTitle(const QString& title);

    /// One-pass series build + extents. Empty data → default bounds, no series, count unchanged.
    [[nodiscard]] AxisBounds addRealCurve(const VecPair& points, const QString& name);
    [[nodiscard]] AxisBounds addComplexCurve(const VecComp& points, const QString& name);
    /// Deadzone / envelope band; does not consume a series colour index.
    [[nodiscard]] AxisBounds addBand(const VecPair& lower, const VecPair& upper, const QString& name);
    /// Replace last data curve (or add if none); returns extents of written data.
    [[nodiscard]] AxisBounds replaceLastRealCurve(const VecPair& points, const QString& name);
    [[nodiscard]] AxisBounds replaceLastComplexCurve(const VecComp& points, const QString& name);

    void clearCurves();
    void clearDataSeries();

    /// niceX/niceY: true → nice range + 1–2–5 Dynamic grid; false X → exact (t, ω); false Y → pad ~2%.
    void fitAxes(double minX, double maxX, double minY, double maxY, bool niceX = true, bool niceY = true);
};
