#include "code/tabs/id-tab.h"
#include "code/util/data-file-parser.hpp"
#include "numina/classes/control/identification/duhamel-solver.h"
#include "ui_id-tab.h"

#include <exception>
#include <QFileInfo>
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

bool IdTab::preview_loaded_file() {
    const auto method = static_cast<Method>(ui->methodCombo->currentIndex());
    const bool loaded =
        (method == Method::StepResponse) ? load_step_file(file_path_) : load_valve_signal_file(file_path_);
    if (!loaded)
        return false;

    if (method == Method::StepResponse) {
        if (step_series_.size() < 2) {
            show_error(tr("Недостаточно точек переходной характеристики."));
            return false;
        }
    }
    else {
        if (valve_series_.size() < 2 || signal_series_.size() < 2) {
            show_error(tr("Недостаточно точек клапана/сигнала (строк: %1).").arg(valve_series_.size()));
            return false;
        }
        const std::size_t n_valve = valve_series_.size();
        for (std::size_t i = 1; i < n_valve; ++i) {
            if (valve_series_[i].first < valve_series_[i - 1].first) {
                show_error(tr("Время должно быть неубывающим (нарушение около точки %1).").arg(i));
                return false;
            }
        }
    }

    has_data_ = true;
    display_->clear();
    maybe_show_structure_template();
    show_file_preview();
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
    if (!preview_loaded_file())
        return;

    const auto method   = static_cast<Method>(ui->methodCombo->currentIndex());
    const auto settings = read_id_settings();

    try {
        if (settings.plantKind == IdSettings::PlantKind::Astatic) {
            run_astatic(method);
        }
        else {
            Series experimental_h;
            if (method == Method::StepResponse) {
                experimental_h = step_series_;
            }
            else {
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
            run_static(experimental_h);
        }

        ui->fileLabel->setText(QFileInfo(file_path_).fileName());
    }
    catch (const std::exception& ex) {
        show_error(tr("Ошибка идентификации: %1").arg(QString::fromUtf8(ex.what())));
    }
}
