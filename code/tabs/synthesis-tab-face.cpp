#include "code/tabs/synthesis-tab.h"
#include "code/util/format.hxx"
#include "ui_synthesis-tab.h"

#include <cmath>
#include <QColor>
#include <QEvent>
#include <QSignalBlocker>
#include <QStackedWidget>

namespace {
QString html_cell(const QString& html) {
    return QStringLiteral("<td valign='middle' align='center'>%1</td>").arg(html);
}

QString html_frac(const QString& num, const QString& den, const QString& bar) {
    return QStringLiteral(
               "<table cellspacing='0' cellpadding='1'>"
               "<tr><td align='center' style='padding:0 4px 2px 4px'>%1</td></tr>"
               "<tr><td align='center' style='border-top:1px solid %3;padding:2px 4px 0 4px'>%2</td></tr>"
               "</table>")
        .arg(num, den, bar);
}

QString html_row(const QString& cells) {
    return QStringLiteral("<table cellspacing='0' cellpadding='2'><tr>%1</tr></table>").arg(cells);
}

QString html_center(const QString& inner) {
    return QStringLiteral(
               "<table width='100%' cellspacing='0' cellpadding='0'>"
               "<tr><td align='center'>%1</td></tr></table>")
        .arg(inner);
}

QString wr_eq(const QString& rhs) {
    return html_center(html_row(html_cell(QStringLiteral("W<sub>р</sub>(p) = ")) + html_cell(rhs)));
}

QString wr_eq_frac(const QString& num, const QString& den, const QString& bar) {
    return wr_eq(html_frac(num, den, bar));
}

QString face_ink(const QWidget* w) {
    return w->palette().color(w->foregroundRole()).name(QColor::HexRgb);
}
}

void SynthesisTab::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (!ui || parameters_.empty())
        return;
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
        update_regulator_face();
}

bool SynthesisTab::is_pi_structure() const noexcept {
    return parameters_[0]->enabled() && parameters_[1]->enabled() && !parameters_[2]->enabled();
}

void SynthesisTab::update_c0c1_visibility() {
    const bool pi    = is_pi_structure();
    const bool plane = pi && show_plane_;
    ui->viewPlaneButton->setVisible(pi);
    ui->viewFaceButton->setVisible(pi);
    ui->viewPlaneButton->setEnabled(pi);
    {
        const QSignalBlocker b1(ui->viewPlaneButton);
        const QSignalBlocker b2(ui->viewFaceButton);
        ui->viewPlaneButton->setChecked(plane);
        ui->viewFaceButton->setChecked(!plane);
    }
    ui->c0c1Stack->setCurrentWidget(plane ? static_cast<QWidget*>(ui->c0c1Chart)
                                          : static_cast<QWidget*>(ui->regulatorFace));
    update_regulator_face();
}

void SynthesisTab::update_regulator_face() {
    const bool p_on = parameters_[0]->enabled();
    const bool i_on = parameters_[1]->enabled();
    const bool d_on = parameters_[2]->enabled();
    const double kp = parameters_[0]->value();
    const double ti = parameters_[1]->value();
    const double td = parameters_[2]->value();
    const int id    = static_cast<int>(p_on) + 2 * static_cast<int>(i_on) + 4 * static_cast<int>(d_on);

    static const char* k_law[] = {"не выбран", "П", "И", "ПИ", "Д", "ПД", "ИД", "ПИД"};
    ui->faceLawLabel->setText(tr("Закон: %1").arg(QString::fromUtf8(k_law[id])));

    const QString bar = face_ink(ui->faceFormulaLabel);
    switch (id) {
        case 1:
            ui->faceFormulaLabel->setText(wr_eq(QStringLiteral("K<sub>p</sub>")));
            break;
        case 2:
            ui->faceFormulaLabel->setText(wr_eq_frac(QStringLiteral("1"), QStringLiteral("T<sub>i</sub> p"), bar));
            break;
        case 3:
            ui->faceFormulaLabel->setText(
                html_center(html_row(html_cell(QStringLiteral("W<sub>р</sub>(p) = K<sub>p</sub>(1 + ")) +
                                     html_cell(html_frac(QStringLiteral("1"), QStringLiteral("T<sub>i</sub> p"), bar)) +
                                     html_cell(QStringLiteral(")")))));
            break;
        case 4:
            ui->faceFormulaLabel->setText(wr_eq(QStringLiteral("T<sub>d</sub> p")));
            break;
        case 5:
            ui->faceFormulaLabel->setText(wr_eq(QStringLiteral("K<sub>p</sub>(1 + T<sub>d</sub> p)")));
            break;
        case 6:
            ui->faceFormulaLabel->setText(
                html_center(html_row(html_cell(QStringLiteral("W<sub>р</sub>(p) = ")) +
                                     html_cell(html_frac(QStringLiteral("1"), QStringLiteral("T<sub>i</sub> p"), bar)) +
                                     html_cell(QStringLiteral(" + T<sub>d</sub> p")))));
            break;
        case 7:
            ui->faceFormulaLabel->setText(
                html_center(html_row(html_cell(QStringLiteral("W<sub>р</sub>(p) = K<sub>p</sub>(1 + ")) +
                                     html_cell(html_frac(QStringLiteral("1"), QStringLiteral("T<sub>i</sub> p"), bar)) +
                                     html_cell(QStringLiteral(" + T<sub>d</sub> p)")))));
            break;
        default:
            ui->faceFormulaLabel->setText(wr_eq(QStringLiteral("1")));
            break;
    }

    const bool c0_on = i_on && ti > 0.0;
    const bool c1_on = p_on;
    const bool c2_on = d_on;

    QStringList num_terms;
    if (c0_on)
        num_terms << QStringLiteral("C<sub>0</sub>");
    if (c1_on)
        num_terms << QStringLiteral("C<sub>1</sub> p");
    if (c2_on)
        num_terms << QStringLiteral("C<sub>2</sub> p<sup>2</sup>");

    if (num_terms.isEmpty()) {
        ui->facePolyLabel->clear();
        ui->facePolyLabel->hide();
    }
    else {
        ui->facePolyLabel->show();
        if (i_on)
            ui->facePolyLabel->setText(
                wr_eq_frac(num_terms.join(QStringLiteral(" + ")), QStringLiteral("p"), face_ink(ui->facePolyLabel)));
        else {
            QStringList simp;
            if (c1_on)
                simp << QStringLiteral("C<sub>1</sub>");
            if (c2_on)
                simp << QStringLiteral("C<sub>2</sub> p");
            ui->facePolyLabel->setText(wr_eq(simp.join(QStringLiteral(" + "))));
        }
    }

    auto coeff = [](bool on, double v) { return on && std::isfinite(v) ? num_format::format(v) : QStringLiteral("—"); };
    ui->faceC0Value->setText(coeff(c0_on, (p_on ? kp : 1.0) / ti));
    ui->faceC1Value->setText(coeff(c1_on, kp));
    ui->faceC2Value->setText(coeff(c2_on, (p_on ? kp : 1.0) * td));
}
