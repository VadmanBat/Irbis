#pragma once

#include <initializer_list>
#include <QColor>
#include <QLabel>
#include <QPalette>

/// Muted secondary text that stays readable on light and dark themes.
/// `palette(mid)` is a bevel fill and often vanishes on dark backgrounds.
namespace secondary_text {
inline QColor colorFor(const QPalette& pal) {
    const QColor bg = pal.color(QPalette::Window);
    const QColor fg = pal.color(QPalette::WindowText);
    const QColor ph = pal.color(QPalette::PlaceholderText);

    // Prefer PlaceholderText when it has enough contrast against the window.
    if (qAbs(ph.lightness() - bg.lightness()) >= 70)
        return ph;

    // Blend ~68% window-text with window → secondary, always legible.
    constexpr int t = 68;
    constexpr int b = 100 - t;
    return QColor((fg.red() * t + bg.red() * b) / 100, (fg.green() * t + bg.green() * b) / 100,
                  (fg.blue() * t + bg.blue() * b) / 100);
}

/// Apply secondary color; keep font-size from any existing stylesheet (9pt etc.).
inline void apply(QLabel* label) {
    if (!label)
        return;

    const QColor c = colorFor(label->palette());
    QString font_rule;
    const QString ss = label->styleSheet();
    const int idx    = ss.indexOf(QStringLiteral("font-size"));
    if (idx >= 0) {
        const int end = ss.indexOf(QLatin1Char(';'), idx);
        font_rule     = QLatin1Char(' ') + (end >= 0 ? ss.mid(idx, end - idx + 1) : ss.mid(idx));
    }
    label->setStyleSheet(QStringLiteral("color: %1;%2").arg(c.name(), font_rule));
}

inline void applyAll(std::initializer_list<QLabel*> labels) {
    for (QLabel* label : labels)
        apply(label);
}
} // namespace secondary_text
