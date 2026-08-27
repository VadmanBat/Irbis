#pragma once

#include "numina/classes/control/models/transfer-function.h"

#include <QDialog>

class FormulaView;
class TfDisplayWidget;

namespace Ui {
class TranFuncDialog;
}

class TranFuncDialog : public QDialog {
    Q_OBJECT

private:
    Ui::TranFuncDialog* ui;
    numina::TransferFunction tf_;
    double delay_tau_{0.0};
    TfDisplayWidget* formula_{nullptr};
    FormulaView* ht_{nullptr};
    FormulaView* wt_{nullptr};
    FormulaView* ode_{nullptr};
    FormulaView* first_{nullptr};
    FormulaView* euler_{nullptr};

    void fill_formula();
    void fill_poles();
    void fit_poles_table();
    void setup_copy_menus();
    void show_solutions();
    void copy_solution_text(const QString& text, QWidget* anchor);
    static QColor root_color(double value, bool dark);

public:
    /// delayTau>0: exact e^{−τp} — wrap h(t), w(t) as 1(t−τ)·f(t−τ). Lab still has no DelayedPlant.
    explicit TranFuncDialog(const numina::TransferFunction& tf, QWidget* parent = nullptr, double delayTau = 0.0);
    ~TranFuncDialog() override;

protected:
    void showEvent(QShowEvent* event) override;
};
