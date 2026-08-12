#pragma once

#include "code/charts/response-chart-bank.h"

#include <QMenu>
#include <QToolButton>
#include <QWidget>

namespace tab_ui {
/// Shared charts-visibility tool button for Analysis / Synthesis / Id tabs.
[[nodiscard]] inline QToolButton* makeChartsButton(QWidget* parent, ResponseChartBank* charts, QMenu* menu) {
    QObject::connect(menu, &QMenu::aboutToShow, parent, [charts, menu] { charts->populateMenu(menu); });

    auto* btn = new QToolButton(parent);
    btn->setObjectName(QStringLiteral("chartsButton"));
    btn->setText(QStringLiteral("◫"));
    btn->setToolTip(QObject::tr("Отображаемые графики"));
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setMenu(menu);
    btn->setFixedSize(40, 40);
    return btn;
}
} // namespace tab_ui
