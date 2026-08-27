#include "code/dialogs/tran-func-dialog.h"

#include "code/util/dialog-icons.hxx"
#include "code/util/format.hxx"
#include "code/widgets/formula-view.h"
#include "code/widgets/tf-display-widget.h"
#include "numina/classes/calculus/laplace-solution.h"
#include "ui_tran-func-dialog.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHeaderView>
#include <QLayout>
#include <QMenu>
#include <QShowEvent>
#include <QStyle>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
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

FormulaView* mount_formula(QWidget* host) {
    auto* lay = new QVBoxLayout(host);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    auto* view = new FormulaView(host);
    lay->addWidget(view);
    return view;
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
    paint_bg(ui->htBlock);
    paint_bg(ui->wtBlock);
    paint_bg(ui->odeBlock);
    paint_bg(ui->firstOrderBlock);
    paint_bg(ui->eulerBlock);

    auto* formula_lay = new QVBoxLayout(ui->formulaHost);
    formula_lay->setContentsMargins(12, 10, 12, 10);
    formula_lay->setSpacing(0);
    formula_ = new TfDisplayWidget(QStringLiteral("W(p) = "), ui->formulaHost);
    formula_lay->addWidget(formula_);

    ht_    = mount_formula(ui->htHost);
    wt_    = mount_formula(ui->wtHost);
    ode_   = mount_formula(ui->odeHost);
    first_ = mount_formula(ui->firstOrderHost);
    euler_ = mount_formula(ui->eulerHost);

    ui->bodyScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->bodyScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    ui->polesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->polesTable->verticalHeader()->setVisible(false);
    ui->polesTable->setFrameShape(QFrame::NoFrame);
    ui->polesTable->setAttribute(Qt::WA_StyledBackground, true);
    ui->polesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->polesTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->polesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->polesTable->horizontalHeader()->setStretchLastSection(true);
    ui->polesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->polesTable->horizontalHeader()->setTextElideMode(Qt::ElideRight);

    layout()->setSizeConstraint(QLayout::SetNoConstraint);
    ui->bodyScroll->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    ui->bodyContents->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    ui->deToggle->setArrowType(Qt::RightArrow);
    ui->deToggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->deToggle->setFocusPolicy(Qt::TabFocus);
    ui->deCardLayout->setSpacing(0);
    ui->deBody->show();
    ui->deBody->setMaximumHeight(0);
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
    formula_->setTransferFunction(tf_, delay_tau_);
}

void TranFuncDialog::show_solutions() {
    ht_->setHtml(
        with_lhs_html(QStringLiteral("h(t)"), solution_text(tf_.transientSolution(), TfFormat::Html, delay_tau_)));
    wt_->setHtml(
        with_lhs_html(QStringLiteral("w(t)"), solution_text(tf_.impulseSolution(), TfFormat::Html, delay_tau_)));
    ode_->setHtml(de_text(tf_, DeKind::HighOrder, TfFormat::Html));
    first_->setHtml(de_text(tf_, DeKind::FirstOrder, TfFormat::Html));
    euler_->setHtml(de_text(tf_, DeKind::Euler, TfFormat::Html));
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
