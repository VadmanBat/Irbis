#pragma once

#include "code/charts/utils/chart-utils.hpp"

#include <cmath>
#include <limits>
#include <QPoint>
#include <QWidget>
#include <vector>

class QChart;
class QChartView;
class QEvent;
class QGraphicsSimpleTextItem;
class QLineSeries;
class QResizeEvent;
class QScatterSeries;
class QVBoxLayout;

/// Interactive C₁–C₀ plane (РКЧХ): X = C₁, Y = C₀; click/drag free selection.
class C0C1Chart : public QWidget {
    Q_OBJECT

public:
    /// One sample of the positive C₀,C₁ band (logical coeffs, not plot order).
    struct Sample {
        double omega{};
        double c0{};
        double c1{};
        double c2{};
        double kp{};
        double ti{};
        double td{};
    };

    /// Optimal point for one quality criterion (ЛИК / ИКК / СКО).
    struct Optimum {
        bool valid{false};
        double c0{};
        double c1{};
        double kp{};
        double ti{};
        double omega{};
        QString label;
    };

private:
    QChart* chart_{nullptr};
    QChartView* view_{nullptr};
    QVBoxLayout* layout_{nullptr};
    QLineSeries* locus_series_{nullptr};
    QScatterSeries* opt_lik_{nullptr};
    QScatterSeries* opt_ikk_{nullptr};
    QScatterSeries* opt_sko_{nullptr};
    QScatterSeries* selection_series_{nullptr};
    std::vector<QScatterSeries*> pin_series_{};
    QGraphicsSimpleTextItem* tag_c1_{nullptr};
    QGraphicsSimpleTextItem* tag_c0_{nullptr};

    std::vector<Sample> locus_{};
    bool has_selection_{false};
    double sel_c0_{};
    double sel_c1_{};
    std::size_t live_index_{0};

    bool dragging_{false};
    bool refit_pending_{false};
    QPoint last_pixel_{-1, -1};
    double last_emit_c0_{std::numeric_limits<double>::quiet_NaN()};
    double last_emit_c1_{std::numeric_limits<double>::quiet_NaN()};

    void build_chart();
    void apply_theme();
    void apply_axis_titles();
    void place_axis_tags();
    void tighten_plot();
    void refit_axes();
    void schedule_refit();
    void update_selection_marker();
    void ensure_selection_visible();
    void style_live_marker();
    void style_pin_marker(QScatterSeries* series, std::size_t color_index);
    void clear_pins();
    void restyle_pins();
    /// Plot point: X = C₁, Y = C₀.
    [[nodiscard]] static QPointF to_plot(double c0, double c1) noexcept { return {c1, c0}; }
    [[nodiscard]] static bool nearly_same(double a, double b, double eps) noexcept {
        if (!std::isfinite(a) || !std::isfinite(b))
            return false;
        return std::abs(a - b) <= eps;
    }
    [[nodiscard]] bool value_at_pixel(const QPoint& viewport_pos, double& c0, double& c1) const;
    void handle_pointer(const QPoint& viewport_pos, bool force_emit);
    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

public:
    explicit C0C1Chart(QWidget* parent = nullptr);

    void clear();
    void setSquareSide(int side);
    void setLocus(std::vector<Sample> samples, const QString& name = {});
    void setOptima(const Optimum& lik, const Optimum& ikk, const Optimum& sko);
    /// Logical coeffs (C₀, C₁); drawn as (C₁, C₀) on the chart.
    void setSelection(double c0, double c1);
    void clearSelection();
    void requestRefit();
    /// Freeze the live point (same color as the last closed-loop series) and start the next color.
    void pinSelection(const QString& name = {});
    void setLiveIndex(std::size_t index);
    void trimPins(std::size_t max_count);

    [[nodiscard]] bool hasLocus() const noexcept { return !locus_.empty(); }
    [[nodiscard]] bool hasSelection() const noexcept { return has_selection_; }
    [[nodiscard]] const std::vector<Sample>& locus() const noexcept { return locus_; }

signals:
    /// Pointer pick/drag: emitted only when (C₀, C₁) actually changed.
    void samplePicked(const Sample& sample);
};
