#pragma once

#include "code/charts/response-chart-bank.h"
#include "code/dialogs/mod-par-dialog.h"
#include "code/model/model-param.hpp"
#include "code/util/tf-builder.hpp"
#include "code/widgets/regulation-widget.h"

#include <QBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

namespace tab_ui {
inline void wireChartsButton(QToolButton* btn, ResponseChartBank* charts, QMenu* menu) {
    QObject::connect(menu, &QMenu::aboutToShow, btn, [charts, menu] { charts->populateMenu(menu); });
    btn->setMenu(menu);
}

inline void mountInHost(QWidget* host, QWidget* child, Qt::Alignment align) {
    QLayout* layout = host->layout();
    if (!layout) {
        auto* box = new QVBoxLayout(host);
        box->setContentsMargins(0, 0, 0, 0);
        layout = box;
    }
    layout->addWidget(child);
    if (auto* box = qobject_cast<QBoxLayout*>(layout))
        box->setAlignment(child, align);
}

inline void showError(QWidget* parent, const QString& title, const QString& message) {
    QMessageBox::critical(parent, title, message);
}

[[nodiscard]] inline QString plantInputError(const std::vector<double>& num, const std::vector<double>& den) {
    if (tf_builder::validInput(num, den))
        return {};
    if (den.empty())
        return QObject::tr("Знаменатель НЕ может быть равен нулю!");
    if (den.size() == 1)
        return QObject::tr("Порядок знаменателя НЕ может быть меньше первого!");
    return QObject::tr("Порядок числителя НЕ может быть больше порядка знаменателя!");
}

[[nodiscard]] inline bool editModelParam(QWidget* parent, ModelParam& param) {
    ModParDialog dialog(param, parent);
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
