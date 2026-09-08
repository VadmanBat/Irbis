#include "irbis/charts/response-chart-bank.h"

#include <QAction>
#include <QGridLayout>
#include <QMenu>

ResponseChartBank::ResponseChartBank(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("ResponseChartBank"));
    grid_ = new QGridLayout(this);
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setSpacing(6);

    chart_tran_    = new ChartPanel(tr("Переходная характеристика h(t)"), tr("t, с"), tr("h(t)"), this);
    chart_impulse_ = new ChartPanel(tr("Импульсная характеристика w(t)"), tr("t, с"), tr("w(t)"), this);
    chart_nyquist_ = new ChartPanel(tr("КЧХ (годограф Найквиста)"), tr("Re W(jω)"), tr("Im W(jω)"), this);
    chart_amp_     = new ChartPanel(tr("АЧХ |W(jω)|"), tr("ω, рад/с"), tr("|W|"), this);
    chart_phase_   = new ChartPanel(tr("ФЧХ arg(W(jω))"), tr("ω, рад/с"), tr("φ, °"), this);

    rebuild_layout();
}

void ResponseChartBank::setTransientTitle(const QString& title) {
    chart_tran_->setChartTitle(title);
}

void ResponseChartBank::populateMenu(QMenu* menu) {
    if (!menu)
        return;
    menu->clear();

    auto add = [this, menu](const QString& title, bool ChartVisibility::* field) {
        auto* act = menu->addAction(title);
        act->setCheckable(true);
        act->setChecked(vis_.*field);
        connect(act, &QAction::toggled, this, [this, field, act](bool on) {
            ChartVisibility next = vis_;
            next.*field          = on;
            if (next.count() == 0) {
                act->blockSignals(true);
                act->setChecked(true);
                act->blockSignals(false);
                return;
            }
            setVisibility(next);
        });
    };

    add(tr("Переходная"), &ChartVisibility::transient);
    add(tr("Импульсная"), &ChartVisibility::impulse);
    add(tr("КЧХ"), &ChartVisibility::nyquist);
    add(tr("ФЧХ"), &ChartVisibility::phase);
    add(tr("АЧХ"), &ChartVisibility::amplitude);
}

void ResponseChartBank::setVisibility(ChartVisibility vis) {
    if (vis.count() == 0)
        vis.transient = true;
    vis_ = vis;
    rebuild_layout();
    for (Batch& batch : history_)
        ensure_visible_channels(batch);
    rebuild_bounds_from_history();
    ensure_visible_series();
    refit_all();
}

void ResponseChartBank::rebuild_layout() {
    while (QLayoutItem* item = grid_->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->setParent(this);
        delete item;
    }
    for (int r = 0; r < 4; ++r)
        grid_->setRowStretch(r, 0);
    for (int c = 0; c < 3; ++c)
        grid_->setColumnStretch(c, 0);

    ChartPanel* panels[] = {chart_tran_, chart_impulse_, chart_nyquist_, chart_amp_, chart_phase_};
    const bool flags[]   = {vis_.transient, vis_.impulse, vis_.nyquist, vis_.amplitude, vis_.phase};

    ChartPanel* visible[5];
    int n = 0;
    for (int i = 0; i < 5; ++i) {
        panels[i]->setVisible(flags[i]);
        if (flags[i])
            visible[n++] = panels[i];
    }
    if (n == 0)
        return;

    switch (n) {
        case 1:
            grid_->addWidget(visible[0], 0, 0);
            grid_->setRowStretch(0, 1);
            grid_->setColumnStretch(0, 1);
            break;
        case 2:
            grid_->addWidget(visible[0], 0, 0);
            grid_->addWidget(visible[1], 0, 1);
            grid_->setRowStretch(0, 1);
            grid_->setColumnStretch(0, 1);
            grid_->setColumnStretch(1, 1);
            break;
        case 3:
            grid_->addWidget(visible[0], 0, 0);
            grid_->addWidget(visible[1], 0, 1);
            grid_->addWidget(visible[2], 0, 2);
            grid_->setRowStretch(0, 1);
            grid_->setColumnStretch(0, 1);
            grid_->setColumnStretch(1, 1);
            grid_->setColumnStretch(2, 1);
            break;
        case 4:
            grid_->addWidget(visible[0], 0, 0);
            grid_->addWidget(visible[1], 0, 1);
            grid_->addWidget(visible[2], 1, 0);
            grid_->addWidget(visible[3], 1, 1);
            grid_->setRowStretch(0, 1);
            grid_->setRowStretch(1, 1);
            grid_->setColumnStretch(0, 1);
            grid_->setColumnStretch(1, 1);
            break;
        default:
            grid_->addWidget(visible[0], 0, 0);
            grid_->addWidget(visible[1], 0, 1);
            grid_->addWidget(visible[2], 1, 0);
            grid_->addWidget(visible[3], 1, 1);
            grid_->addWidget(visible[4], 1, 2);
            grid_->setRowStretch(0, 1);
            grid_->setRowStretch(1, 1);
            grid_->setColumnStretch(0, 1);
            grid_->setColumnStretch(1, 1);
            grid_->setColumnStretch(2, 1);
            break;
    }
}
