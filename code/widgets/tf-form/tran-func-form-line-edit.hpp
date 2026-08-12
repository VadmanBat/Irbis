#pragma once

#include "code/util/format.hxx"
#include "code/util/style-core.hpp"
#include "code/widgets/tf-form/tran-func-form.h"

#include <QFocusEvent>
#include <QLabel>
#include <QLineEdit>

/// Nested editor type for TranFuncForm (private header, not public API).
class TranFuncForm::LineEdit : public QLineEdit {
public:
    explicit LineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

    double stored{0.0};
    bool dirty{false};
    bool cleared{false};

    void setStored(double v) {
        stored = num_format::roundSignificant(v, num_format::STORED_DIGITS);
        showDisplay();
    }

    void showDisplay() {
        const QString disp = num_format::formatCoeff(stored, num_format::SIGNIFICANT_DIGITS, true);
        blockSignals(true);
        setText(disp);
        blockSignals(false);
        dirty   = false;
        cleared = false;
    }

    void showStoredForEdit() {
        const QString full = num_format::formatCoeff(stored, num_format::STORED_DIGITS, true);
        blockSignals(true);
        setText(full);
        blockSignals(false);
        dirty   = false;
        cleared = false;
    }

    void commitIfDirty() {
        if (!dirty)
            return;
        const QString t = text().trimmed();
        if (cleared || t.isEmpty() || t == QLatin1String("+") || t == QLatin1String("-")) {
            stored = 0.0;
        }
        else {
            bool ok        = false;
            const double v = num_format::parse(t, &ok);
            stored         = ok ? num_format::roundSignificant(v, num_format::STORED_DIGITS) : 0.0;
        }
        dirty   = false;
        cleared = false;
    }

protected:
    void focusInEvent(QFocusEvent* event) override {
        QLineEdit::focusInEvent(event);
        showStoredForEdit();
        selectAll();
    }
};

namespace tf_form_style {
inline void apply_coeff_sign(QLineEdit* line_edit, double value) {
    const char* sign = "zero";
    if (value > 0)
        sign = "positive";
    else if (value < 0)
        sign = "negative";
    style_util::setProperty(line_edit, "coeffSign", QByteArray(sign));
}

inline void apply_power_active(QLabel* label, bool active) {
    style_util::setProperty(label, "powerActive", active ? QByteArray("true") : QByteArray("false"));
}
} // namespace tf_form_style
