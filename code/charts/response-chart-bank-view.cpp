#include "code/charts/response-chart-bank.h"

#include <algorithm>

namespace {
void set_panel_updates(ChartPanel* panel, bool on) {
    if (!panel || !panel->view())
        return;
    panel->view()->setUpdatesEnabled(on);
    if (on)
        panel->view()->viewport()->update();
}

void fit_panel(ChartPanel* panel, const BoundsSet& b, bool nice_x, bool nice_y) {
    if (!panel || b.empty())
        return;
    panel->fitAxes(b.min_x(), b.max_x(), b.min_y(), b.max_y(), nice_x, nice_y);
}
} // namespace

void ResponseChartBank::set_panel_updates_all(bool on) {
    const bool show[] = {vis_.transient, vis_.impulse, vis_.nyquist, vis_.amplitude, vis_.phase};
    for (int i = 0; i < kPanelCount; ++i) {
        if (show[i])
            set_panel_updates(panel_at(i), on);
    }
}

void ResponseChartBank::fit_transient() {
    if (!vis_.transient)
        return;

    const bool has_hist = !tran_bounds_.empty();
    if (!has_hist && !has_tran_overlay_)
        return;

    double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    if (has_hist) {
        min_x = tran_bounds_.min_x();
        max_x = tran_bounds_.max_x();
        min_y = tran_bounds_.min_y();
        max_y = tran_bounds_.max_y();
        if (has_tran_overlay_ && tran_overlay_bounds_.valid) {
            min_x = std::min(min_x, tran_overlay_bounds_.min_x);
            max_x = std::max(max_x, tran_overlay_bounds_.max_x);
            min_y = std::min(min_y, tran_overlay_bounds_.min_y);
            max_y = std::max(max_y, tran_overlay_bounds_.max_y);
        }
    }
    else if (tran_overlay_bounds_.valid) {
        min_x = tran_overlay_bounds_.min_x;
        max_x = tran_overlay_bounds_.max_x;
        min_y = tran_overlay_bounds_.min_y;
        max_y = tran_overlay_bounds_.max_y;
    }
    else {
        return;
    }
    chart_tran_->fitAxes(min_x, max_x, min_y, max_y, false, true);
}

void ResponseChartBank::refit_all() {
    fit_transient();
    if (vis_.impulse)
        fit_panel(chart_impulse_, impulse_bounds_, false, true);
    if (vis_.nyquist)
        fit_panel(chart_nyquist_, nyquist_bounds_, true, true);
    if (vis_.amplitude)
        fit_panel(chart_amp_, amp_bounds_, false, true);
    if (vis_.phase)
        fit_panel(chart_phase_, phase_bounds_, false, true);
}

void ResponseChartBank::rematerialize_panel(int panel_idx) {
    ChartPanel* panel = panel_at(panel_idx);
    const Channel ch  = static_cast<Channel>(panel_idx);

    for (Batch& batch : history_)
        ensure_channel(batch, ch);

    set_panel_updates(panel, false);
    panel->clearDataSeries();

    if (panel_idx == 0 && has_tran_overlay_)
        (void)panel->addRealCurve(tran_overlay_points_, tran_overlay_name_);

    for (const Batch& batch : history_) {
        switch (ch) {
            case Channel::Transient:
                if (batch.has_transient)
                    (void)panel->addRealCurve(batch.transient, batch.name);
                break;
            case Channel::Impulse:
                if (batch.has_impulse)
                    (void)panel->addRealCurve(batch.impulse, batch.name);
                break;
            case Channel::Nyquist:
                if (batch.has_freq)
                    (void)panel->addComplexCurve(batch.nyquist, batch.name);
                break;
            case Channel::Amplitude:
                if (batch.has_freq)
                    (void)panel->addRealCurve(batch.amplitude, batch.name);
                break;
            case Channel::Phase:
                if (batch.has_freq)
                    (void)panel->addRealCurve(batch.phase, batch.name);
                break;
        }
    }

    set_panel_updates(panel, true);
    panel_gen_[static_cast<std::size_t>(panel_idx)] = data_gen_;
}

void ResponseChartBank::ensure_visible_series() {
    const bool show[] = {vis_.transient, vis_.impulse, vis_.nyquist, vis_.amplitude, vis_.phase};
    for (int i = 0; i < kPanelCount; ++i) {
        if (show[i] && panel_gen_[static_cast<std::size_t>(i)] != data_gen_)
            rematerialize_panel(i);
    }
}
