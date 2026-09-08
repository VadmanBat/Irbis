#pragma once

#include <QDoubleSpinBox>
#include <QLocale>

/// QDoubleSpinBox that keeps max precision for editing but does not pad trailing zeros
/// (shows 500 instead of 500.000, 0.01 instead of 0.0100).
class CompactDoubleSpinBox : public QDoubleSpinBox {
public:
    using QDoubleSpinBox::QDoubleSpinBox;

protected:
    [[nodiscard]] QString textFromValue(double value) const override {
        // Fixed-point with full allowed precision, then strip trailing zeros.
        QString text = locale().toString(value, 'f', decimals());

        // Group separators make the field wider and can confuse re-parse; drop them.
        // Qt6: groupSeparator()/decimalPoint() return QString.
        const QString group = locale().groupSeparator();
        const QString dec   = locale().decimalPoint();
        if (!group.isEmpty() && group != dec)
            text.remove(group);
        text.remove(QChar(0x202F));
        text.remove(QChar(0x00A0));

        if (!dec.isEmpty() && text.contains(dec)) {
            while (text.endsWith(QLatin1Char('0')))
                text.chop(1);
            if (text.endsWith(dec))
                text.chop(dec.size());
        }
        return text;
    }
};
