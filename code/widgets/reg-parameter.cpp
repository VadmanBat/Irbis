#include "code/widgets/reg-parameter.h"

#include "code/widgets/param-double-spin-box.h"

#include <algorithm>
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QWidget>

namespace {
constexpr int kCheckW       = 22;
constexpr int kValueW       = 72;
constexpr int kSpinH        = 28;
constexpr int kBtn          = 28;
constexpr int kEditDecimals = 6;

QWidget* widget_host(QObject* parent) {
    return qobject_cast<QWidget*>(parent);
}
}

RegParameter::RegParameter(const QString& title, double min, double max, double minValue, double maxValue,
                           QObject* parent)
    : QObject(parent),
      check_box_(new QCheckBox(widget_host(parent))),
      label_(new QLabel(title, widget_host(parent))),
      value_spin_(new ParamDoubleSpinBox(widget_host(parent))),
      settings_btn_(new QPushButton(QStringLiteral("⚙"), widget_host(parent))),
      slider_(new DoubleSlider(Qt::Horizontal, widget_host(parent))),
      hard_min_(std::min(min, max)),
      hard_max_(std::max(min, max)),
      min_(minValue),
      max_(maxValue) {
    check_box_->setObjectName(QStringLiteral("regParamCheck"));
    label_->setObjectName(QStringLiteral("regParamLabel"));
    value_spin_->setObjectName(QStringLiteral("regParamValue"));
    settings_btn_->setObjectName(QStringLiteral("regParamSettings"));
    slider_->setObjectName(QStringLiteral("regParamSlider"));

    check_box_->setFixedWidth(kCheckW);
    check_box_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label_->setTextFormat(Qt::RichText);
    label_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    value_spin_->setDecimals(kEditDecimals);
    value_spin_->setKeyboardTracking(false);
    value_spin_->setSingleStep(0.01);
    value_spin_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value_spin_->setFixedSize(kValueW, kSpinH);
    value_spin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    value_spin_->setToolTip(tr("Значение параметра (min…max ползунка)"));

    slider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    slider_->setMinimumWidth(64);

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
    connect(value_spin_, &QDoubleSpinBox::valueChanged, this, &RegParameter::on_spin_changed);
    connect(settings_btn_, &QPushButton::clicked, this, &RegParameter::open_settings);

    apply_default_style();
}

void RegParameter::placeIn(QGridLayout* grid, int row) {
    grid->addWidget(check_box_, row, 0, Qt::AlignVCenter);
    grid->addWidget(label_, row, 1);
    grid->addWidget(value_spin_, row, 2, Qt::AlignVCenter);
    grid->addWidget(slider_, row, 3);
    grid->addWidget(settings_btn_, row, 4, Qt::AlignVCenter);
}

void RegParameter::blockUiSignals(bool block) {
    blockSignals(block);
    check_box_->blockSignals(block);
    slider_->blockSignals(block);
    value_spin_->blockSignals(block);
}

void RegParameter::enable(bool checked) {
    label_->setEnabled(checked);
    value_spin_->setEnabled(checked);
    slider_->setEnabled(checked);
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
    const QSignalBlocker block_slider(slider_);
    const QSignalBlocker block_spin(value_spin_);
    slider_->setValue(v);
    value_spin_->setValue(slider_->value());
}
