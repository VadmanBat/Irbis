#pragma once

#include "code/charts/utils/chart-utils-theme.hpp"
#include "code/charts/utils/nice-axis.hpp"
#include "code/series/axis-bounds.hpp"

#include <complex>
#include <QAbstractSeries>
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
    Tab,    ///< Dynamic 1–2–5 ticks inside the window (t, ω keep data span)
    Viewer, ///< Dynamic 1–2–5; park anchor if the window is far from 0
};

QPen penForIndex(std::size_t index);
/// Origin guides and QAreaSeries boundary lines (not user data).
[[nodiscard]] bool isAccessorySeries(QChart* chart, QAbstractSeries* series);

void createAxes(QChart* chart, const QString& titleX, const QString& titleY);
void createChartContextMenu(QChartView* chartView);
void openChartViewer(QChart* chart, QWidget* parent = nullptr);
void removeAllSeries(QChart* chart);

/// Interval, then range (never the reverse — leftover tiny step freezes Qt).
void updateAxes(QChart* chart, const Pair& range_x, const Pair& range_y, GridMode mode = GridMode::Tab,
                bool snap_x = false, bool snap_y = false);

void applyViewerGrid(QValueAxis* axis);
void applyViewerGrid(QValueAxis* axis, double lo, double hi);
void applyViewerGrid(QChart* chart);
void updateOriginGuides(QChart* chart, const Pair& range_x, const Pair& range_y);
/// Drop default QChart padding; plot rect stays with QChart (titles / labels / legend).
void tightenChartFrame(QChart* chart);

/// One pass: QLineSeries points + AxisBounds. wrote=false if empty / no chart / no series to replace.
[[nodiscard]] SeriesWrite addRealSeries(QChart* chart, const VecPair& points, const QString& title,
                                        std::size_t index = 0);
/// Filled band between two polylines (deadzone tube). Not a data-curve colour slot.
[[nodiscard]] SeriesWrite addBandSeries(QChart* chart, const VecPair& lower, const VecPair& upper,
                                        const QString& title);
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
