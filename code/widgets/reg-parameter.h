#pragma once

#include "code/widgets/double-slider.h"

#include <QObject>
#include <QString>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QLayout;

class RegParameter : public QObject {
    Q_OBJECT

private:
    QCheckBox* check_box_{nullptr};
    QLabel* label_{nullptr};
    QLabel* value_label_{nullptr};
    QDoubleSpinBox* min_spin_{nullptr};
    QDoubleSpinBox* max_spin_{nullptr};
    DoubleSlider* slider_{nullptr};
    QLayout* layout_{nullptr};

    static constexpr int kSliderIntervals = 1000;

    void apply_default_style();
    void refresh_value_label();

private slots:
    void update_slider_range();
    void enable(bool checked);
    void on_slider_moved(double v);

public:
    RegParameter(const QString& title, double min, double max, double minValue, double maxValue,
                 QObject* parent = nullptr);

    [[nodiscard]] QLayout* layout() const { return layout_; }
    [[nodiscard]] QCheckBox* checkBox() const { return check_box_; }
    [[nodiscard]] DoubleSlider* slider() const { return slider_; }
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] double value() const;
    [[nodiscard]] double rangeMin() const;
    [[nodiscard]] double rangeMax() const;

    void setEnabled(bool on);
    void setValue(double v);
    void setRange(double min, double max);
    void setLimits(double hard_min, double hard_max);
    /// Expand [min,max] only if needed so `v` is inside; never shrinks the range.
    void ensureValueInRange(double v);
};
