#pragma once

#include "irbis/control/rim-law.hpp"
#include "irbis/util/tf-stepper.hpp"
#include "irbis/widgets/tf-formula-panel.h"
#include "numina/classes/control/discrete/pid-controller.h"
#include "numina/classes/control/models/transfer-function.h"

#include <memory>
#include <QWidget>
#include <utility>
#include <vector>

class ChartPanel;

namespace Ui {
class RimTab;
}

/// Настройка РИМ: объект W(p), PidSettings, уставка; y(t) реального и идеального регулятора.
class RimTab : public QWidget {
    Q_OBJECT

private:
    using Series = std::vector<std::pair<double, double>>;

    struct Session {
        TfStepper plant_real;
        TfStepper plant_ideal;
        rim::Regulator real;
        TfStepper ideal;
        double t{};
        double dt{};
        double deadzone{};
        double y_real{};
        double y_ideal{};
        Series sp;
        Series y_r;
        Series y_i;
        Series mu_r;
        Series mu_i;

        Session(TfStepper pr, TfStepper pi, rim::Regulator r, TfStepper ir, double step)
            : plant_real(std::move(pr)),
              plant_ideal(std::move(pi)),
              real(std::move(r)),
              ideal(std::move(ir)),
              dt(step) {}
    };

    Ui::RimTab* ui;
    TfFormulaPanel* panel_{nullptr};
    ChartPanel* y_chart_{nullptr};
    ChartPanel* mu_chart_{nullptr};
    std::unique_ptr<Session> session_;
    std::vector<double> plant_num_;
    std::vector<double> plant_den_;
    double plant_tau_{0.0};
    bool has_plant_{false};

    void install_custom_widgets();
    void show_error(const QString& message);
    void sync_law_ui();
    void set_run_locked(bool on);
    void redraw_charts();
    void seed_initial_point(double sp, double mu0);
    void run_segment(double duration, double sp);
    [[nodiscard]] bool build_plant(numina::TransferFunction& out, double& tau);
    bool apply_plant(std::vector<double> num, std::vector<double> den, double tau);
    [[nodiscard]] numina::ControlLaw selected_law() const noexcept;
    [[nodiscard]] numina::PidSettings read_pid_settings() const;

private slots:
    void runSimulation();
    void continueSimulation();
    void resetSimulation();
    void editPlant();
    void pastePlant();

public:
    explicit RimTab(QWidget* parent = nullptr);
    ~RimTab() override;
};
