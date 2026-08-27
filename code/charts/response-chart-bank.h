#pragma once

#include "code/charts/chart-panel.h"
#include "code/model/model-param.hpp"
#include "code/series/axis-bounds.hpp"
#include "code/series/bounds-set.hpp"
#include "code/util/tf-builder.hpp"
#include "numina/classes/control/models/quality-report.h"
#include "numina/classes/control/models/transfer-function.h"

#include <array>
#include <cstdint>
#include <QWidget>
#include <vector>

class QGridLayout;
class QMenu;

struct ChartVisibility {
    bool transient = true;
    bool impulse   = false;
    bool nyquist   = true;
    bool amplitude = false;
    bool phase     = false;

    [[nodiscard]] int count() const {
        return static_cast<int>(transient) + static_cast<int>(impulse) + static_cast<int>(nyquist) +
               static_cast<int>(amplitude) + static_cast<int>(phase);
    }
};

/// Multi-plot host: h(t), w(t), Nyquist, АЧХ, ФЧХ + visibility menu.
class ResponseChartBank : public QWidget {
    Q_OBJECT

private:
    enum class Channel : int { Transient = 0, Impulse, Nyquist, Amplitude, Phase };

    struct Batch {
        QString name;
        numina::TransferFunction tf;
        double delayTau{0.0}; ///< exact / Padé delay applied at sample time (0 if already in tf)
        ModelParam params;

        tf_builder::VecPair transient;
        tf_builder::VecPair impulse;
        tf_builder::VecComp nyquist;
        tf_builder::VecPair amplitude;
        tf_builder::VecPair phase;

        bool has_transient{false};
        bool has_impulse{false};
        bool has_freq{false}; ///< nyquist + amplitude + phase filled together

        void clear_channels() {
            transient.clear();
            impulse.clear();
            nyquist.clear();
            amplitude.clear();
            phase.clear();
            has_transient = has_impulse = has_freq = false;
        }
    };

    static constexpr std::size_t kMaxHistory = 24;
    static constexpr int kPanelCount         = 5;

    ChartVisibility vis_{};
    QGridLayout* grid_{nullptr};

    ChartPanel* chart_tran_{nullptr};
    ChartPanel* chart_impulse_{nullptr};
    ChartPanel* chart_nyquist_{nullptr};
    ChartPanel* chart_amp_{nullptr};
    ChartPanel* chart_phase_{nullptr};

    BoundsSet tran_bounds_;
    BoundsSet impulse_bounds_;
    BoundsSet nyquist_bounds_;
    BoundsSet amp_bounds_;
    BoundsSet phase_bounds_;

    std::vector<Batch> history_;
    std::uint64_t data_gen_{0};
    std::array<std::uint64_t, kPanelCount> panel_gen_{};

    numina::QualityReport last_quality_{};
    bool has_last_quality_{false};

    bool has_tran_overlay_{false};
    QString tran_overlay_name_;
    chart_utils::VecPair tran_overlay_points_;
    AxisBounds tran_overlay_bounds_{};

    void rebuild_layout();
    void refit_all();
    void fit_transient();
    void bump_data() { ++data_gen_; }

    void ensure_channel(Batch& batch, Channel ch);
    void ensure_visible_channels(Batch& batch);
    void ensure_visible_series();
    void rebuild_bounds_from_history();
    bool trim_history();
    void rematerialize_panel(int panel_idx);
    void push_batch(Batch b, bool replace_last);
    Batch make_batch(const numina::TransferFunction& tf, const ModelParam& params, const QString& name,
                     double delayTau = 0.0);
    void refresh_last_quality();

    [[nodiscard]] ChartPanel* panel_at(int idx) const noexcept;
    [[nodiscard]] BoundsSet& bounds_at(int idx) noexcept;
    void pop_all_bounds();
    void set_panel_updates_all(bool on);

public:
    explicit ResponseChartBank(QWidget* parent = nullptr);

    void setTransientTitle(const QString& title);

    [[nodiscard]] ChartVisibility visibility() const { return vis_; }
    void setVisibility(ChartVisibility vis);

    void populateMenu(QMenu* menu);

    void appendFromTf(const numina::TransferFunction& tf, const ModelParam& params, const QString& name,
                      double delayTau = 0.0);
    void replaceLastFromTf(const numina::TransferFunction& tf, const ModelParam& params, const QString& name,
                           double delayTau = 0.0);
    void recomputeAll(const ModelParam& params);
    void appendTransientCurve(const chart_utils::VecPair& points, const QString& name);
    void clearAll();

    [[nodiscard]] bool empty() const { return history_.empty(); }
    [[nodiscard]] std::size_t seriesCount() const noexcept { return history_.size(); }
    [[nodiscard]] bool hasLastQuality() const noexcept { return has_last_quality_; }
    [[nodiscard]] const numina::QualityReport& lastQuality() const noexcept { return last_quality_; }
};
