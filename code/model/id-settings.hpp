#pragma once

/// Identification algorithm options (dialog «Настройки идентификации»).
struct IdSettings {
    bool autoOrder   = true; ///< Simoyu identifyAuto
    int denOrder     = 2;    ///< n = deg(D), used when !autoOrder
    int numOrder     = 0;    ///< m = deg(N), m ≤ n
    int maxAutoOrder = 6;    ///< max plant order for auto structure (not Padé approxOrder)
    bool estimateTau = true; ///< DeadTimeEstimator + strip before Simoyu
};
