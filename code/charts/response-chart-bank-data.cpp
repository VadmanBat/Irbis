#include "code/charts/response-chart-bank.h"

void ResponseChartBank::push_batch(Batch b, bool replace_last) {
    if (replace_last && !history_.empty()) {
        history_.pop_back();
        pop_all_bounds();
    }

    bump_data();
    set_panel_updates_all(false);

    // Overlay is always first data series on transient; replace last skips it.
    auto hist_count = [this](ChartPanel* panel, bool is_tran) -> std::size_t {
        const std::size_t n = panel->curveCount();
        return (is_tran && has_tran_overlay_ && n > 0) ? n - 1 : n;
    };

    auto push_real = [&](bool visible, ChartPanel* panel, BoundsSet& bounds, const tf_builder::VecPair& data,
                         bool has_data, bool replace, bool is_tran) {
        if (visible && has_data) {
            if (replace && hist_count(panel, is_tran) > 0)
                bounds.push_back(panel->replaceLastRealCurve(data, b.name));
            else
                bounds.push_back(panel->addRealCurve(data, b.name));
        }
        else {
            bounds.push_back(has_data ? boundsOfReal(data) : AxisBounds{});
        }
    };
    auto push_cx = [&](bool visible, ChartPanel* panel, BoundsSet& bounds, const tf_builder::VecComp& data,
                       bool has_data, bool replace) {
        if (visible && has_data) {
            if (replace && panel->curveCount() > 0)
                bounds.push_back(panel->replaceLastComplexCurve(data, b.name));
            else
                bounds.push_back(panel->addComplexCurve(data, b.name));
        }
        else {
            bounds.push_back(has_data ? boundsOfComplex(data) : AxisBounds{});
        }
    };

    push_real(vis_.transient, chart_tran_, tran_bounds_, b.transient, b.has_transient, replace_last, true);
    push_real(vis_.impulse, chart_impulse_, impulse_bounds_, b.impulse, b.has_impulse, replace_last, false);
    push_cx(vis_.nyquist, chart_nyquist_, nyquist_bounds_, b.nyquist, b.has_freq, replace_last);
    push_real(vis_.amplitude, chart_amp_, amp_bounds_, b.amplitude, b.has_freq, replace_last, false);
    push_real(vis_.phase, chart_phase_, phase_bounds_, b.phase, b.has_freq, replace_last, false);

    history_.push_back(std::move(b));
    const bool trimmed = trim_history();

    if (trimmed) {
        ensure_visible_series();
    }
    else {
        const bool show[] = {vis_.transient, vis_.impulse, vis_.nyquist, vis_.amplitude, vis_.phase};
        for (int i = 0; i < kPanelCount; ++i) {
            if (show[i])
                panel_gen_[static_cast<std::size_t>(i)] = data_gen_;
        }
    }
    refit_all();
    set_panel_updates_all(true);
}

void ResponseChartBank::appendFromTf(const numina::TransferFunction& tf, const ModelParam& params, const QString& name,
                                     double delayTau) {
    push_batch(make_batch(tf, params, name, delayTau), false);
}

void ResponseChartBank::replaceLastFromTf(const numina::TransferFunction& tf, const ModelParam& params,
                                          const QString& name, double delayTau) {
    if (history_.empty()) {
        appendFromTf(tf, params, name, delayTau);
        return;
    }
    push_batch(make_batch(tf, params, name, delayTau), true);
}

void ResponseChartBank::appendTransientCurve(const chart_utils::VecPair& points, const QString& name) {
    if (points.empty())
        return;

    has_tran_overlay_    = true;
    tran_overlay_name_   = name;
    tran_overlay_points_ = points;
    tran_overlay_bounds_ = boundsOfReal(points);
    bump_data();
    panel_gen_[0] = 0;

    if (vis_.transient)
        rematerialize_panel(0);
    fit_transient();
}

void ResponseChartBank::recomputeAll(const ModelParam& params) {
    if (history_.empty())
        return;

    for (Batch& batch : history_) {
        // Delay mode is per series (how it was added). Grid/sample settings are shared.
        const bool use_pade        = batch.params.usePadeApprox;
        const int order            = batch.params.approxOrder;
        batch.params               = params;
        batch.params.usePadeApprox = use_pade;
        batch.params.approxOrder   = order;
        batch.clear_channels();
        ensure_visible_channels(batch);
    }
    rebuild_bounds_from_history();
    bump_data();
    panel_gen_.fill(0);
    ensure_visible_series();
    refresh_last_quality();
    refit_all();
}

void ResponseChartBank::clearAll() {
    history_.clear();
    has_tran_overlay_ = false;
    tran_overlay_name_.clear();
    tran_overlay_points_.clear();
    tran_overlay_bounds_ = {};
    has_last_quality_    = false;
    last_quality_        = {};
    data_gen_            = 0;
    panel_gen_.fill(0);

    for (int i = 0; i < kPanelCount; ++i) {
        panel_at(i)->clearCurves();
        bounds_at(i).clear();
    }
}
