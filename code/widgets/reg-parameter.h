#pragma once

#include "code/widgets/double-slider.h"

#include <QObject>
#include <QString>

class QCheckBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QPushButton;

class RegParameter : public QObject {
    Q_OBJECT

private:
    QCheckBox* check_box_{nullptr};
    QLabel* label_{nullptr};
    QDoubleSpinBox* value_spin_{nullptr};
    QPushButton* settings_btn_{nullptr};
    DoubleSlider* slider_{nullptr};
    double hard_min_{};
    double hard_max_{};
    double min_{};
    double max_{};
    int intervals_{1000};
    double factory_min_{};
    double factory_max_{};
    int factory_intervals_{1000};

    void apply_default_style();
    void update_slider_range();
    void open_settings();

private slots:
    void enable(bool checked);
    void on_slider_moved(double v);
    void on_spin_changed(double v);

public:
    RegParameter(const QString& title, double min, double max, double minValue, double maxValue,
                 QObject* parent = nullptr);

    void placeIn(QGridLayout* grid, int row);
    void blockUiSignals(bool block);

    [[nodiscard]] QCheckBox* checkBox() const { return check_box_; }
    [[nodiscard]] DoubleSlider* slider() const { return slider_; }
    [[nodiscard]] bool enabled() const;
    [[nodiscard]] double value() const;
    [[nodiscard]] double rangeMin() const { return min_; }
    [[nodiscard]] double rangeMax() const { return max_; }

    void setEnabled(bool on);
    void setValue(double v);
    void setRange(double min, double max);
    void setLimits(double hard_min, double hard_max);
    /// Expand [min,max] only if needed so `v` is inside; never shrinks the range.
    void ensureValueInRange(double v);

signals:
    void valueChanged(double value);
};
