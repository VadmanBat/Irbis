#pragma once

#include <QString>

/// Russian PID captions for UI only (code still uses kp / ti / td).
namespace pid_ui {
[[nodiscard]] inline QString kp() {
    return QStringLiteral("K<sub>П</sub>");
}
[[nodiscard]] inline QString ti() {
    return QStringLiteral("T<sub>И</sub>");
}
[[nodiscard]] inline QString td() {
    return QStringLiteral("T<sub>Д</sub>");
}
}

