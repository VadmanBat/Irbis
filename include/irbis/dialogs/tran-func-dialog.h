#pragma once

#include "numina/classes/control/models/transfer-function.h"

#include <QColor>
#include <QDialog>

namespace Ui {
class TranFuncDialog;
}

class TranFuncDialog : public QDialog {
    Q_OBJECT

private:
    Ui::TranFuncDialog* ui;
    numina::TransferFunction tf_;
    double delay_tau_{0.0};

    void fill_formula();
    void fill_poles();
    void fit_poles_table();
    void setup_copy_menus();
    void show_solutions();
    void copy_solution_text(const QString& text, QWidget* anchor);
    static QColor root_color(double value, bool dark);

public:
    /// delayTau>0: wrap symbolic h(t), w(t) as 1(t−τ)·f(t−τ). Charts/quality bind DelayedPlant in tf_builder.
    explicit TranFuncDialog(const numina::TransferFunction& tf, QWidget* parent = nullptr, double delayTau = 0.0);
    ~TranFuncDialog() override;

protected:
    void showEvent(QShowEvent* event) override;
};
