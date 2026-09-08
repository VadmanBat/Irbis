#pragma once

#include <QString>

/// Fonts and default QSS for Irbis widgets. Safe to call more than once.
/// App 2: call `irbis::loadFonts()` after `QApplication`; `applyStyleSheet()` if you want the Irbis look.
namespace irbis {
void loadFonts();
void applyStyleSheet();
[[nodiscard]] QString defaultStyleSheet();
}

