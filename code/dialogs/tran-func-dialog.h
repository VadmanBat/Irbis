#pragma once

#include "numina/classes/control/transfer-function.h"

#include <QDialog>

namespace Ui {
class TranFuncDialog;
}

class TranFuncDialog : public QDialog {
    Q_OBJECT

private:
    Ui::TranFuncDialog* ui;
    /// Own a copy so the dialog never depends on caller lifetime / moves.
    numina::TransferFunction tf_;
    double delay_tau_{0.0};

    void fill_poles();
    void setup_copy_menus();
    void show_solutions();
    void copy_solution_text(const QString& text, QWidget* anchor);
    static QColor root_color(double value);

public:
    /// delayTau>0: exact e^{−τp} — wrap h(t), w(t) as 1(t−τ)·f(t−τ). Lab still has no DelayedPlant.
    explicit TranFuncDialog(const numina::TransferFunction& tf, QWidget* parent = nullptr, double delayTau = 0.0);
    ~TranFuncDialog() override;
};
