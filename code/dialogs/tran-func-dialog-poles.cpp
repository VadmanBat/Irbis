#include "code/dialogs/tran-func-dialog.h"
#include "code/util/format.hxx"
#include "ui_tran-func-dialog.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QPainter>
#include <QPalette>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

namespace {
constexpr auto kAlignNum = Qt::AlignRight | Qt::AlignVCenter;
constexpr auto kAlignMid = Qt::AlignCenter;

constexpr bool is_cx_col(int col) noexcept {
    return col == 1 || col == 5 || col == 6;
}

int lerp_i(int a, int b, double t) {
    return static_cast<int>(std::lround(static_cast<double>(a) + (static_cast<double>(b - a) * t)));
}

QColor mix_rgb(const QColor& a, const QColor& b, double t) {
    return {lerp_i(a.red(), b.red(), t), lerp_i(a.green(), b.green(), t), lerp_i(a.blue(), b.blue(), t)};
}

QFont mono_font(const QWidget* w) {
    QFont f = w->font();
    f.setFamilies({QStringLiteral("Consolas"), QStringLiteral("Cascadia Mono"), QStringLiteral("Courier New")});
    return f;
}

QTableWidgetItem* make_num(const QString& text, const QFont& font, const QColor& muted = {}) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(kAlignNum);
    item->setFont(font);
    if (muted.isValid())
        item->setForeground(muted);
    return item;
}

QTableWidgetItem* make_mid(const QString& text, const QFont& font) {
    auto* item = new QTableWidgetItem(text);
    item->setTextAlignment(kAlignMid);
    item->setFont(font);
    return item;
}

class PolesHeader : public QHeaderView {
    QColor head_bg_;
    QColor head_fg_;
    QColor cx_bg_;
    QColor cx_fg_;
    QColor cx_bar_;
    QColor line_;

public:
    explicit PolesHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent) {
        setProperty("cxBand", true);
        setSectionsClickable(false);
        setHighlightSections(false);
        setDefaultAlignment(kAlignNum);
        setStretchLastSection(true);
        setTextElideMode(Qt::ElideRight);
        setSectionResizeMode(QHeaderView::Stretch);
        // Local sheet so app QSS does not paint over the complex-column band.
        setStyleSheet(
            QStringLiteral("QHeaderView { background: transparent; border: none; }"
                           "QHeaderView::section { background: transparent; border: none; padding: 0; }"));
    }

    void setColors(const QColor& head_bg, const QColor& head_fg, const QColor& cx_bg, const QColor& cx_fg,
                   const QColor& cx_bar, const QColor& line) {
        head_bg_ = head_bg;
        head_fg_ = head_fg;
        cx_bg_   = cx_bg;
        cx_fg_   = cx_fg;
        cx_bar_  = cx_bar;
        line_    = line;
        update();
    }

protected:
    void paintSection(QPainter* p, const QRect& rect, int logical) const override {
        if (!rect.isValid() || !model())
            return;
        const bool cx = is_cx_col(logical);
        p->save();
        p->setClipRect(rect);
        p->fillRect(rect, cx ? cx_bg_ : head_bg_);
        if (cx)
            p->fillRect(QRect(rect.left(), rect.top(), rect.width(), 3), cx_bar_);
        p->setPen(line_);
        p->drawLine(rect.bottomLeft(), rect.bottomRight());
        p->drawLine(rect.topRight() + QPoint(0, 1), rect.bottomRight());

        const QString text = model()->headerData(logical, Qt::Horizontal, Qt::DisplayRole).toString();
        QVariant al        = model()->headerData(logical, Qt::Horizontal, Qt::TextAlignmentRole);
        const auto align   = al.isValid() ? Qt::Alignment(al.toInt()) : kAlignNum;
        QFont f            = font();
        f.setWeight(QFont::DemiBold);
        p->setFont(f);
        p->setPen(cx ? cx_fg_ : head_fg_);
        const QRect tr = rect.adjusted(8, cx ? 3 : 0, -12, 0);
        p->drawText(tr, align | Qt::TextSingleLine, QFontMetrics(f).elidedText(text, Qt::ElideRight, tr.width()));
        p->restore();
    }
};

PolesHeader* poles_header(QTableWidget* table) {
    auto* header = table->horizontalHeader();
    if (!header->property("cxBand").toBool()) {
        header = new PolesHeader(table);
        table->setHorizontalHeader(header);
    }
    return static_cast<PolesHeader*>(header);
}
}

QColor TranFuncDialog::root_color(double value, bool dark) {
    const QColor base = dark ? QColor(0x2a, 0x2a, 0x30) : QColor(0xf5, 0xf5, 0xf7);
    if (value < 0)
        return dark ? QColor(0x3a, 0x3a, 0x40) : QColor(0xe8, 0xe8, 0xec);

    QColor hot;
    if (value <= 0.5) {
        const double t = value * 2.0;
        hot            = QColor(lerp_i(0x43, 0xf9, t), lerp_i(0xa0, 0xa8, t), lerp_i(0x47, 0x25, t));
    }
    else {
        const double t = std::min(1.0, (value - 0.5) * 2.0);
        hot            = QColor(lerp_i(0xf9, 0xe5, t), lerp_i(0xa8, 0x39, t), lerp_i(0x25, 0x35, t));
    }
    constexpr double kMix = 0.42;
    return mix_rgb(base, hot, kMix);
}

void TranFuncDialog::fit_poles_table() {
    auto* table  = ui->polesTable;
    auto* header = table->horizontalHeader();
    table->resizeRowsToContents();

    int header_h = header->sizeHint().height();
    if (header_h < 8)
        header_h = table->fontMetrics().height() + 16;
    header->setFixedHeight(header_h);

    const int rows     = table->rowCount();
    constexpr int kCap = 12;
    const int vis      = rows < 1 ? 1 : std::min(rows, kCap);
    int h              = header_h + 2 * table->frameWidth() + 2;
    if (rows == 0)
        h += table->fontMetrics().height() + 12;
    else {
        for (int i = 0; i < vis; ++i)
            h += table->rowHeight(i);
    }
    table->setFixedHeight(h);
    table->setVerticalScrollBarPolicy(rows > kCap ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

void TranFuncDialog::fill_poles() {
    auto* table         = ui->polesTable;
    PolesHeader* header = poles_header(table);
    table->setRowCount(0);

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
    table->setColumnCount(any_multi ? 9 : 8);
    if (any_multi)
        table->setHorizontalHeaderItem(kCol, new QTableWidgetItem(tr("k")));

    const struct {
        int col;
        const char* tip;
    } header_tips[] = {
        {0, QT_TR_NOOP("Действительная часть полюса (1/с). Цвет: синий — Re < 0 (устойчиво), розовый — Re ≥ 0.")},
        {1, QT_TR_NOOP("Мнимая часть. Только для комплексных полюсов; колонка выделена. Для пары — ±|Im|.")},
        {2, QT_TR_NOOP("Модуль полюса |p| = √(Re² + Im²).")},
        {3, QT_TR_NOOP("Аргумент полюса, градусы. Для пары — ±|Arg|.")},
        {4, QT_TR_NOOP("Постоянная времени τ = −1/Re. При Re = 0 — ∞.")},
        {5, QT_TR_NOOP("Коэффициент демпфирования ζ = −Re / |p|. Только для комплексных полюсов; колонка выделена.")},
        {6,
         QT_TR_NOOP("Период собственных колебаний T = 2π / |Im|. Только для комплексных полюсов; колонка выделена.")},
        {7, QT_TR_NOOP("Значимость относительно доминантного полюса (max Re): Re_dom / Re. Фон — тепловая шкала.")},
        {8, QT_TR_NOOP("Кратность корня k (если есть кратные).")},
    };

    const bool dark    = table->palette().color(QPalette::Base).lightness() < 128;
    const QColor paper = table->palette().color(QPalette::Base);
    const QColor accent(0x7e, 0x57, 0xc2);
    const QColor cx_bar = dark ? QColor(0xce, 0x93, 0xd8) : accent;
    const QColor cx_fg  = dark ? QColor(0xe1, 0xbe, 0xe7) : QColor(0x5e, 0x35, 0xb1);
    const QBrush cx_cell(mix_rgb(paper, accent, dark ? 0.34 : 0.14));
    header->setColors(dark ? QColor(0x36, 0x36, 0x3c) : QColor(0xec, 0xec, 0xf2),
                      table->palette().color(QPalette::WindowText), mix_rgb(paper, accent, dark ? 0.50 : 0.22), cx_fg,
                      cx_bar, table->palette().color(QPalette::Mid));

    const int n_tip = any_multi ? 9 : 8;
    for (int c = 0; c < n_tip; ++c) {
        auto* item = table->horizontalHeaderItem(c);
        if (!item) {
            item = new QTableWidgetItem;
            table->setHorizontalHeaderItem(c, item);
        }
        item->setToolTip(tr(header_tips[c].tip));
        item->setTextAlignment(c == kCol ? kAlignMid : kAlignNum);
    }

    header->setSectionResizeMode(QHeaderView::Stretch);

    const QFont mono    = mono_font(table);
    const QColor muted  = table->palette().color(QPalette::PlaceholderText);
    const QColor re_neg = dark ? QColor(0x80, 0xd8, 0xff) : QColor(0x02, 0x77, 0xbd);
    const QColor re_pos = dark ? QColor(0xff, 0xb0, 0xc8) : QColor(0xc6, 0x28, 0x28);
    const double dom_re = tf_.hasPoles() ? tf_.dominantPole().real() : 0.0;
    const QString dash  = QStringLiteral("—");

    auto place = [&](int row, int col, QTableWidgetItem* item) {
        if (is_cx_col(col))
            item->setBackground(cx_cell);
        table->setItem(row, col, item);
    };

    auto add_row = [&](std::complex<double> pole, std::size_t mult) {
        const int row = table->rowCount();
        table->insertRow(row);

        auto* re_item = make_num(num_format::format(pole.real()), mono);
        re_item->setForeground(QBrush(pole.real() < 0 ? re_neg : re_pos));
        table->setItem(row, 0, re_item);

        const double abs_p    = std::abs(pole);
        const bool is_complex = std::abs(pole.imag()) > 1e-10;
        const double arg_deg  = std::arg(pole) * 180.0 / std::numbers::pi;

        place(row, 1,
              is_complex ? make_num(QStringLiteral("±") + num_format::format(std::abs(pole.imag())), mono)
                         : make_num(dash, mono, muted));
        table->setItem(row, 2, make_num(num_format::format(abs_p), mono));
        table->setItem(row, 3,
                       make_num(is_complex ? QStringLiteral("±") + num_format::format(std::abs(arg_deg))
                                           : num_format::format(arg_deg),
                                mono));
        table->setItem(
            row, 4, make_num(pole.real() != 0.0 ? num_format::format(-1.0 / pole.real()) : QStringLiteral("∞"), mono));
        place(row, 5,
              is_complex ? make_num(num_format::format(-pole.real() / abs_p), mono) : make_num(dash, mono, muted));
        place(row, 6,
              is_complex ? make_num(num_format::format(2.0 * std::numbers::pi / std::abs(pole.imag())), mono)
                         : make_num(dash, mono, muted));

        if (dom_re != 0.0 && pole.real() != 0.0) {
            const double rel = dom_re / pole.real();
            auto* item       = make_num(num_format::format(rel), mono);
            item->setBackground(root_color(rel, dark));
            table->setItem(row, 7, item);
        }
        else {
            table->setItem(row, 7, make_num(dash, mono, muted));
        }

        if (any_multi)
            table->setItem(row, kCol, make_mid(QString::number(static_cast<int>(mult)), mono));
    };

    for (const auto& [r, mult] : reals)
        add_row({r, 0.0}, mult);

    const std::size_t n_comp = comps.size();
    for (std::size_t i = 0; i < n_comp; ++i) {
        add_row(comps[i].first, comps[i].second);
        if (i + 1 < n_comp && comps[i + 1].first == std::conj(comps[i].first))
            ++i;
    }

    fit_poles_table();
}
