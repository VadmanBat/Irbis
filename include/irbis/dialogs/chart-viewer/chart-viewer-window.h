#pragma once

#include "irbis/charts/interactive-chart-view.h"
#include "irbis/charts/utils/chart-utils.hpp"

#include <QMainWindow>

class QAction;
class QChart;
class QLabel;

/// Detached chart viewer: clone of a panel chart + navigation tools.
class ChartViewerWindow : public QMainWindow {
    Q_OBJECT

private:
    chart_viewer::InteractiveChartView* view_{nullptr};
    QChart* chart_{nullptr};
    chart_utils::Pair home_x_{-1.0, 1.0};
    chart_utils::Pair home_y_{-1.0, 1.0};

    QAction* act_zoom_{nullptr};
    QAction* act_pan_{nullptr};
    QAction* act_zoom_in_{nullptr};
    QAction* act_zoom_out_{nullptr};
    QAction* act_home_{nullptr};
    QAction* act_grid_{nullptr};
    QAction* act_legend_{nullptr};
    QAction* act_save_{nullptr};
    QAction* act_copy_{nullptr};
    QAction* act_props_{nullptr};
    QAction* act_fullscreen_{nullptr};
    QAction* act_close_{nullptr};
    QLabel* coord_label_{nullptr};

    void build_toolbar();
    void build_status();
    void setup_shortcuts();
    void save_as();
    void copy_image();
    void open_properties();
    void toggle_fullscreen(bool on);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    explicit ChartViewerWindow(QChart* source_chart, QWidget* parent = nullptr);
    ~ChartViewerWindow() override = default;

    static void open(QChart* source_chart, QWidget* parent = nullptr);
};
