#pragma once

/// Identification options (sidebar on IdTab).
struct IdSettings {
    enum class PlantKind : int {
        Static  = 0, ///< shelf h∞ → Simoyu
        Astatic = 1, ///< integrator k/p
    };

    PlantKind plantKind = PlantKind::Static;
    bool autoOrder      = true; ///< Simoyu identifyAuto
    int denOrder        = 2;    ///< n = deg(D), used when !autoOrder
    int numOrder        = 0;    ///< m = deg(N), m ≤ n
    int maxAutoOrder    = 6;    ///< max plant order for auto structure
    bool estimateTau    = true; ///< DeadTimeEstimator + strip before Simoyu
};
