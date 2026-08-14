#include "code/widgets/double-slider.h"

#include <algorithm>
#include <cmath>
#include <QPaintEvent>
#include <QSignalBlocker>

DoubleSlider::DoubleSlider(Qt::Orientation orientation, QWidget* parent) : QSlider(orientation, parent) {
    single_step_ = (max_ - min_) / static_cast<double>(intervals_);
    value_       = min_;
    QSlider::setRange(0, intervals_);
    connect(this, &QSlider::valueChanged, this, &DoubleSlider::on_int_value_changed);
}

void DoubleSlider::sync_handle_from_value() {
    if (max_ <= min_ || intervals_ <= 0) {
        const QSignalBlocker block(this);
        QSlider::setValue(0);
        return;
    }
    const double t = (value_ - min_) / (max_ - min_);
    const int tick = static_cast<int>(std::lround(std::clamp(t, 0.0, 1.0) * static_cast<double>(intervals_)));
    const QSignalBlocker block(this);
    QSlider::setValue(std::clamp(tick, 0, intervals_));
}

void DoubleSlider::setRange(double min, double max, int intervals) {
    const double old_value = value_;
    min_                   = min;
    max_                   = max;
    intervals_             = std::max(1, intervals);
    QSlider::setRange(0, intervals_);
    single_step_ = (max_ - min_) / static_cast<double>(intervals_);
    // Keep continuous value; only clamp if outside new range.
    if (std::isfinite(old_value)) {
        if (old_value < min_)
            value_ = min_;
        else if (old_value > max_)
            value_ = max_;
        else
            value_ = old_value;
    }
    else {
        value_ = min_;
    }
    const bool changed = value_ != old_value;
    sync_handle_from_value();
    if (changed)
        emit doubleValueChanged(value_);
}

void DoubleSlider::setValue(double value) {
    if (!std::isfinite(value))
        return;
    // Exact value for math / labels; handle is only a visual nearest tick.
    if (max_ > min_)
        value_ = std::clamp(value, min_, max_);
    else
        value_ = value;
    sync_handle_from_value();
}

void DoubleSlider::paintEvent(QPaintEvent* event) {
    // Value / limits shown in RegParameter row — keep the track clean.
    QSlider::paintEvent(event);
}

void DoubleSlider::on_int_value_changed(int) {
    // User moved the handle — apply discrete grid of the slider only here.
    value_ = min_ + static_cast<double>(QSlider::value()) * single_step_;
    emit doubleValueChanged(value_);
}
