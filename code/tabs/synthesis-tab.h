#pragma once

#include "code/charts/c0-c1-chart.h"
#include "code/charts/response-chart-bank.h"
#include "code/model/model-param.hpp"
#include "code/widgets/reg-parameter.h"
#include "code/widgets/regulation-widget.h"
#include "code/widgets/tf-form/tran-func-form.h"
#include "numina/classes/control/regulator-designer.h"
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
    ResponseChartBank* charts_{nullptr};
    C0C1Chart* c0c1_chart_{nullptr};
    QMenu* charts_menu_{nullptr};
    ModelParam model_param_;
    numina::TransferFunction plant_tf_;   ///< Object for form «Подробнее»
    numina::TransferFunction current_tf_; ///< Closed loop for charts / metrics

    void install_custom_widgets();
    void setup_metrics();
    void show_error(const QString& message);
    void apply_current_regulator(bool replace_last);
    void update_metrics_from_bank();
    void block_param_signals(bool block);
    void apply_pi_settings(double kp, double tu, bool replace_last);
    void apply_design(const numina::RegulatorDesigner::Settings& s, bool replace_last);
    void sync_c0c1_selection_from_params();
    [[nodiscard]] numina::RegulatorDesigner::Criterion selected_criterion() const noexcept;
    [[nodiscard]] numina::RegulatorDesigner::Law selected_law() const noexcept;
    [[nodiscard]] numina::RegulatorDesigner::Region selected_region() const noexcept;
    [[nodiscard]] bool build_plant(numina::TransferFunction& out);

private slots:
    void addTransferFunction();
    void replaceTransferFunction();
    void clearCharts();
    void openSettings();
    void openHelp();
    void autoSynthesize();
    void on_c0c1_sample_picked(const C0C1Chart::Sample& sample);

public:
    explicit SynthesisTab(QWidget* parent = nullptr);
    ~SynthesisTab() override;
};
