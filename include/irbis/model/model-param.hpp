#pragma once

/// Simulation / frequency-sweep settings shared by all tabs.
struct ModelParam {
    /// Time response: auto «от–до» and auto sample count.
    bool autoTimeRange     = true;
    bool autoTimeIntervals = true;
    /// Frequency: auto ω-range (lab auto), else range (adaptive), else range + N (log).
    bool autoFreqRange     = true;
    bool autoFreqIntervals = true;

    double timeMin    = 0.0;
    double timeMax    = 500.0;
    int timeIntervals = 100;

    double freqMin    = 0.01;
    double freqMax    = 10.0;
    int freqIntervals = 100;

    int approxOrder    = 6;    ///< Padé order for e^{-τp}
    bool usePadeApprox = true; ///< false → exact e^{-τp} (analysis); synthesis always true
};
