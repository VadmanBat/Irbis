#pragma once

#include "code/model/model-param.hpp"
#include "code/widgets/regulation-widget.h"
#include "code/widgets/tf-form/tran-func-form.h"
#include "numina/classes/control/models/transfer-function.h"

#include <QWidget>

class QMenu;

namespace Ui {
class AnalysisTab;
}

class AnalysisTab : public QWidget {
    Q_OBJECT

private:
    Ui::AnalysisTab* ui;
    TranFuncForm* form_{nullptr};
    RegulationWidget* metrics_{nullptr};
    QMenu* charts_menu_{nullptr};
    ModelParam model_param_;
    numina::TransferFunction current_tf_;

    void show_error(const QString& message);
    void install_custom_widgets();
    void update_metrics();

private slots:
    void addTransferFunction();
    void replaceTransferFunction();
    void clearCharts();
    void openSettings();

public:
    explicit AnalysisTab(QWidget* parent = nullptr);
    ~AnalysisTab() override;
};
