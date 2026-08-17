#include "code/charts/response-chart-bank.h"

#include <utility>

ChartPanel* ResponseChartBank::panel_at(int idx) const noexcept {
    switch (idx) {
        case 0:
            return chart_tran_;
        case 1:
            return chart_impulse_;
        case 2:
            return chart_nyquist_;
        case 3:
            return chart_amp_;
        case 4:
            return chart_phase_;
        default:
            return chart_tran_;
    }
}

BoundsSet& ResponseChartBank::bounds_at(int idx) noexcept {
    switch (idx) {
        case 0:
            return tran_bounds_;
        case 1:
            return impulse_bounds_;
        case 2:
            return nyquist_bounds_;
        case 3:
            return amp_bounds_;
        case 4:
            return phase_bounds_;
        default:
            return tran_bounds_;
    }
}

void ResponseChartBank::ensure_channel(Batch& batch, Channel ch) {
    if (ch == Channel::Transient && batch.has_transient)
        return;
    if (ch == Channel::Impulse && batch.has_impulse)
        return;
    if ((ch == Channel::Nyquist || ch == Channel::Amplitude || ch == Channel::Phase) && batch.has_freq)
        return;

    switch (ch) {
        case Channel::Transient:
            batch.transient     = tf_builder::transient(batch.tf, batch.params, batch.delayTau);
            batch.has_transient = true;
            break;
        case Channel::Impulse:
            batch.impulse     = tf_builder::impulse(batch.tf, batch.params, batch.delayTau);
            batch.has_impulse = true;
            break;
        case Channel::Nyquist:
        case Channel::Amplitude:
        case Channel::Phase: {
            auto freq       = tf_builder::frequencyBundle(batch.tf, batch.params, batch.delayTau);
            batch.nyquist   = std::move(freq.nyquist);
            batch.amplitude = std::move(freq.amplitude);
            batch.phase     = std::move(freq.phase);
            batch.has_freq  = true;
            break;
        }
    }
}

void ResponseChartBank::ensure_visible_channels(Batch& batch) {
    if (vis_.transient)
        ensure_channel(batch, Channel::Transient);
    if (vis_.impulse)
        ensure_channel(batch, Channel::Impulse);
    if (vis_.nyquist || vis_.amplitude || vis_.phase)
        ensure_channel(batch, Channel::Nyquist);
}

void ResponseChartBank::refresh_last_quality() {
    if (history_.empty()) {
        has_last_quality_ = false;
        last_quality_     = {};
        return;
    }
    last_quality_     = tf_builder::quality(history_.back().tf, history_.back().params, history_.back().delayTau);
    has_last_quality_ = true;
}

ResponseChartBank::Batch ResponseChartBank::make_batch(const numina::TransferFunction& tf, const ModelParam& params,
                                                       const QString& name, double delayTau) {
    Batch b;
    b.name     = name;
    b.tf       = tf;
    b.delayTau = delayTau;
    b.params   = params;

    last_quality_     = tf_builder::quality(b.tf, params, delayTau);
    has_last_quality_ = true;

    if (vis_.transient) {
        b.transient     = tf_builder::transient(b.tf, params, delayTau);
        b.has_transient = true;
    }
    if (vis_.impulse) {
        b.impulse     = tf_builder::impulse(b.tf, params, delayTau);
        b.has_impulse = true;
    }
    if (vis_.nyquist || vis_.amplitude || vis_.phase) {
        auto freq   = tf_builder::frequencyBundle(b.tf, params, delayTau);
        b.nyquist   = std::move(freq.nyquist);
        b.amplitude = std::move(freq.amplitude);
        b.phase     = std::move(freq.phase);
        b.has_freq  = true;
    }
    return b;
}

void ResponseChartBank::rebuild_bounds_from_history() {
    for (int i = 0; i < kPanelCount; ++i)
        bounds_at(i).clear();

    for (Batch& batch : history_) {
        ensure_visible_channels(batch);
        tran_bounds_.push_back(batch.has_transient ? boundsOfReal(batch.transient) : AxisBounds{});
        impulse_bounds_.push_back(batch.has_impulse ? boundsOfReal(batch.impulse) : AxisBounds{});
        nyquist_bounds_.push_back(batch.has_freq ? boundsOfComplex(batch.nyquist) : AxisBounds{});
        amp_bounds_.push_back(batch.has_freq ? boundsOfReal(batch.amplitude) : AxisBounds{});
        phase_bounds_.push_back(batch.has_freq ? boundsOfReal(batch.phase) : AxisBounds{});
    }
}

bool ResponseChartBank::trim_history() {
    if (history_.size() <= kMaxHistory)
        return false;
    history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(history_.size() - kMaxHistory));
    rebuild_bounds_from_history();
    panel_gen_.fill(0);
    return true;
}

void ResponseChartBank::pop_all_bounds() {
    for (int i = 0; i < kPanelCount; ++i)
        bounds_at(i).pop_back();
}
