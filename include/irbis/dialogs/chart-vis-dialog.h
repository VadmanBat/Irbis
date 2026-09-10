#pragma once

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

