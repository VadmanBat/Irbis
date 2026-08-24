#pragma once

#include "code/model/id-settings.hpp"
#include "code/model/model-param.hpp"
#include "code/widgets/regulation-widget.h"
#include "code/widgets/tf-display-widget.h"
#include "numina/classes/control/transfer-function.h"

#include <QString>
#include <QWidget>
#include <utility>
#include <vector>

class QMenu;

namespace Ui {
class IdTab;
}

/// Идентификация: файл → Дюамель? → DeadTimeEstimator? → Simoyu → ПФ и графики.
class IdTab : public QWidget {
    Q_OBJECT

private:
    enum class Method : int {
        StepResponse = 0,
        ValveSignal  = 1,
    };

    using Series = std::vector<std::pair<double, double>>;

    Ui::IdTab* ui;
    TfDisplayWidget* display_{nullptr};
    RegulationWidget* metrics_{nullptr};
    QMenu* charts_menu_{nullptr};
    ModelParam model_param_;
    IdSettings id_settings_;

    QString file_path_;
    Series step_series_;
    Series valve_series_;
    Series signal_series_;
    bool has_data_{false};

    void install_custom_widgets();
    void show_error(const QString& message);
    [[nodiscard]] bool load_step_file(const QString& path);
    [[nodiscard]] bool load_valve_signal_file(const QString& path);
    void apply_result(const numina::TransferFunction& plant, double tau, const Series& experimental_h);

private slots:
    void openFile();
    void runIdentification();
    void clearAll();
    void openSettings();
    void openIdSettings();

public:
    explicit IdTab(QWidget* parent = nullptr);
    ~IdTab() override;
};
