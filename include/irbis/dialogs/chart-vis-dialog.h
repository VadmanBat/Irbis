#pragma once
// Created by Vadim on 10.09.2026.

#include "irbis/charts/response-chart-bank.h"

#include <QDialog>

namespace Ui {
class ChartVisDialog;
}

class ChartVisDialog : public QDialog {
    Q_OBJECT

    Ui::ChartVisDialog* ui;

    void apply_to_checks(ChartVisibility vis);
    void keep_at_least_one();

public:
    explicit ChartVisDialog(ChartVisibility vis, QWidget* parent = nullptr);
    ~ChartVisDialog() override;

    [[nodiscard]] ChartVisibility data() const;
};
