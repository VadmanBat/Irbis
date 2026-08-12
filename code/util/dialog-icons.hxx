#pragma once

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
    f.setPixelSize(pixel_size);
    f.setHintingPreference(QFont::PreferFullHinting);
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
        case Kind::Help:
            return QColor(0x45, 0x5a, 0x64); // blue-grey
        case Kind::App:
            return QColor(0x4a, 0x5d, 0x6e); // cold slate (irbis has own face)
    }
    return QColor(0x42, 0x42, 0x42);
}

/// Snow leopard (ирбис) face for the application icon.
inline void paint_irbis(QPainter& p, int size) {
    const qreal s = size / 32.0;

    // Cold mountain sky tile.
    {
        QLinearGradient g(0, 0, 0, size);
        g.setColorAt(0.0, QColor(0x6b, 0x7d, 0x8c));
        g.setColorAt(1.0, QColor(0x3d, 0x4a, 0x56));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(QRectF(0.4 * s, 0.4 * s, 31.2 * s, 31.2 * s), 6.5 * s, 6.5 * s);
    }

    // Head fur (soft gray-white).
    const QColor fur(0xe8, 0xe6, 0xe0);
    const QColor fur_shadow(0xc8, 0xc4, 0xba);
    const QColor spot(0x3a, 0x36, 0x32);
    const QColor nose(0x2a, 0x24, 0x22);
    const QColor eye_rim(0x1a, 0x18, 0x16);
    const QColor eye_iris(0xc8, 0xd4, 0x3a); // pale chartreuse, typical for irbis
    const QColor eye_shine(0xff, 0xff, 0xff);

    // Ears (behind head slightly).
    p.setBrush(fur);
    p.setPen(QPen(fur_shadow, 0.6 * s));
    {
        QPolygonF left_ear;
        left_ear << QPointF(7 * s, 14 * s) << QPointF(5 * s, 5 * s) << QPointF(13 * s, 9 * s);
        p.drawPolygon(left_ear);
        QPolygonF right_ear;
        right_ear << QPointF(25 * s, 14 * s) << QPointF(27 * s, 5 * s) << QPointF(19 * s, 9 * s);
        p.drawPolygon(right_ear);
    }
    // Ear inner (rosy-gray) + dark tips.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xb8, 0xa0, 0x98));
    p.drawPolygon(QPolygonF() << QPointF(8.2 * s, 12.5 * s) << QPointF(7 * s, 7.5 * s) << QPointF(11.5 * s, 9.5 * s));
    p.drawPolygon(QPolygonF() << QPointF(23.8 * s, 12.5 * s) << QPointF(25 * s, 7.5 * s) << QPointF(20.5 * s, 9.5 * s));
    p.setBrush(spot);
    p.drawEllipse(QPointF(6.5 * s, 6.2 * s), 1.3 * s, 1.3 * s);
    p.drawEllipse(QPointF(25.5 * s, 6.2 * s), 1.3 * s, 1.3 * s);

    // Head.
    {
        QRadialGradient hg(QPointF(16 * s, 15 * s), 12 * s);
        hg.setColorAt(0.0, fur);
        hg.setColorAt(1.0, fur_shadow);
        p.setBrush(hg);
        p.setPen(QPen(QColor(0x9a, 0x96, 0x8e), 0.5 * s));
        p.drawEllipse(QPointF(16 * s, 17 * s), 11.2 * s, 10.2 * s);
    }

    // Rosette spots (simplified — readable at 16–32 px).
    p.setPen(Qt::NoPen);
    p.setBrush(spot);
    const QPointF spots[] = {
        {9.5 * s, 14 * s},  {22.5 * s, 14 * s},  {11 * s, 20 * s},     {21 * s, 20 * s},
        {16 * s, 11.5 * s}, {8.5 * s, 17.5 * s}, {23.5 * s, 17.5 * s},
    };
    for (const QPointF& c : spots)
        p.drawEllipse(c, 1.15 * s, 0.95 * s);
    // Smaller secondary flecks.
    p.setBrush(QColor(0x55, 0x50, 0x4a));
    p.drawEllipse(QPointF(13 * s, 13 * s), 0.55 * s, 0.45 * s);
    p.drawEllipse(QPointF(19 * s, 13 * s), 0.55 * s, 0.45 * s);
    p.drawEllipse(QPointF(14.5 * s, 22 * s), 0.5 * s, 0.4 * s);
    p.drawEllipse(QPointF(17.5 * s, 22 * s), 0.5 * s, 0.4 * s);

    // Eyes.
    auto draw_eye = [&](qreal cx) {
        p.setBrush(eye_rim);
        p.drawEllipse(QPointF(cx * s, 16.2 * s), 2.4 * s, 2.6 * s);
        p.setBrush(eye_iris);
        p.drawEllipse(QPointF(cx * s, 16.3 * s), 1.55 * s, 1.7 * s);
        p.setBrush(eye_rim);
        p.drawEllipse(QPointF(cx * s, 16.3 * s), 0.55 * s, 1.15 * s); // vertical pupil
        p.setBrush(eye_shine);
        p.drawEllipse(QPointF((cx - 0.55) * s, 15.5 * s), 0.45 * s, 0.45 * s);
    };
    draw_eye(11.5);
    draw_eye(20.5);

    // Brow / cheek tuft lines (subtle).
    p.setPen(QPen(QColor(0x7a, 0x74, 0x6c), 0.7 * s, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(8.5 * s, 13.5 * s), QPointF(10.5 * s, 14.2 * s));
    p.drawLine(QPointF(23.5 * s, 13.5 * s), QPointF(21.5 * s, 14.2 * s));

    // Muzzle.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xf2, 0xf0, 0xea));
    p.drawEllipse(QPointF(16 * s, 21.5 * s), 4.2 * s, 3.2 * s);

    // Nose.
    p.setBrush(nose);
    {
        QPolygonF n;
        n << QPointF(16 * s, 19.2 * s) << QPointF(14.2 * s, 20.8 * s) << QPointF(17.8 * s, 20.8 * s);
        p.drawPolygon(n);
        p.drawEllipse(QPointF(16 * s, 20.9 * s), 1.5 * s, 0.85 * s);
    }

    // Mouth.
    p.setPen(QPen(nose, 0.75 * s, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(13.5 * s, 20.5 * s, 5 * s, 3.5 * s), 200 * 16, 140 * 16);

    // Whiskers (skip on tiny icons).
    if (size >= 24) {
        p.setPen(QPen(QColor(0x90, 0x8a, 0x82), 0.55 * s, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(11.5 * s, 21 * s), QPointF(5.5 * s, 19.5 * s));
        p.drawLine(QPointF(11.5 * s, 22 * s), QPointF(5.2 * s, 22 * s));
        p.drawLine(QPointF(11.5 * s, 23 * s), QPointF(5.5 * s, 24.5 * s));
        p.drawLine(QPointF(20.5 * s, 21 * s), QPointF(26.5 * s, 19.5 * s));
        p.drawLine(QPointF(20.5 * s, 22 * s), QPointF(26.8 * s, 22 * s));
        p.drawLine(QPointF(20.5 * s, 23 * s), QPointF(26.5 * s, 24.5 * s));
    }
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
} // namespace dialog_icons
