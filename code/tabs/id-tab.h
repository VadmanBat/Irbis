#pragma once

#include "code/model/id-settings.hpp"
#include "code/widgets/tf-display-widget.h"
#include "numina/classes/control/models/transfer-function.h"

#include <QString>
#include <QWidget>
#include <utility>
#include <vector>

class ChartPanel;

namespace Ui {
class IdTab;
}

/// Идентификация: файл → Симою (статика) или интегратор k/p (астатика) → ПФ и h(t).
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
    ChartPanel* chart_{nullptr};

    QString file_path_;
    Series step_series_;
    Series valve_series_;
    Series signal_series_;
    bool has_data_{false};

    void install_custom_widgets();
    void show_error(const QString& message);
    [[nodiscard]] IdSettings read_id_settings() const;
    void sync_plant_kind_ui();
    void sync_struct_ui();
    void maybe_show_structure_template();
    void sync_tf_actions();
    [[nodiscard]] bool load_step_file(const QString& path);
    [[nodiscard]] bool load_valve_signal_file(const QString& path);
    [[nodiscard]] bool preview_loaded_file();
    void show_file_preview();
    void show_overlay(const Series& experiment, const Series& model);
    void apply_result(const numina::TransferFunction& plant, double tau, const Series& experiment);
    void run_static(const Series& experimental_h);
    void run_astatic(Method method);

private slots:
    void openFile();
    void runIdentification();
    void clearAll();
    void copyIdentifiedTf();
    void showEquations();

public:
    explicit IdTab(QWidget* parent = nullptr);
    ~IdTab() override;
};
