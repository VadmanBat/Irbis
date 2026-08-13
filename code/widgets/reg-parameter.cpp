#include "code/widgets/reg-parameter.h"

#include "code/util/format.hxx"

#include <algorithm>
#include <cmath>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>

namespace {
constexpr int kLabelW = 40;
constexpr int kCheckW = 22;
constexpr int kValueW = 72;
constexpr int kSpinW  = 100;
constexpr int kSpinH  = 28;
} // namespace

RegParameter::RegParameter(const QString& title, double min, double max, double minValue, double maxValue,
                           QObject* parent)
    : QObject(parent),
      check_box_(new QCheckBox),
      label_(new QLabel(title)),
      value_label_(new QLabel),
      min_spin_(new QDoubleSpinBox),
      max_spin_(new QDoubleSpinBox),
      slider_(new DoubleSlider(Qt::Horizontal)),
      layout_(new QHBoxLayout) {
    check_box_->setObjectName(QStringLiteral("regParamCheck"));
    label_->setObjectName(QStringLiteral("regParamLabel"));
    value_label_->setObjectName(QStringLiteral("regParamValue"));
    min_spin_->setObjectName(QStringLiteral("regParamMin"));
    max_spin_->setObjectName(QStringLiteral("regParamMax"));
    slider_->setObjectName(QStringLiteral("regParamSlider"));

    // Order: checkbox | symbol | current | from | to | slider
    check_box_->setFixedWidth(kCheckW);
    check_box_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    label_->setFixedWidth(kLabelW);
    label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    label_->setTextFormat(Qt::RichText);

    value_label_->setFixedWidth(kValueW);
    value_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value_label_->setMinimumHeight(kSpinH);

    min_spin_->setRange(min, max);
    min_spin_->setDecimals(2);
    min_spin_->setValue(minValue);
    min_spin_->setPrefix(tr("от: "));
    min_spin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    min_spin_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    max_spin_->setRange(min, max);
    max_spin_->setDecimals(2);
    max_spin_->setValue(maxValue);
    max_spin_->setPrefix(tr("до: "));
    max_spin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    max_spin_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    update_slider_range();
    enable(false);

    connect(min_spin_, &QDoubleSpinBox::editingFinished, this, [this] {
        if (min_spin_->value() > max_spin_->value())
            max_spin_->setValue(min_spin_->value());
        update_slider_range();
    });
    connect(max_spin_, &QDoubleSpinBox::editingFinished, this, [this] {
        if (min_spin_->value() > max_spin_->value())
            min_spin_->setValue(max_spin_->value());
        update_slider_range();
    });
    connect(check_box_, &QCheckBox::toggled, this, &RegParameter::enable);
    connect(slider_, &DoubleSlider::doubleValueChanged, this, &RegParameter::on_slider_moved);

    auto* row = qobject_cast<QHBoxLayout*>(layout_);
    row->setContentsMargins(0, 4, 0, 4);
    row->setSpacing(8);
    row->setAlignment(Qt::AlignVCenter);

    row->addWidget(check_box_, 0, Qt::AlignVCenter);
    row->addWidget(label_, 0, Qt::AlignVCenter);
    row->addWidget(value_label_, 0, Qt::AlignVCenter);
    row->addWidget(min_spin_, 0, Qt::AlignVCenter);
    row->addWidget(max_spin_, 0, Qt::AlignVCenter);
    row->addWidget(slider_, 1);

    apply_default_style();
    refresh_value_label();
}

void RegParameter::update_slider_range() {
    const auto min = min_spin_->value();
    const auto max = max_spin_->value();
    if (!(min < max))
        return;
    const auto value = slider_->value();
    slider_->setRange(min, max, kSliderIntervals);
    slider_->setValue(value);
    refresh_value_label();
}

void RegParameter::enable(bool checked) {
    label_->setEnabled(checked);
    value_label_->setEnabled(checked);
    min_spin_->setEnabled(checked);
    max_spin_->setEnabled(checked);
    slider_->setEnabled(checked);
}

void RegParameter::on_slider_moved(double) {
    refresh_value_label();
}

void RegParameter::refresh_value_label() {
    value_label_->setText(num_format::format(slider_->value(), num_format::SIGNIFICANT_DIGITS));
}

void RegParameter::apply_default_style() {
    min_spin_->setFixedSize(kSpinW, kSpinH);
    max_spin_->setFixedSize(kSpinW, kSpinH);
}

bool RegParameter::enabled() const {
    return check_box_->isChecked();
}

double RegParameter::value() const {
    return slider_->value();
}

double RegParameter::rangeMin() const {
    return min_spin_->value();
}

double RegParameter::rangeMax() const {
    return max_spin_->value();
}

void RegParameter::setEnabled(bool on) {
    check_box_->setChecked(on);
}

void RegParameter::setValue(double v) {
    slider_->setValue(v);
    refresh_value_label();
}

void RegParameter::setRange(double min, double max) {
    if (max < min)
        std::swap(min, max);
    if (!(max > min))
        max = min + 1.0;
    min_spin_->setValue(min);
    max_spin_->setValue(max);
    update_slider_range();
}

void RegParameter::ensureValueInRange(double v) {
    if (!std::isfinite(v))
        return;
    double lo = min_spin_->value();
    double hi = max_spin_->value();
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
    min_spin_->setRange(hard_min, hard_max);
    max_spin_->setRange(hard_min, hard_max);
}
