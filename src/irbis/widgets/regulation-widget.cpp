#include "irbis/widgets/regulation-widget.h"

#include "irbis/util/format.hxx"
#include "irbis/util/style-core.hpp"

#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

RegulationWidget::RegulationWidget(int rows, int cols, QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("RegulationWidget"));
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    layout_ = new QGridLayout(this);
    layout_->setContentsMargins(4, 4, 4, 4);
    layout_->setHorizontalSpacing(6);
    layout_->setVerticalSpacing(6);

    const auto size = rows * cols;
    labels_.reserve(size);
    line_edits_.reserve(size);
    last_values_.assign(size, -1);
    colors_.assign(size, {1, 2});

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            auto* label = new QLabel(this);
            label->setObjectName(QStringLiteral("metricLabel"));
            label->setTextFormat(Qt::RichText);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
            label->setWordWrap(false);

            auto* edit = new QLineEdit(this);
            edit->setObjectName(QStringLiteral("metricValue"));
            edit->setReadOnly(true);
            edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            style_util::setProperty(edit, "metricFlash", 0);

            labels_.push_back(label);
            line_edits_.push_back(edit);
            layout_->addWidget(label, i, j * 2);
            layout_->addWidget(edit, i, j * 2 + 1);
        }
        layout_->setRowStretch(i, 1);
    }

    for (int j = 0; j < cols; ++j) {
        layout_->setColumnStretch(j * 2, 0);
        layout_->setColumnStretch(j * 2 + 1, 0);
    }

    apply_default_style();
}

void RegulationWidget::apply_default_style() {
    const QFontMetrics fm(font());
    const int row_h = qMax(24, fm.height() + fm.descent() + 8);
    const int val_w = fm.horizontalAdvance(QStringLiteral("-1.23456e+1230")) + 16;

    for (auto* label : labels_) {
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setTextFormat(Qt::RichText);
        label->setMinimumHeight(row_h);
        label->setMinimumWidth(0);
    }
    for (auto* edit : line_edits_) {
        edit->setReadOnly(true);
        edit->setMinimumHeight(row_h);
        edit->setFixedWidth(val_w);
    }
}

QString RegulationWidget::format_double(double value) {
    return num_format::format(value, num_format::SIGNIFICANT_DIGITS);
}

int RegulationWidget::color_index(std::size_t index, double old_value, double new_value) const {
    if (old_value == -1)
        return 0;
    if (new_value < old_value)
        return colors_[index].first;
    if (new_value > old_value)
        return colors_[index].second;
    return 0;
}

void RegulationWidget::update_cell_style(std::size_t index, double new_value) {
    const int flash = color_index(index, last_values_[index], new_value);
    style_util::setProperty(line_edits_[index], "metricFlash", flash);
}

void RegulationWidget::setLabels(const std::vector<QString>& labelNames, const std::vector<QString>& tooltips) {
    const auto n = std::min(labels_.size(), labelNames.size());
    for (std::size_t i = 0; i < n; ++i) {
        labels_[i]->setText(labelNames[i]);
        const QString tip = i < tooltips.size() ? tooltips[i] : QString();
        labels_[i]->setToolTip(tip);
        line_edits_[i]->setToolTip(tip);
        labels_[i]->adjustSize();
        labels_[i]->updateGeometry();
    }
    updateGeometry();
}

void RegulationWidget::setColors(const std::vector<std::pair<int, int>>& valueColors) {
    const auto n = std::min(colors_.size(), valueColors.size());
    for (std::size_t i = 0; i < n; ++i)
        colors_[i] = valueColors[i];
}

void RegulationWidget::updateValues(const std::vector<double>& values) {
    if (values.empty()) {
        const std::size_t n = line_edits_.size();
        for (std::size_t i = 0; i < n; ++i) {
            line_edits_[i]->setText({});
            last_values_[i] = -1;
            style_util::setProperty(line_edits_[i], "metricFlash", 0);
        }
        return;
    }
    const auto n = std::min(line_edits_.size(), values.size());
    for (std::size_t i = 0; i < n; ++i) {
        update_cell_style(i, values[i]);
        line_edits_[i]->setText(format_double(values[i]));
        last_values_[i] = values[i];
    }
}
