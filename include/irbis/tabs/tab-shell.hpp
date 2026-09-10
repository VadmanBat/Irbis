#pragma once

#include "irbis/charts/response-chart-bank.h"
#include "irbis/dialogs/chart-vis-dialog.h"
#include "irbis/dialogs/help-dialog.h"
#include "irbis/dialogs/mod-par-dialog.h"
#include "irbis/model/model-param.hpp"
#include "irbis/style.hpp"
#include "irbis/util/tf-builder.hpp"
#include "irbis/widgets/regulation-widget.h"

#include <QBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

namespace tab_ui {
inline void ensureFonts() {
    irbis::loadFonts();
}

inline bool editChartVisibility(QWidget* parent, ResponseChartBank* charts) {
    if (!charts)
        return false;
    ChartVisDialog dialog(charts->visibility(), parent);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    charts->setVisibility(dialog.data());
    return true;
}

inline void mountInHost(QWidget* host, QWidget* child, Qt::Alignment align, int stretch = 0) {
    QLayout* layout = host->layout();
    if (!layout) {
        auto* box = new QVBoxLayout(host);
        box->setContentsMargins(0, 0, 0, 0);
        layout = box;
    }
    if (auto* box = qobject_cast<QBoxLayout*>(layout)) {
        box->addWidget(child, stretch);
        if (align)
            box->setAlignment(child, align);
    }
    else {
        layout->addWidget(child);
    }
}

inline void showError(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::critical(parent, title, message);
}

inline void showHelp(QWidget* parent, HelpDialog::Topic topic) {
    HelpDialog dialog(topic, parent);
    dialog.exec();
}

[[nodiscard]] inline QString plantInputError(const std::vector<double>& num, const std::vector<double>& den) {
    if (tf_builder::validInput(num, den))
        return {};
    bool num_zero = num.empty();
    if (!num_zero) {
        num_zero = true;
        for (double c : num) {
            if (c != 0.0) {
                num_zero = false;
                break;
            }
        }
    }
    if (num_zero)
        return QObject::tr("Числитель НЕ может быть равен нулю!");
    if (den.empty())
        return QObject::tr("Знаменатель НЕ может быть равен нулю!");
    if (den.size() == 1)
        return QObject::tr("Порядок знаменателя НЕ может быть меньше первого!");
    return QObject::tr("Порядок числителя НЕ может быть больше порядка знаменателя!");
}

[[nodiscard]] inline bool editModelParam(QWidget* parent, ModelParam& param, bool allowIdealDelay = false) {
    ModParDialog dialog(param, parent, allowIdealDelay);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    param = dialog.data();
    return true;
}

inline void setupPlantQualityMetrics(RegulationWidget* metrics) {
    metrics->setLabels(
        {
            QStringLiteral("t<sub>р</sub>:"),
            QStringLiteral("ω<sub>n</sub>:"),
            QStringLiteral("t<sub>н</sub>:"),
            QStringLiteral("ω<sub>c</sub>:"),
            QStringLiteral("ζ:"),
            QStringLiteral("h<sub>уст</sub>:"),
        },
        {
            QObject::tr("Время регулирования, с"),
            QObject::tr("Собственная частота, рад/с"),
            QObject::tr("Время нарастания, с"),
            QObject::tr("Частота среза, рад/с"),
            QObject::tr("Коэффициент демпфирования, %"),
            QObject::tr("Установившееся значение"),
        });
    metrics->setColors({{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}});
}

inline void applySettledPlantMetrics(RegulationWidget* metrics, ResponseChartBank* charts) {
    if (!charts->hasLastQuality() || !charts->lastQuality().is_settled) {
        metrics->updateValues({});
        return;
    }
    const auto& q = charts->lastQuality();
    metrics->updateValues(
        {q.settling_time, q.natural_frequency, q.rise_time, q.cut_frequency, q.damping_ratio, q.steady_state});
}
}
