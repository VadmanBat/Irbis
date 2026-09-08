#pragma once

#include <QStyle>
#include <QVariant>
#include <QWidget>

/// Dynamic-property helpers for QSS (no hardcoded colors in C++).
namespace style_util {
inline void setProperty(QWidget* widget, const char* name, const QVariant& value) {
    if (!widget)
        return;
    widget->setProperty(name, value);
    if (QStyle* style = widget->style()) {
        style->unpolish(widget);
        style->polish(widget);
    }
    widget->update();
}
} // namespace style_util
