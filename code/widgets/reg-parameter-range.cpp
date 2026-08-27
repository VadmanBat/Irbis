#include "code/dialogs/slider-settings-dialog.h"
#include "code/widgets/reg-parameter.h"

#include <algorithm>
#include <cmath>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSignalBlocker>

void RegParameter::update_slider_range() {
    if (!(min_ < max_))
        return;
    const auto value = slider_->value();
    const QSignalBlocker block_slider(slider_);
    const QSignalBlocker block_spin(value_spin_);
    slider_->setRange(min_, max_, intervals_);
    slider_->setValue(value);
    value_spin_->setRange(min_, max_);
    value_spin_->setValue(slider_->value());
}

void RegParameter::open_settings() {
    QWidget* host = slider_ ? slider_->window() : nullptr;
    SliderSettingsDialog dialog(label_->text(), hard_min_, hard_max_, {min_, max_, intervals_, slider_->value()},
                                {factory_min_, factory_max_, factory_intervals_}, host);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const SliderSettings s = dialog.data();
    min_                   = s.min;
    max_                   = s.max;
    intervals_             = s.intervals;
    const double old       = slider_->value();
    update_slider_range();
    if (slider_->value() != old)
        emit valueChanged(slider_->value());
}

void RegParameter::on_slider_moved(double v) {
    {
        const QSignalBlocker block(value_spin_);
        value_spin_->setValue(v);
    }
    emit valueChanged(v);
}

void RegParameter::on_spin_changed(double v) {
    {
        const QSignalBlocker block(slider_);
        slider_->setValue(v);
    }
    const double applied = slider_->value();
    if (value_spin_->value() != applied) {
        const QSignalBlocker block(value_spin_);
        value_spin_->setValue(applied);
    }
    emit valueChanged(applied);
}

void RegParameter::setRange(double min, double max) {
    if (max < min)
        std::swap(min, max);
    if (!(max > min))
        max = min + 1.0;
    min_ = min;
    max_ = max;
    update_slider_range();
}

void RegParameter::ensureValueInRange(double v) {
    if (!std::isfinite(v))
        return;
    double lo = min_;
    double hi = max_;
    if (!(hi > lo)) {
        setRange(std::min(v, lo), std::max(v, hi) + 1.0);
        return;
    }
    if (v >= lo && v <= hi)
        return;
    if (v < lo)
        lo = v - 0.05 * std::max(1.0, hi - v);
    if (v > hi)
        hi = v + 0.05 * std::max(1.0, v - lo);
    if (!(hi > lo))
        hi = lo + 1.0;
    setRange(lo, hi);
}

void RegParameter::setLimits(double hard_min, double hard_max) {
    if (hard_max < hard_min)
        std::swap(hard_min, hard_max);
    hard_min_ = hard_min;
    hard_max_ = hard_max;
    min_      = std::clamp(min_, hard_min_, hard_max_);
    max_      = std::clamp(max_, hard_min_, hard_max_);
    if (!(max_ > min_))
        max_ = std::min(hard_max_, min_ + 1.0);
    update_slider_range();
}
