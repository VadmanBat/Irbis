#include "irbis/control/controller-design.hpp"
#include "irbis/tabs/synthesis-tab.h"
#include "ui_synthesis-tab.h"

#include <exception>
#include <QApplication>
#include <QObject>
#include <vector>

namespace {
C0C1Chart::Optimum to_optimum(const numina::ControllerDesigner::Design& d, const QString& label) {
    C0C1Chart::Optimum o;
    o.valid = d.isOk();
    o.c0    = d.settings.c0;
    o.c1    = d.settings.c1;
    o.c2    = d.settings.c2;
    o.kp    = d.settings.kp;
    o.ti    = d.settings.ti;
    o.td    = d.settings.td;
    o.omega = d.settings.omega;
    o.label = label;
    return o;
}

C0C1Chart::Sample to_sample(const numina::ControllerDesigner::Settings& s) {
    C0C1Chart::Sample sample;
    sample.omega = s.omega;
    sample.c0    = s.c0;
    sample.c1    = s.c1;
    sample.c2    = s.c2;
    sample.kp    = s.kp;
    sample.ti    = s.ti;
    sample.td    = s.td;
    return sample;
}

QString design_fail(const numina::ControllerDesigner::Law law) {
    using L = numina::ControllerDesigner::Law;
    switch (law) {
        case L::P:
            return QObject::tr("П недоступен: нет вершины ω_П (C₀=0, C₁>0).");
        case L::I:
            return QObject::tr("И недоступен: нет вершины ω_И (C₁=0, C₀>0).");
        case L::Pd:
            return QObject::tr("ПД недоступен: нет полосы C₁>0, C₂>0 с доминированием.");
        case L::Pid:
            return QObject::tr("ПИД недоступен: нет настроек с C₂>0.\n"
                               "Если ПИ уже обеспечивает φ, выберите ПИ или Авто.");
        case L::Pi:
            return QObject::tr("ПИ недоступен (C₀, C₁ > 0 и доминирование).\n"
                               "При пустой Ω_доп выберите ПИД или Авто.");
        default:
            return QObject::tr("Автонастройка не нашла ни ПИ, ни ПИД.");
    }
}
}

void SynthesisTab::autoSynthesize() {
    numina::TransferFunction plant;
    if (!build_plant(plant)) {
        show_error(tr("Задайте корректную ПФ объекта управления."));
        return;
    }

    try {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        const double phi = ui->phiSpin->value() / 100.0;
        numina::ControllerDesigner designer(plant, phi);

        numina::ControllerDesigner::Spec spec;
        spec.phi = phi;
        spec.law = selected_law();

        using L           = numina::ControllerDesigner::Law;
        const auto bundle = controller_design::synthesize(designer, spec, selected_criterion());
        ui->c0c1Chart->setPlane(spec.law == L::Pd ? C0C1Chart::Plane::Pd : C0C1Chart::Plane::Pi);

        if (controller_design::drawsRegion(spec.law)) {
            const auto loc = controller_design::locus(designer, spec.law);
            if (loc.empty() && !bundle.face) {
                QApplication::restoreOverrideCursor();
                if (spec.law == L::Pd)
                    show_error(tr("Нет области C₁>0, C₂>0 при φ = %1 %.\n"
                                  "ПД недоступен (полоса пуста). Выберите другой закон.")
                                   .arg(ui->phiSpin->value(), 0, 'f', 2));
                else
                    show_error(tr("Нет области C₀>0, C₁>0 при φ = %1 %.\n"
                                  "ПИ недоступен (Ω_доп пуста). Выберите ПИД или Авто.")
                                   .arg(ui->phiSpin->value(), 0, 'f', 2));
                return;
            }
            std::vector<C0C1Chart::Sample> samples;
            samples.reserve(loc.size());
            for (const auto& s : loc)
                samples.push_back(to_sample(s));
            QString loc_name = tr("ЛРЗ");
            if (bundle.face)
                loc_name = tr("ЛРЗ (грань C₂*)");
            else if (spec.law == L::Pd)
                loc_name = tr("ЛРЗ (ПД)");
            ui->c0c1Chart->setLocus(std::move(samples), loc_name);
            ui->c0c1Chart->setOptima(to_optimum(bundle.lik, tr("опт. ЛИК")), to_optimum(bundle.ikk, tr("опт. ИКК")),
                                     to_optimum(bundle.sko, tr("опт. СКО")));
        }
        else {
            ui->c0c1Chart->setLocus({}, {});
            ui->c0c1Chart->setOptima({}, {}, {});
        }

        QApplication::restoreOverrideCursor();

        if (!bundle.selected().isOk()) {
            show_error(design_fail(spec.law));
            return;
        }

        apply_design(bundle.selected(), !ui->charts->empty());
        ui->c0c1Chart->requestRefit();
    }
    catch (const std::exception& ex) {
        QApplication::restoreOverrideCursor();
        show_error(tr("Автосинтез: %1").arg(QString::fromUtf8(ex.what())));
    }
}
