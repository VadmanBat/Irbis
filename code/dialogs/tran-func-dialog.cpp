#include "code/dialogs/tran-func-dialog.h"

#include "code/util/dialog-icons.hxx"
#include "code/util/format.hxx"
#include "ui_tran-func-dialog.h"

#include <cmath>
#include <complex>
#include <numbers>
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QHeaderView>
#include <QMenu>
#include <QTableWidgetItem>
#include <QToolTip>

namespace {
enum class SolutionFormat { Html, Plain, Latex };

QString solution_text(const numina::LaplaceSolution& sol, SolutionFormat format) {
    switch (format) {
        case SolutionFormat::Html:
            return QString::fromStdString(sol.htmlString());
        case SolutionFormat::Plain:
            return QString::fromStdString(sol.plainString());
        case SolutionFormat::Latex:
            return QString::fromStdString(sol.latexString());
    }
    return {};
}
} // namespace

TranFuncDialog::TranFuncDialog(const numina::TransferFunction& tf, QWidget* parent)
    : QDialog(parent), ui(new Ui::TranFuncDialog), tf_(tf) {
    ui->setupUi(this);
    dialog_icons::apply(this, dialog_icons::Kind::TransferFunction);

    ui->polesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->polesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fill_poles();

    ui->htTextEdit->setHtml(QString::fromStdString(tf_.transientSolution().htmlString()));
    ui->wtTextEdit->setHtml(QString::fromStdString(tf_.impulseSolution().htmlString()));

    setup_copy_menus();

    connect(ui->okButton, &QPushButton::clicked, this, &QDialog::accept);
}

TranFuncDialog::~TranFuncDialog() {
    delete ui;
}

void TranFuncDialog::setup_copy_menus() {
    auto wire = [this](QToolButton* button, bool transient) {
        auto* menu = new QMenu(button);

        const auto add = [&](const QString& title, SolutionFormat format) {
            auto* act = menu->addAction(title);
            connect(act, &QAction::triggered, this, [this, button, transient, format] {
                const numina::LaplaceSolution& sol = transient ? tf_.transientSolution() : tf_.impulseSolution();
                copy_solution_text(solution_text(sol, format), button);
            });
        };

        add(tr("Обычный текст"), SolutionFormat::Plain);
        add(tr("LaTeX"), SolutionFormat::Latex);
        add(tr("HTML"), SolutionFormat::Html);

        button->setMenu(menu);
    };

    wire(ui->htCopyButton, true);
    wire(ui->wtCopyButton, false);
}

void TranFuncDialog::copy_solution_text(const QString& text, QWidget* anchor) {
    QApplication::clipboard()->setText(text);
    if (anchor) {
        QToolTip::showText(anchor->mapToGlobal(QPoint(0, anchor->height())), tr("Скопировано"), anchor, QRect(), 1500);
    }
}

QColor TranFuncDialog::root_color(double value) {
    if (value < 0)
        return QColor(80, 80, 80);
    if (value <= 0.5)
        return {static_cast<int>(255 * (value * 2)), static_cast<int>(255 * (1 - value * 2)), 0};
    return {255, static_cast<int>(255 * (1 - (value - 0.5) * 2)), 0};
}

void TranFuncDialog::fill_poles() {
    ui->polesTable->setRowCount(0);

    const auto& reals = tf_.poles().first;
    const auto& comps = tf_.poles().second;

    bool any_multi = false;
    for (const auto& e : reals) {
        if (e.second > 1) {
            any_multi = true;
            break;
        }
    }
    if (!any_multi) {
        for (const auto& e : comps) {
            if (e.second > 1) {
                any_multi = true;
                break;
            }
        }
    }

    constexpr int kCol = 8;
    ui->polesTable->setColumnCount(any_multi ? 9 : 8);
    if (any_multi)
        ui->polesTable->setHorizontalHeaderItem(kCol, new QTableWidgetItem(tr("k")));

    // Tooltips on header hover (QTableWidgetItem::setToolTip).
    const struct {
        int col;
        const char* tip;
    } header_tips[] = {
        {0, QT_TR_NOOP("Действительная часть полюса (1/с). Цвет: синий — Re < 0 (устойчиво), розовый — Re ≥ 0.")},
        {1, QT_TR_NOOP("Мнимая часть. Для сопряжённой пары — ±|Im| в одной строке.")},
        {2, QT_TR_NOOP("Модуль полюса |p| = √(Re² + Im²).")},
        {3, QT_TR_NOOP("Аргумент полюса, градусы. Для пары — ±|Arg|.")},
        {4, QT_TR_NOOP("Постоянная времени τ = −1/Re. При Re = 0 — ∞.")},
        {5, QT_TR_NOOP("Коэффициент демпфирования ζ = −Re / |p|.")},
        {6, QT_TR_NOOP("Период собственных колебаний T = 2π / |Im|.")},
        {7, QT_TR_NOOP("Значимость относительно доминантного полюса (max Re): фон по отношению Re_dom / Re.")},
        {8, QT_TR_NOOP("Кратность корня k (если есть кратные).")},
    };
    // Im, ζ, T — complex-related columns (subtle tint).
    constexpr int kComplexCols[] = {1, 5, 6};
    const QBrush complex_header_bg(QColor(0x4a, 0x3d, 0x66));
    const QBrush complex_header_fg(QColor(0xce, 0x93, 0xd8));
    const QBrush complex_cell_bg(QColor(0x3a, 0x32, 0x4e));

    const int n_tip = any_multi ? 9 : 8;
    for (int c = 0; c < n_tip; ++c) {
        auto* item = ui->polesTable->horizontalHeaderItem(c);
        if (!item) {
            item = new QTableWidgetItem;
            ui->polesTable->setHorizontalHeaderItem(c, item);
        }
        item->setToolTip(tr(header_tips[c].tip));
        for (const int cc : kComplexCols) {
            if (c == cc) {
                item->setBackground(complex_header_bg);
                item->setForeground(complex_header_fg);
                break;
            }
        }
    }

    // Dominant = rightmost pole (max Re); numina keeps roots sorted, dominantPole() uses that.
    const double dom_re = tf_.hasPoles() ? tf_.dominantPole().real() : 0.0;

    auto add_row = [&](std::complex<double> pole, std::size_t mult) {
        const int row = ui->polesTable->rowCount();
        ui->polesTable->insertRow(row);

        auto* re_item = new QTableWidgetItem(pole.real() < 0 ? num_format::format(pole.real())
                                                             : QStringLiteral("+") + num_format::format(pole.real()));
        re_item->setForeground(QBrush(pole.real() < 0 ? QColor(0x80, 0xd8, 0xff) : QColor(0xff, 0xb0, 0xc8)));
        ui->polesTable->setItem(row, 0, re_item);

        const double abs_p    = std::abs(pole);
        const bool is_complex = std::abs(pole.imag()) > 1e-10;
        const double arg_deg  = std::arg(pole) * 180.0 / std::numbers::pi;

        ui->polesTable->setItem(row, 2, new QTableWidgetItem(num_format::format(abs_p)));
        // Complex pair: Arg as ±|φ|; real pole — single signed angle.
        ui->polesTable->setItem(
            row, 3,
            new QTableWidgetItem(is_complex ? QStringLiteral("±") + num_format::format(std::abs(arg_deg))
                                            : num_format::format(arg_deg)));
        ui->polesTable->setItem(
            row, 4,
            new QTableWidgetItem(pole.real() != 0.0 ? num_format::format(-1.0 / pole.real()) : QStringLiteral("∞")));

        auto place_complex_cell = [&](int col, const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setBackground(complex_cell_bg);
            ui->polesTable->setItem(row, col, item);
        };
        // Keep complex columns tinted even when empty (real poles).
        place_complex_cell(1, is_complex ? QStringLiteral("±") + num_format::format(std::abs(pole.imag())) : QString());
        place_complex_cell(5, is_complex ? num_format::format(-pole.real() / abs_p) : QString());
        place_complex_cell(6,
                           is_complex ? num_format::format(2.0 * std::numbers::pi / std::abs(pole.imag())) : QString());

        auto* color_item = new QTableWidgetItem;
        if (dom_re != 0.0 && pole.real() != 0.0)
            color_item->setBackground(root_color(dom_re / pole.real()));
        ui->polesTable->setItem(row, 7, color_item);

        if (any_multi)
            ui->polesTable->setItem(row, kCol, new QTableWidgetItem(QString::number(static_cast<int>(mult))));
    };

    for (const auto& [r, mult] : reals)
        add_row({r, 0.0}, mult);

    // numina: complex roots come as consecutive conjugate pairs (z, conj(z)).
    const std::size_t n_comp = comps.size();
    for (std::size_t i = 0; i < n_comp; ++i) {
        add_row(comps[i].first, comps[i].second);
        if (i + 1 < n_comp && comps[i + 1].first == std::conj(comps[i].first))
            ++i; // skip conjugate partner
    }
}
