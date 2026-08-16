#include "code/tabs/id-tab.h"
#include "code/tabs/tab-shell.hpp"
#include "code/util/data-file-parser.hpp"
#include "code/util/tf-builder.hpp"
#include "numina/classes/control/duhamel-solver.h"
#include "numina/classes/control/simoyu-identifier.h"
#include "ui_id-tab.h"

#include <algorithm>
#include <QFileInfo>
#include <QMessageBox>
#include <utility>

bool IdTab::load_step_file(const QString& path) {
    auto opt = data_file_parser::readStepResponse(path);
    if (!opt) {
        show_error(
            tr("Не удалось прочитать пары time, value.\n"
               "Числа с «.»/«,» и научной записью; мусор отбрасывается."));
        return false;
    }
    step_series_ = std::move(*opt);
    valve_series_.clear();
    signal_series_.clear();
    return true;
}

bool IdTab::load_valve_signal_file(const QString& path) {
    const auto numbers = data_file_parser::extractNumbersFromFile(path);
    if (numbers.size() < 6) {
        show_error(tr("Файл слишком короткий для time, valve, value "
                      "(найдено чисел: %1).")
                       .arg(numbers.size()));
        return false;
    }
    data_file_parser::VecPair valve, signal;
    if (!data_file_parser::asValveSignal(numbers, valve, signal)) {
        show_error(tr("Не удалось разобрать тройки time, valve, value.\n"
                      "Чисел: %1 (нужно кратно 3, минимум 6).")
                       .arg(numbers.size()));
        return false;
    }
    valve_series_  = std::move(valve);
    signal_series_ = std::move(signal);
    step_series_.clear();
    return true;
}

void IdTab::runIdentification() {
    if (file_path_.isEmpty()) {
        show_error(tr("Сначала выберите файл с экспериментальными данными."));
        return;
    }

    const auto method = static_cast<Method>(ui->methodCombo->currentIndex());
    const bool loaded =
        (method == Method::StepResponse) ? load_step_file(file_path_) : load_valve_signal_file(file_path_);
    if (!loaded)
        return;
    has_data_ = true;

    try {
        numina::SimoyuIdentifier simoyu;
        Series experimental_h;
        const bool auto_order   = id_settings_.autoOrder;
        const std::size_t den_n = static_cast<std::size_t>(std::clamp(id_settings_.denOrder, 1, 6));
        const std::size_t num_m =
            static_cast<std::size_t>(std::clamp(id_settings_.numOrder, 0, static_cast<int>(den_n)));
        // Plant structure order is independent of Padé approxOrder (model settings).
        const std::size_t max_order = static_cast<std::size_t>(std::clamp(id_settings_.maxAutoOrder, 2, 6));
        const bool want_tau         = id_settings_.estimateTau;

        if (method == Method::StepResponse) {
            if (step_series_.size() < 2) {
                show_error(tr("Недостаточно точек переходной характеристики."));
                return;
            }
            experimental_h = step_series_;
        }
        else {
            if (valve_series_.size() < 2 || signal_series_.size() < 2) {
                show_error(tr("Недостаточно точек клапана/сигнала (строк: %1).").arg(valve_series_.size()));
                return;
            }
            const std::size_t n_valve = valve_series_.size();
            for (std::size_t i = 1; i < n_valve; ++i) {
                if (valve_series_[i].first < valve_series_[i - 1].first) {
                    show_error(tr("Время должно быть неубывающим (нарушение около точки %1).").arg(i));
                    return;
                }
            }
            numina::DuhamelSolver duhamel;
            experimental_h = duhamel.stepResponse(valve_series_, signal_series_);
            if (experimental_h.size() < 2) {
                show_error(tr("Дюамель не восстановил h(t) (точек: %1).\n"
                              "Проверьте, что u(t) меняется и y(t) согласован по времени.")
                               .arg(experimental_h.size()));
                return;
            }
            step_series_ = experimental_h;
        }

        numina::SimoyuIdentifier::DelayFit fit;
        if (want_tau) {
            if (auto_order)
                fit = simoyu.identifyDelayAuto(experimental_h, max_order, max_order);
            else
                fit = simoyu.identifyDelay(experimental_h, den_n, num_m);
        }
        else {
            fit.tau = 0.0;
            if (auto_order)
                fit.plant = simoyu.identifyAuto(experimental_h, max_order, max_order);
            else
                fit.plant = simoyu.identify(experimental_h, den_n, num_m);
        }

        if (!fit.isOk()) {
            show_error(tr("Идентификация не дала модели (deg D < 1).\n"
                          "Точек h(t): %1. Проверьте данные и метод.")
                           .arg(experimental_h.size()));
            return;
        }

        const double tau = fit.tau > 0.0 ? fit.tau : 0.0;
        apply_result(fit.plant, tau, experimental_h);
        ui->fileLabel->setText(QFileInfo(file_path_).fileName());
    }
    catch (const std::exception& ex) {
        show_error(tr("Ошибка идентификации: %1").arg(QString::fromUtf8(ex.what())));
    }
}

void IdTab::apply_result(const numina::TransferFunction& plant, double tau, const Series& experimental_h) {
    numina::TransferFunction model = plant;
    if (tau > 0.0)
        model *= numina::TransferFunction::makeDelay(tau, static_cast<std::uint8_t>(model_param_.approxOrder));

    display_->setTransferFunction(plant, tau);

    ui->charts->clearAll();
    ui->charts->appendTransientCurve(experimental_h, tr("Эксперимент"));
    ui->charts->appendFromTf(model, model_param_, tr("Модель"));
    tab_ui::applySettledPlantMetrics(metrics_, ui->charts);
}
