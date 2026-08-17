#pragma once

#include "code/model/model-param.hpp"

#include <QDialog>

namespace Ui {
class ModParDialog;
}

class ModParDialog : public QDialog {
    Q_OBJECT

private:
    Ui::ModParDialog* ui;
    bool allow_ideal_delay_{false};

    void on_auto_time_range_toggled(bool checked);
    void on_auto_time_intervals_toggled(bool checked);
    void on_auto_freq_range_toggled(bool checked);
    void on_auto_freq_intervals_toggled(bool checked);
    void on_use_pade_toggled(bool checked);

public:
    /// allowIdealDelay: analysis may keep e^{-τp} exact; order spin is then gated by «Паде».
    explicit ModParDialog(const ModelParam& values, QWidget* parent = nullptr, bool allowIdealDelay = false);
    ~ModParDialog() override;

    [[nodiscard]] ModelParam data() const;
};
