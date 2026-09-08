#pragma once

#include <QSlider>

class DoubleSlider : public QSlider {
    Q_OBJECT

private:
    double min_{0.0};
    double max_{1.0};
    double single_step_{0.01};
    int intervals_{100};
    /// Continuous value: exact when set programmatically; discrete after user drag.
    double value_{0.0};

    void sync_handle_from_value();

private slots:
    void on_int_value_changed(int);

public:
    explicit DoubleSlider(Qt::Orientation orientation = Qt::Horizontal, QWidget* parent = nullptr);

    void setRange(double min, double max, int intervals);
    /// Exact continuous value (not snapped to slider ticks).
    [[nodiscard]] double value() const noexcept { return value_; }

public slots:
    /// Store exact `value`; handle shows nearest tick. Does not quantize `value()`.
    void setValue(double value);

signals:
    void doubleValueChanged(double value);

protected:
    void paintEvent(QPaintEvent* event) override;
};
