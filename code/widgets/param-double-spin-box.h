#pragma once

#include <QAbstractSpinBox>
#include <QDoubleSpinBox>
#include <QFocusEvent>
#include <QLocale>

/// Idle: always 2 fraction digits. While editing: up to decimals() digits (trailing zeros stripped).
class ParamDoubleSpinBox : public QDoubleSpinBox {
    static constexpr int kIdleDecimals = 2;

    bool editing_{false};

    [[nodiscard]] QString format_value(double value, int digits) const {
        QString text        = locale().toString(value, 'f', digits);
        const QString group = locale().groupSeparator();
        const QString dec   = locale().decimalPoint();
        if (!group.isEmpty() && group != dec)
            text.remove(group);
        text.remove(QChar(0x202F));
        text.remove(QChar(0x00A0));
        return text;
    }

public:
    explicit ParamDoubleSpinBox(QWidget* parent = nullptr) : QDoubleSpinBox(parent) {
        setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

    [[nodiscard]] QSize sizeHint() const override { return minimumSizeHint(); }

    [[nodiscard]] QSize minimumSizeHint() const override {
        QSize s = QDoubleSpinBox::minimumSizeHint();
        if (maximumWidth() < QWIDGETSIZE_MAX)
            s.setWidth(maximumWidth());
        return s;
    }

protected:
    [[nodiscard]] QString textFromValue(double value) const override {
        if (!editing_)
            return format_value(value, kIdleDecimals);

        QString text      = format_value(value, decimals());
        const QString dec = locale().decimalPoint();
        if (!dec.isEmpty() && text.contains(dec)) {
            while (text.endsWith(QLatin1Char('0')))
                text.chop(1);
            if (text.endsWith(dec))
                text.chop(dec.size());
        }
        return text;
    }

    void focusInEvent(QFocusEvent* event) override {
        editing_ = true;
        QDoubleSpinBox::focusInEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override {
        editing_ = false;
        QDoubleSpinBox::focusOutEvent(event);
    }
};
