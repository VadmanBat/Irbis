#pragma once

#include "irbis/model/model-param.hpp"
#include "irbis/widgets/regulation-widget.h"
#include "irbis/widgets/tf-formula-panel.h"
#include "numina/classes/control/models/transfer-function.h"

#include <QWidget>
#include <vector>

namespace Ui {
class AnalysisTab;
}

class AnalysisTab : public QWidget {
    Q_OBJECT

private:
    Ui::AnalysisTab* ui;
    TfFormulaPanel* panel_{nullptr};
    RegulationWidget* metrics_{nullptr};
    ModelParam model_param_;
    numina::TransferFunction current_tf_;
    double delay_{0.0};

    void show_error(const QString& message);
    void install_custom_widgets();
    void update_metrics();
    bool apply_plant(std::vector<double> num, std::vector<double> den, double tau);

private slots:
    void addTransferFunction();
    void replaceTransferFunction();
    void clearCharts();
    void editPlant();
    void pastePlant();

public:
    explicit AnalysisTab(QWidget* parent = nullptr);
    ~AnalysisTab() override;

    void openHelp();
    void openSettings();
    void openChartSettings();
};
