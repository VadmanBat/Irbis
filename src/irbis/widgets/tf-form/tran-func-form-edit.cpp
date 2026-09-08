#include "irbis/widgets/tf-form/tran-func-form-line-edit.hpp"
#include "irbis/widgets/tf-form/tran-func-form.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

using tf_form_style::apply_coeff_sign;
using tf_form_style::apply_power_active;

QLayout* TranFuncForm::create_coeff_row(VecLine& line_edits, int factor) {
    auto* layout = new QHBoxLayout;
    int p        = -1;
    for (auto& line_edit : line_edits) {
        line_edit = new LineEdit(this);
        line_edit->setObjectName(QStringLiteral("tfCoeff"));
        layout->addWidget(line_edit);
        line_edit->setAlignment(Qt::AlignRight);
        line_edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        line_edit->setStored(0.0);
        apply_coeff_sign(line_edit, 0);
        adjust_line_edit_width(line_edit);

        auto* label = new QLabel(QString("p<sup>%1</sup>").arg(++p), this);
        label->setObjectName(QStringLiteral("tfPower"));
        apply_power_active(label, false);
        layout->addWidget(label);

        is_active_.push_back(false);
        const std::size_t index = is_active_.size() - 1;

        connect(line_edit, &QLineEdit::textEdited, this, [line_edit, label](const QString& text) {
            line_edit->dirty   = true;
            line_edit->cleared = text.trimmed().isEmpty();
            if (auto corrected = correct_line(text); !corrected.isEmpty()) {
                line_edit->blockSignals(true);
                line_edit->setText(corrected);
                line_edit->blockSignals(false);
            }
            const QString cur = line_edit->text().trimmed();
            if (cur.isEmpty()) {
                apply_power_active(label, false);
                apply_coeff_sign(line_edit, 0.0);
            }
            else {
                bool ok            = false;
                const double value = num_format::parse(cur, &ok);
                apply_power_active(label, ok && value != 0.0);
                apply_coeff_sign(line_edit, ok ? value : 0.0);
            }
            adjust_line_edit_width(line_edit);
        });

        connect(line_edit, &QLineEdit::editingFinished, this, [this, line_edit, label, factor, p, index] {
            if (line_edit->text().trimmed().isEmpty())
                line_edit->cleared = true;
            line_edit->commitIfDirty();
            line_edit->showDisplay();
            adjust_line_edit_width(line_edit);
            const double v = line_edit->stored;
            apply_power_active(label, v != 0.0);
            apply_coeff_sign(line_edit, v);

            if (v == 0.0) {
                if (is_active_[index]) {
                    id_ -= factor << p;
                    is_active_[index] = false;
                }
            }
            else if (!is_active_[index]) {
                id_ += factor << p;
                is_active_[index] = true;
            }
        });
    }
    layout->setAlignment(Qt::AlignLeft);
    return layout;
}

void TranFuncForm::set_field_value(LineEdit* line_edit, double value) {
    line_edit->setStored(value);
    apply_coeff_sign(line_edit, line_edit->stored);
    adjust_line_edit_width(line_edit);
}

void TranFuncForm::set_coeff_fields(VecLine& fields, const Vec& high_to_low) {
    for (auto* le : fields)
        set_field_value(le, 0.0);
    if (high_to_low.empty())
        return;
    const int n = static_cast<int>(fields.size());
    const int m = static_cast<int>(high_to_low.size());
    for (int j = 0; j < n; ++j) {
        const int idx = m - 1 - j;
        if (idx >= 0)
            set_field_value(fields[static_cast<std::size_t>(j)], high_to_low[static_cast<std::size_t>(idx)]);
    }
}

void TranFuncForm::rebuild_activity_mask() {
    id_ = 0;
    is_active_.assign(numerator_.size() + denominator_.size(), false);
    const int n = static_cast<int>(numerator_.size());
    for (int p = 0; p < n; ++p) {
        if (numerator_[static_cast<std::size_t>(p)]->stored != 0.0) {
            is_active_[static_cast<std::size_t>(p)] = true;
            id_ += 1 << p;
        }
    }
    const int m = static_cast<int>(denominator_.size());
    for (int p = 0; p < m; ++p) {
        const std::size_t index = static_cast<std::size_t>(n + p);
        if (denominator_[static_cast<std::size_t>(p)]->stored != 0.0) {
            is_active_[index] = true;
            id_ += (1 << n) << p;
        }
    }
}

void TranFuncForm::adjust_line_edit_width(QLineEdit* line_edit) {
    const int width = qMax(36, line_edit->fontMetrics().horizontalAdvance(line_edit->text()) + 16);
    line_edit->setFixedWidth(width);
}

TranFuncForm::Vec TranFuncForm::reverse_optimize(const Vec& container) {
    auto first      = container.rbegin();
    const auto last = container.rend();
    if (first == last)
        return {};
    while (first != last && *first == 0)
        ++first;
    return {first, last};
}

TranFuncForm::Vec TranFuncForm::get_line_edit_data(const VecLine& line_edits) {
    const auto size = line_edits.size();
    Vec values(size);
    for (std::size_t i = 0; i < size; ++i) {
        line_edits[i]->commitIfDirty();
        values[i] = line_edits[i]->stored;
    }
    return reverse_optimize(values);
}

QString TranFuncForm::correct_line(const QString& text) {
    if (text.isEmpty())
        return {};
    if (text.at(0) == ',')
        return QStringLiteral("+0") + text;
    if (text.at(0).isDigit())
        return QChar('+') + text;
    if (text.size() > 1 && text.at(1) == ',')
        return text.at(0) + QStringLiteral("0") + text.mid(1);
    return {};
}

void TranFuncForm::reposition_clip_buttons() {
    const QRect g     = delay_group_->geometry();
    constexpr int gap = 2;
    constexpr int btn = 24;
    const int x1      = g.right() - 2 * btn - gap;
    const int x2      = g.right() - btn;
    int y             = g.bottom() + 1;
    if (y + btn > height())
        y = qMax(0, height() - btn);
    if (y < g.top())
        y = g.bottom() - btn;
    copy_btn_->move(x1, y);
    paste_btn_->move(x2, y);
    copy_btn_->raise();
    paste_btn_->raise();
}
