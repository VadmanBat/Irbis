#pragma once

#include "code/charts/utils/chart-utils-theme.hpp"
#include "code/charts/utils/nice-axis.hpp"
#include "code/series/axis-bounds.hpp"

#include <complex>
#include <QChart>
#include <QChartView>
#include <QPen>
#include <QString>
#include <utility>
#include <vector>

class QValueAxis;

/// Stateless helpers for QChart (shared by ChartPanel, InteractiveChartView, clone).
/// Stateful UI lives in ChartPanel / InteractiveChartView / ResponseChartBank — not here.
namespace chart_utils {
using Pair    = std::pair<double, double>;
using VecPair = std::vector<Pair>;
using VecComp = std::vector<std::complex<double>>;

inline constexpr auto kHorGuide  = "hor-line";
inline constexpr auto kVerGuide  = "ver-line";
inline constexpr int kMajorTicks = 5;

/// Result of adding/replacing a series (stack POD — no heap, no optional).
struct SeriesWrite {
    bool wrote{false};
    AxisBounds bounds{};
};

enum class GridMode {
    Tab,    ///< Fixed ticks; optional snap_x/y (tabs, no pan)
    Viewer, ///< Dynamic ticks + anchor 0 (detached viewer)
};

QPen penForIndex(std::size_t index);

void createAxes(QChart* chart, const QString& titleX, const QString& titleY);
void createChartContextMenu(QChartView* chartView);
void openChartViewer(QChart* chart, QWidget* parent = nullptr);
void removeAllSeries(QChart* chart);

/// Set range then style. Default: no lattice re-snap (fitAxes already applied nice/data ranges).
void updateAxes(QChart* chart, const Pair& range_x, const Pair& range_y, GridMode mode = GridMode::Tab,
                bool snap_x = false, bool snap_y = false);

void applyViewerGrid(QValueAxis* axis);
void applyViewerGrid(QValueAxis* axis, double lo, double hi);
void applyViewerGrid(QChart* chart);
void updateOriginGuides(QChart* chart, const Pair& range_x, const Pair& range_y);

/// One pass: QLineSeries points + AxisBounds. wrote=false if empty / no chart / no series to replace.
[[nodiscard]] SeriesWrite addRealSeries(QChart* chart, const VecPair& points, const QString& title,
                                        std::size_t index = 0);
[[nodiscard]] SeriesWrite addComplexSeries(QChart* chart, const VecComp& points, const QString& title,
                                           std::size_t index = 0);
[[nodiscard]] SeriesWrite replaceLastRealSeries(QChart* chart, const VecPair& points, const QString& title);
[[nodiscard]] SeriesWrite replaceLastComplexSeries(QChart* chart, const VecComp& points, const QString& title);

/// Remove last non-guide QLineSeries. Returns true if a series was deleted.
bool removeLastDataSeries(QChart* chart);

bool saveChartToFile(const QString& fileName, QChart* chart);

/// Save by path; format from suffix (.png / .svg / .txt). Returns false on failure.
bool saveChartExport(QChartView* view, const QString& path);

/// «Сохранить как…» dialog (PNG / SVG / TXT). Returns true if a file was written.
bool saveChartAsDialog(QWidget* parent, QChartView* view, const QString& suggestedName);

QChartView* makeChartView(QChart* chart, QWidget* parent, const QString& title, const QString& titleX,
                          const QString& titleY);

[[nodiscard]] QChart* cloneChart(QChart* src);
} // namespace chart_utils
