#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QRadialGradient>
#include <QWidget>

/// Window icons for small dialogs (Windows title bar / Alt-Tab).
/// Prefer Font Awesome (loaded in MainWindow); fall back to painted shapes.
namespace dialog_icons {
enum class Kind {
    IdSettings,       // structure / identification
    ModelParams,      // time & frequency ranges
    TransferFunction, // poles / h(t) / w(t)
    ChartProps,       // chart title & series
    SliderSettings,   // regulator slider range
    Help,
    App,
};

namespace detail {
inline QFont awesome_font(int pixel_size) {
    QFont f;
    // Family after QFontDatabase::addApplicationFont on FA6 Free Solid.
    f.setFamilies({QStringLiteral("Font Awesome 6 Free Solid"), QStringLiteral("Font Awesome 6 Free")});
    f.setStyleName(QStringLiteral("Solid"));
    f.setWeight(QFont::Black);
    if (pixel_size > 0)
        f.setPixelSize(pixel_size);
    f.setHintingPreference(QFont::PreferFullHinting);
    return f;
}

inline QFont awesome_ui_font(int point_size = 9) {
    QFont f = awesome_font(0);
    f.setPointSize(point_size > 0 ? point_size : 9);
    return f;
}

inline bool font_has_glyph(const QFont& font, QChar ch) {
    return QFontMetrics(font).inFont(ch);
}

// FA6 Free Solid codepoints (solid style). App uses a painted irbis, not FA.
inline QChar glyph_for(Kind kind) {
    switch (kind) {
        case Kind::IdSettings:
            return QChar(0xf1de); // sliders
        case Kind::ModelParams:
            return QChar(0xf017); // clock
        case Kind::TransferFunction:
            return QChar(0xf0ce); // table
        case Kind::ChartProps:
            return QChar(0xf201); // chart-line
        case Kind::SliderSettings:
            return QChar(0xf013); // gear
        case Kind::Help:
            return QChar(0xf059); // circle-question
        case Kind::App:
            return QChar(); // painted snow leopard
    }
    return QChar(0xf013);
}

inline QColor bg_for(Kind kind) {
    switch (kind) {
        case Kind::IdSettings:
            return QColor(0x15, 0x65, 0xc0); // blue
        case Kind::ModelParams:
            return QColor(0xef, 0x6c, 0x00); // orange
        case Kind::TransferFunction:
            return QColor(0x6a, 0x1b, 0x9a); // purple
        case Kind::ChartProps:
            return QColor(0x2e, 0x7d, 0x32); // green
        case Kind::SliderSettings:
            return QColor(0x00, 0x79, 0x6b); // teal
        case Kind::Help:
            return QColor(0x45, 0x5a, 0x64); // blue-grey
        case Kind::App:
            return QColor(0x4a, 0x5d, 0x6e); // cold slate (irbis has own face)
    }
    return QColor(0x42, 0x42, 0x42);
}

/// Snow leopard (ирбис) face — fallback if the baked .ico is missing.
/// Designed to read at 16 px: dark tile, large head, glowing eyes, few spots.
inline void paint_irbis(QPainter& p, int size) {
    const qreal s = size / 32.0;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x12, 0x1c, 0x30));
    p.drawRoundedRect(QRectF(0.2 * s, 0.2 * s, 31.6 * s, 31.6 * s), 6.0 * s, 6.0 * s);

    const QColor fur(0xec, 0xea, 0xe4);
    const QColor fur_shadow(0xc4, 0xc0, 0xb6);
    const QColor spot(0x1c, 0x18, 0x16);
    const QColor nose(0x2a, 0x24, 0x22);
    const QColor eye_rim(0x14, 0x12, 0x10);
    const QColor eye_iris(0xdc, 0xf0, 0x22);
    const QColor eye_glow(0xf0, 0xff, 0x6a);

    p.setBrush(fur);
    p.drawPolygon(QPolygonF() << QPointF(6.5 * s, 13 * s) << QPointF(3.5 * s, 3.5 * s) << QPointF(13 * s, 7.5 * s));
    p.drawPolygon(QPolygonF() << QPointF(25.5 * s, 13 * s) << QPointF(28.5 * s, 3.5 * s) << QPointF(19 * s, 7.5 * s));
    p.setBrush(spot);
    p.drawPolygon(QPolygonF() << QPointF(7.2 * s, 11 * s) << QPointF(5.2 * s, 5.2 * s) << QPointF(11 * s, 8 * s));
    p.drawPolygon(QPolygonF() << QPointF(24.8 * s, 11 * s) << QPointF(26.8 * s, 5.2 * s) << QPointF(21 * s, 8 * s));

    {
        QRadialGradient hg(QPointF(16 * s, 16 * s), 14 * s);
        hg.setColorAt(0.0, fur);
        hg.setColorAt(1.0, fur_shadow);
        p.setBrush(hg);
        p.drawEllipse(QPointF(16 * s, 17.5 * s), 13.0 * s, 11.6 * s);
    }

    p.setBrush(spot);
    for (const QPointF& c : {QPointF(8.5 * s, 16 * s), QPointF(23.5 * s, 16 * s), QPointF(16 * s, 10.5 * s),
                             QPointF(10 * s, 22 * s), QPointF(22 * s, 22 * s)})
        p.drawEllipse(c, 1.45 * s, 1.2 * s);

    auto draw_eye = [&](qreal cx) {
        QRadialGradient glow(QPointF(cx * s, 16.0 * s), 4.2 * s);
        glow.setColorAt(0.0, QColor(0xf4, 0xff, 0x70, 180));
        glow.setColorAt(1.0, QColor(0xf4, 0xff, 0x70, 0));
        p.setBrush(glow);
        p.drawEllipse(QPointF(cx * s, 16.0 * s), 4.2 * s, 3.6 * s);
        p.setBrush(eye_rim);
        p.drawEllipse(QPointF(cx * s, 16.1 * s), 3.15 * s, 3.35 * s);
        p.setBrush(eye_iris);
        p.drawEllipse(QPointF(cx * s, 16.15 * s), 2.25 * s, 2.4 * s);
        p.setBrush(eye_glow);
        p.drawEllipse(QPointF(cx * s, 16.05 * s), 1.55 * s, 1.7 * s);
        p.setBrush(eye_rim);
        p.drawEllipse(QPointF(cx * s, 16.2 * s), 0.55 * s, 1.25 * s);
        p.setBrush(Qt::white);
        p.drawEllipse(QPointF((cx - 0.75) * s, 15.2 * s), 0.55 * s, 0.55 * s);
    };
    draw_eye(10.8);
    draw_eye(21.2);

    p.setBrush(QColor(0xf4, 0xf2, 0xec));
    p.drawEllipse(QPointF(16 * s, 22.6 * s), 4.8 * s, 3.4 * s);
    p.setBrush(nose);
    p.drawPolygon(QPolygonF() << QPointF(16 * s, 19.6 * s) << QPointF(13.8 * s, 21.6 * s) << QPointF(18.2 * s, 21.6 * s));
    p.drawEllipse(QPointF(16 * s, 21.7 * s), 1.7 * s, 0.95 * s);
    p.setPen(QPen(nose, 0.9 * s, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(13.2 * s, 21.0 * s, 5.6 * s, 3.8 * s), 200 * 16, 140 * 16);
}

inline void paint_fallback(QPainter& p, Kind kind, qreal s) {
    p.setPen(QPen(Qt::white, 1.6 * s, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    switch (kind) {
        case Kind::IdSettings: {
            // Three horizontal sliders.
            for (int i = 0; i < 3; ++i) {
                const qreal y = (10 + i * 6) * s;
                p.drawLine(QPointF(8 * s, y), QPointF(24 * s, y));
                p.setBrush(Qt::white);
                p.drawEllipse(QPointF((12 + i * 4) * s, y), 2.2 * s, 2.2 * s);
                p.setBrush(Qt::NoBrush);
            }
            break;
        }
        case Kind::ModelParams: {
            // Clock face.
            p.drawEllipse(QPointF(16 * s, 16 * s), 9 * s, 9 * s);
            p.drawLine(QPointF(16 * s, 16 * s), QPointF(16 * s, 10 * s));
            p.drawLine(QPointF(16 * s, 16 * s), QPointF(21 * s, 16 * s));
            break;
        }
        case Kind::TransferFunction: {
            // Mini table grid.
            p.drawRect(QRectF(7 * s, 8 * s, 18 * s, 16 * s));
            p.drawLine(QPointF(7 * s, 13 * s), QPointF(25 * s, 13 * s));
            p.drawLine(QPointF(7 * s, 18 * s), QPointF(25 * s, 18 * s));
            p.drawLine(QPointF(16 * s, 8 * s), QPointF(16 * s, 24 * s));
            break;
        }
        case Kind::ChartProps: {
            // Axes + polyline.
            p.drawLine(QPointF(7 * s, 24 * s), QPointF(25 * s, 24 * s));
            p.drawLine(QPointF(7 * s, 24 * s), QPointF(7 * s, 8 * s));
            QPolygonF poly;
            poly << QPointF(9 * s, 20 * s) << QPointF(13 * s, 14 * s) << QPointF(17 * s, 17 * s)
                 << QPointF(23 * s, 10 * s);
            p.drawPolyline(poly);
            break;
        }
        case Kind::SliderSettings: {
            p.drawEllipse(QPointF(16 * s, 16 * s), 6 * s, 6 * s);
            p.drawEllipse(QPointF(16 * s, 16 * s), 2.2 * s, 2.2 * s);
            p.drawLine(QPointF(16 * s, 8 * s), QPointF(16 * s, 10.5 * s));
            p.drawLine(QPointF(16 * s, 21.5 * s), QPointF(16 * s, 24 * s));
            p.drawLine(QPointF(8 * s, 16 * s), QPointF(10.5 * s, 16 * s));
            p.drawLine(QPointF(21.5 * s, 16 * s), QPointF(24 * s, 16 * s));
            break;
        }
        case Kind::Help: {
            p.setBrush(Qt::white);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(16 * s, 16 * s), 9 * s, 9 * s);
            p.setPen(QPen(bg_for(kind), 2.0 * s));
            QFont f;
            f.setPixelSize(static_cast<int>(14 * s));
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(0, 0, 32 * s, 32 * s), Qt::AlignCenter, QStringLiteral("?"));
            break;
        }
        case Kind::App:
            break; // handled by paint_irbis
    }
}

inline QPixmap paint(Kind kind, int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (kind == Kind::App) {
        paint_irbis(p, size);
        return pm;
    }

    const qreal s = size / 32.0;
    // Rounded tile background (reads well at 16px in the title bar).
    p.setPen(Qt::NoPen);
    p.setBrush(bg_for(kind));
    p.drawRoundedRect(QRectF(0.5 * s, 0.5 * s, 31 * s, 31 * s), 7 * s, 7 * s);

    const QChar glyph = glyph_for(kind);
    QFont font        = awesome_font(static_cast<int>(size * 0.52));
    if (!glyph.isNull() && font_has_glyph(font, glyph)) {
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, QString(glyph));
    }
    else {
        paint_fallback(p, kind, s);
    }
    return pm;
}
} // namespace detail

[[nodiscard]] inline QIcon icon(Kind kind) {
    QIcon ic;
    for (const int sz : {16, 20, 24, 32, 48, 64})
        ic.addPixmap(detail::paint(kind, sz));
    return ic;
}

inline void apply(QWidget* widget, Kind kind) {
    if (widget)
        widget->setWindowIcon(icon(kind));
}

inline void applyGlyph(QAbstractButton* button, QChar glyph, int point_size = 9) {
    if (!button)
        return;
    button->setFont(detail::awesome_ui_font(point_size));
    button->setText(QString(glyph));
}
} // namespace dialog_icons
