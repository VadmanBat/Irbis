#pragma once

#include "code/charts/c0-c1-chart.h"
#include "code/model/model-param.hpp"
#include "code/widgets/reg-parameter.h"
#include "code/widgets/regulation-widget.h"
#include "code/widgets/tf-form/tran-func-form.h"
#include "numina/classes/control/controller-designer.h"
#include "numina/classes/control/transfer-function.h"

#include <QWidget>
#include <vector>

class QMenu;

namespace Ui {
class SynthesisTab;
}

class SynthesisTab : public QWidget {
    Q_OBJECT

private:
    Ui::SynthesisTab* ui;
    TranFuncForm* form_{nullptr};
    RegulationWidget* metrics_{nullptr};
    std::vector<RegParameter*> parameters_;
    QMenu* charts_menu_{nullptr};
    bool show_plane_{true};
    ModelParam model_param_;
    numina::TransferFunction plant_tf_;
    numina::TransferFunction current_tf_;

    void install_custom_widgets();
    void setup_metrics();
    void show_error(const QString& message);
    void apply_current_controller(bool replace_last);
    void update_metrics_from_bank();
    void block_param_signals(bool block);
    void apply_pi_settings(double kp, double ti, bool replace_last);
    void apply_design(const numina::ControllerDesigner::Design& d, bool replace_last);
    void sync_c0c1_selection_from_params();
    void update_c0c1_visibility();
    void update_regulator_face();
    void changeEvent(QEvent* event) override;
    [[nodiscard]] bool is_pi_structure() const noexcept;
    [[nodiscard]] numina::ControllerDesigner::Criterion selected_criterion() const noexcept;
    [[nodiscard]] numina::ControllerDesigner::Law selected_law() const noexcept;
    [[nodiscard]] numina::ControllerDesigner::Region selected_region() const noexcept;
    [[nodiscard]] bool build_plant(numina::TransferFunction& out);

private slots:
    void addTransferFunction();
    void replaceTransferFunction();
    void clearCharts();
    void openSettings();
    void openHelp();
    void autoSynthesize();
    void onSamplePicked(const C0C1Chart::Sample& sample);

public:
    explicit SynthesisTab(QWidget* parent = nullptr);
    ~SynthesisTab() override;
};
