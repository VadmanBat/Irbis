#pragma once

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QPen>

class QChart;
class QChartView;
class QValueAxis;

namespace chart_utils {
/// Chrome colors for QChart derived from the application palette (light / dark).
struct ChartTheme {
    bool dark{false};
    QColor background; ///< chart + view outer fill
    QColor plot;       ///< plot area
    QColor text;       ///< title, labels, legend
    QColor axis_line;  ///< axis spines
    QColor grid;       ///< major grid
    QColor minor_grid; ///< minor grid
    QColor guide;      ///< origin crosshair
    QColor legend_bg;
};

[[nodiscard]] inline bool isDarkPalette(const QPalette& pal) noexcept {
    return pal.color(QPalette::Window).lightness() < 128;
}

[[nodiscard]] inline ChartTheme themeFromPalette(const QPalette& pal) {
    ChartTheme t;
    t.dark = isDarkPalette(pal);

    if (t.dark) {
        // Soft dark panels (not pure black) so grid and curves stay readable.
        t.background = QColor(0x1e, 0x1e, 0x22);
        t.plot       = QColor(0x25, 0x25, 0x2a);
        t.text       = QColor(0xe8, 0xea, 0xed);
        t.axis_line  = QColor(0x9a, 0xa0, 0xa6);
        t.grid       = QColor(0x3c, 0x40, 0x48);
        t.minor_grid = QColor(0x32, 0x34, 0x3a);
        t.guide      = QColor(0xc0, 0xc4, 0xcc);
        t.legend_bg  = QColor(0x2a, 0x2a, 0x30);
    }
    else {
        t.background = QColor(Qt::white);
        t.plot       = QColor(Qt::white);
        t.text       = pal.color(QPalette::WindowText);
        if (t.text.lightness() > 200)
            t.text = QColor(0x21, 0x21, 0x21);
        t.axis_line  = QColor(0x42, 0x42, 0x42);
        t.grid       = QColor(0xd0, 0xd0, 0xd0);
        t.minor_grid = QColor(0xe8, 0xe8, 0xe8);
        t.guide      = QColor(0x12, 0x12, 0x12);
        t.legend_bg  = QColor(0xfa, 0xfa, 0xfa);
    }
    return t;
}

[[nodiscard]] inline ChartTheme currentTheme() {
    if (qApp)
        return themeFromPalette(qApp->palette());
    return themeFromPalette(QPalette());
}

[[nodiscard]] inline bool isDarkTheme() {
    return currentTheme().dark;
}

/// Series palette: same hue family, brighter on dark backgrounds.
[[nodiscard]] inline QPen penForIndexTheme(std::size_t index, bool dark) {
    static const QColor light_cols[6] = {
        QColor(0x1f, 0x77, 0xb4), QColor(0xff, 0x7f, 0x0e), QColor(0x2c, 0xa0, 0x2c),
        QColor(0xd6, 0x27, 0x28), QColor(0x94, 0x67, 0xbd), QColor(0x8c, 0x56, 0x4b),
    };
    static const QColor dark_cols[6] = {
        QColor(0x6c, 0xb6, 0xff), QColor(0xff, 0xb0, 0x4a), QColor(0x5d, 0xdf, 0x5d),
        QColor(0xff, 0x6b, 0x6b), QColor(0xc4, 0xa0, 0xff), QColor(0xe0, 0xb0, 0x80),
    };
    const QColor* cols = dark ? dark_cols : light_cols;
    return QPen(cols[index % 6], 2);
}

[[nodiscard]] inline QPen guidePen(const ChartTheme& t) {
    QPen pen(t.guide, 2.0);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::FlatCap);
    pen.setStyle(Qt::SolidLine);
    return pen;
}

void applyAxisTheme(QValueAxis* axis, const ChartTheme& theme);
/// Background, axes, legend, guides, and default series colors for light/dark.
void applyChartTheme(QChart* chart, QChartView* view = nullptr);
} // namespace chart_utils
