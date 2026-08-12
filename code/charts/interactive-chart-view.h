#pragma once

#include "code/charts/utils/chart-utils.hpp"

#include <QChartView>
#include <QPoint>

class QEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QValueAxis;

namespace chart_viewer {
/// Detached / fullscreen chart interaction: pan, zoom, world grid (anchor 0).
/// Owns no chart data — operates on the QChart given to QChartView.
class InteractiveChartView : public QChartView {
    Q_OBJECT

public:
    enum class Tool {
        ZoomRect,
        Pan,
    };

private:
    Tool tool_{Tool::ZoomRect};
    bool panning_{false};
    QPoint last_pos_{};
    bool grid_on_{true};

    void apply_tool_cursor();
    void sync_axes_after_view_change();
    void pan_by_pixels(int dx_px, int dy_px);
    void apply_theme();
    [[nodiscard]] QValueAxis* axis_x() const;
    [[nodiscard]] QValueAxis* axis_y() const;

protected:
    void changeEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

public:
    explicit InteractiveChartView(QChart* chart, QWidget* parent = nullptr);

    void setTool(Tool tool);
    [[nodiscard]] Tool tool() const noexcept { return tool_; }

    void zoomInStep();
    void zoomOutStep();
    void resetView(const chart_utils::Pair& home_x, const chart_utils::Pair& home_y);
    void setGridVisible(bool on);
    [[nodiscard]] bool isGridVisible() const noexcept { return grid_on_; }

signals:
    void cursorMoved(double x, double y, bool inside);
    void viewChanged();
};
} // namespace chart_viewer
