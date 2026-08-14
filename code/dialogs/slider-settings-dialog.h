#pragma once

#include <QDialog>
#include <QString>

namespace Ui {
class SliderSettingsDialog;
}

struct SliderSettings {
    double min{};
    double max{};
    int intervals{1000};
    double value{};
};

class SliderSettingsDialog : public QDialog {
    Q_OBJECT

    Ui::SliderSettingsDialog* ui;
    SliderSettings defaults_{};

    void sync_step_hint();
    void restore_defaults();

public:
    SliderSettingsDialog(const QString& param_name, double hard_min, double hard_max, const SliderSettings& values,
                         const SliderSettings& defaults, QWidget* parent = nullptr);
    ~SliderSettingsDialog() override;

    [[nodiscard]] SliderSettings data() const;
};
