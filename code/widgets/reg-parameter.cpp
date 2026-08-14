#include "code/widgets/reg-parameter.h"

#include "code/dialogs/slider-settings-dialog.h"

#include <algorithm>
#include <cmath>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>

namespace {
constexpr int kCheckW = 22;
constexpr int kValueW = 72;
constexpr int kSpinH  = 28;
constexpr int kBtn    = 28;
} // namespace

RegParameter::RegParameter(const QString& title, double min, double max, double minValue, double maxValue,
                           QObject* parent)
    : QObject(parent),
      check_box_(new QCheckBox),
      label_(new QLabel(title)),
      value_label_(new QLabel),
      settings_btn_(new QPushButton(QStringLiteral("⚙"))),
      slider_(new DoubleSlider(Qt::Horizontal)),
      layout_(new QHBoxLayout),
      hard_min_(std::min(min, max)),
      hard_max_(std::max(min, max)),
      min_(minValue),
      max_(maxValue) {
    check_box_->setObjectName(QStringLiteral("regParamCheck"));
    label_->setObjectName(QStringLiteral("regParamLabel"));
    value_label_->setObjectName(QStringLiteral("regParamValue"));
    settings_btn_->setObjectName(QStringLiteral("regParamSettings"));
    slider_->setObjectName(QStringLiteral("regParamSlider"));

    check_box_->setFixedWidth(kCheckW);
    check_box_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label_->setTextFormat(Qt::RichText);
    label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    value_label_->setFixedWidth(kValueW);
    value_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value_label_->setMinimumHeight(kSpinH);

    settings_btn_->setToolTip(tr("Диапазон и шаг ползунка"));
    settings_btn_->setFocusPolicy(Qt::TabFocus);

    if (max_ < min_)
        std::swap(min_, max_);
    if (!(max_ > min_))
        max_ = min_ + 1.0;
    factory_min_       = min_;
    factory_max_       = max_;
    factory_intervals_ = intervals_;

    update_slider_range();
    enable(false);

    connect(check_box_, &QCheckBox::toggled, this, &RegParameter::enable);
    connect(slider_, &DoubleSlider::doubleValueChanged, this, &RegParameter::on_slider_moved);
    connect(settings_btn_, &QPushButton::clicked, this, &RegParameter::open_settings);

    auto* name = new QHBoxLayout;
    name->setContentsMargins(0, 0, 0, 0);
    name->setSpacing(2);
    name->addWidget(check_box_, 0, Qt::AlignVCenter);
    name->addWidget(label_, 0, Qt::AlignVCenter);

    auto* row = qobject_cast<QHBoxLayout*>(layout_);
    row->setContentsMargins(0, 4, 0, 4);
    row->setSpacing(6);
    row->setAlignment(Qt::AlignVCenter);

    row->addLayout(name, 0);
    row->addWidget(value_label_, 0, Qt::AlignVCenter);
    row->addWidget(slider_, 1);
    row->addWidget(settings_btn_, 0, Qt::AlignVCenter);

    apply_default_style();
    refresh_value_label();
}

void RegParameter::update_slider_range() {
    if (!(min_ < max_))
        return;
    const auto value = slider_->value();
    slider_->setRange(min_, max_, intervals_);
    slider_->setValue(value);
    refresh_value_label();
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
    update_slider_range();
}

void RegParameter::enable(bool checked) {
    label_->setEnabled(checked);
    value_label_->setEnabled(checked);
    slider_->setEnabled(checked);
}

void RegParameter::on_slider_moved(double) {
    refresh_value_label();
}

void RegParameter::refresh_value_label() {
    value_label_->setText(QLocale().toString(slider_->value(), 'f', 2));
}

void RegParameter::apply_default_style() {
    settings_btn_->setFixedSize(kBtn, kBtn);
}

bool RegParameter::enabled() const {
    return check_box_->isChecked();
}

double RegParameter::value() const {
    return slider_->value();
}

void RegParameter::setEnabled(bool on) {
    check_box_->setChecked(on);
    enable(on);
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
