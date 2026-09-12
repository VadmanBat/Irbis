#include "irbis/dialogs/tran-func-dialog.h"

#include "irbis/util/dialog-icons.hxx"
#include "irbis/util/format.hxx"
#include "irbis/widgets/formula-view.h"
#include "irbis/widgets/tf-display-widget.h"
#include "irbis/widgets/tf-h-scroll.h"
#include "numina/classes/calculus/laplace-solution.h"
#include "ui_tran-func-dialog.h"

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QMenu>
#include <QPushButton>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QToolTip>
#include <QWidget>
#include <utility>

namespace {
using TfFormat = numina::TransferFunction::Format;

QString shift_time_variable(QString text, const QString& t_sub) {
    text.replace(QStringLiteral(" t"), QStringLiteral(" ") + t_sub);
    return text;
}

QString apply_exact_delay(QString body, double tau, TfFormat format) {
    if (!(tau > 0.0))
        return body;
    const QString tau_s = num_format::format(tau);
    switch (format) {
        case TfFormat::Html: {
            const QString t_sub = QStringLiteral("(t−%1)").arg(tau_s);
            return QStringLiteral("1(t−%1)&nbsp;&middot;&nbsp;(%2)").arg(tau_s, shift_time_variable(body, t_sub));
        }
        case TfFormat::Plain: {
            const QString t_sub = QStringLiteral("(t-%1)").arg(tau_s);
            return QStringLiteral("1(t-%1) * (%2)").arg(tau_s, shift_time_variable(body, t_sub));
        }
        case TfFormat::Latex: {
            const QString t_sub = QStringLiteral("(t-%1)").arg(tau_s);
            return QStringLiteral("1(t-%1)\\,(%2)").arg(tau_s, shift_time_variable(body, t_sub));
        }
    }
    return body;
}

QString solution_text(const numina::LaplaceSolution& sol, TfFormat format, double delay_tau) {
    QString body;
    switch (format) {
        case TfFormat::Html:
            body = QString::fromStdString(sol.htmlString());
            break;
        case TfFormat::Plain:
            body = QString::fromStdString(sol.plainString());
            break;
        case TfFormat::Latex:
            body = QString::fromStdString(sol.latexString());
            break;
    }
    return apply_exact_delay(std::move(body), delay_tau, format);
}

QString as_html(QString text) {
    if (!text.contains(QStringLiteral("<br")) && text.contains(QLatin1Char('\n')))
        text.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return text;
}

QString with_lhs_html(const QString& lhs, const QString& rhs) {
    return lhs + QStringLiteral("&nbsp;=&nbsp;") + rhs;
}

enum class DeKind { HighOrder, FirstOrder, Euler };

QString de_text(const numina::TransferFunction& tf, DeKind kind, TfFormat format) {
    std::string raw;
    switch (kind) {
        case DeKind::HighOrder:
            raw = tf.equation(format);
            break;
        case DeKind::FirstOrder:
            raw = tf.firstOrderEquations(format);
            break;
        case DeKind::Euler:
            raw = tf.implicitEulerEquations(format);
            break;
    }
    QString text = QString::fromStdString(raw);
    return format == TfFormat::Html ? as_html(std::move(text)) : text;
}

}

TranFuncDialog::TranFuncDialog(const numina::TransferFunction& tf, QWidget* parent, double delayTau)
    : QDialog(parent), ui(new Ui::TranFuncDialog), tf_(tf), delay_tau_(delayTau) {
    ui->setupUi(this);
    setObjectName(QStringLiteral("TranFuncDialog"));
    dialog_icons::apply(this, dialog_icons::Kind::TransferFunction);

    auto paint_bg = [](QWidget* w) {
        w->setAttribute(Qt::WA_StyledBackground, true);
        w->style()->unpolish(w);
        w->style()->polish(w);
    };
    paint_bg(ui->headerCard);
    paint_bg(ui->polesCard);
    paint_bg(ui->solutionsCard);
    paint_bg(ui->deCard);
    paint_bg(ui->formulaHost);
    ui->formulaHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    ui->formulaHost->setMinimumWidth(0);
    if (auto* box = qobject_cast<QBoxLayout*>(ui->formulaHost->layout()))
        TfHScroll::wrapInLayout(ui->formula, box);
    paint_bg(ui->htBlock);
    paint_bg(ui->wtBlock);
    paint_bg(ui->odeBlock);
    paint_bg(ui->firstOrderBlock);
    paint_bg(ui->eulerBlock);
    ui->polesTable->setAttribute(Qt::WA_StyledBackground, true);

    connect(ui->deToggle, &QToolButton::toggled, this, [this](bool on) {
        ui->deToggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        setUpdatesEnabled(false);
        ui->deBody->setMaximumHeight(on ? QWIDGETSIZE_MAX : 0);
        setUpdatesEnabled(true);
    });

    fill_formula();
    fill_poles();
    show_solutions();
    setup_copy_menus();

    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
}

TranFuncDialog::~TranFuncDialog() {
    delete ui;
}

void TranFuncDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    fit_poles_table();
}

void TranFuncDialog::fill_formula() {
    ui->formula->setTransferFunction(tf_, delay_tau_);
}

void TranFuncDialog::show_solutions() {
    ui->htView->setHtml(
        with_lhs_html(QStringLiteral("h(t)"), solution_text(tf_.transientSolution(), TfFormat::Html, delay_tau_)));
    ui->wtView->setHtml(
        with_lhs_html(QStringLiteral("w(t)"), solution_text(tf_.impulseSolution(), TfFormat::Html, delay_tau_)));
    ui->odeView->setHtml(de_text(tf_, DeKind::HighOrder, TfFormat::Html));
    ui->firstOrderView->setHtml(de_text(tf_, DeKind::FirstOrder, TfFormat::Html));
    ui->eulerView->setHtml(de_text(tf_, DeKind::Euler, TfFormat::Html));
}

void TranFuncDialog::setup_copy_menus() {
    enum class Kind { Transient, Impulse, Ode, FirstOrder, Euler };

    auto wire = [this](QToolButton* button, Kind kind) {
        auto* menu = new QMenu(button);

        const auto add = [&](const QString& title, TfFormat format) {
            auto* act = menu->addAction(title);
            connect(act, &QAction::triggered, this, [this, button, kind, format] {
                QString text;
                switch (kind) {
                    case Kind::Transient:
                        text = solution_text(tf_.transientSolution(), format, delay_tau_);
                        break;
                    case Kind::Impulse:
                        text = solution_text(tf_.impulseSolution(), format, delay_tau_);
                        break;
                    case Kind::Ode:
                        text = de_text(tf_, DeKind::HighOrder, format);
                        break;
                    case Kind::FirstOrder:
                        text = de_text(tf_, DeKind::FirstOrder, format);
                        break;
                    case Kind::Euler:
                        text = de_text(tf_, DeKind::Euler, format);
                        break;
                }
                copy_solution_text(text, button);
            });
        };

        add(tr("Обычный текст"), TfFormat::Plain);
        add(tr("LaTeX"), TfFormat::Latex);
        add(tr("HTML"), TfFormat::Html);
        button->setMenu(menu);
    };

    wire(ui->htCopyButton, Kind::Transient);
    wire(ui->wtCopyButton, Kind::Impulse);
    wire(ui->odeCopyButton, Kind::Ode);
    wire(ui->firstOrderCopyButton, Kind::FirstOrder);
    wire(ui->eulerCopyButton, Kind::Euler);
}

void TranFuncDialog::copy_solution_text(const QString& text, QWidget* anchor) {
    QApplication::clipboard()->setText(text);
    if (anchor) {
        QToolTip::showText(anchor->mapToGlobal(QPoint(0, anchor->height())), tr("Скопировано"), anchor, QRect(), 1500);
    }
}
