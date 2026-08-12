#pragma once

#include <sstream>
#include <string>
#include <vector>

/// Фабрика классических непрерывных регуляторов (остаётся в RegValve, не в numina).
namespace regulator_factory {
struct Result {
    std::vector<double> num; // high → low
    std::vector<double> den; // high → low
    std::string title;
};

/// P/I/D flags → W_reg(p). Kp, Tu, Td — параметры слайдеров.
inline Result make(bool P, bool I, bool D, double Kp, double Tu, double Td) {
    Result r;
    const int id = static_cast<int>(P) + 2 * static_cast<int>(I) + 4 * static_cast<int>(D);
    std::ostringstream stream;
    stream << std::fixed;
    stream.precision(2);

    switch (id) {
        case 0:
            r.num = {1.0};
            r.den = {1.0};
            stream << "1";
            break;
        case 1: // P
            r.num = {Kp};
            r.den = {1.0};
            stream << "P(" << Kp << ")";
            break;
        case 2: // I: 1/(Tu p)
            r.num = {1.0};
            r.den = {Tu, 0.0};
            stream << "I(" << Tu << ")";
            break;
        case 3: // PI
            r.num = {Kp, Kp / Tu};
            r.den = {1.0, 0.0};
            stream << "PI(" << Kp << ", " << Tu << ")";
            break;
        case 4: // D
            r.num = {Td, 0.0};
            r.den = {1.0};
            stream << "D(" << Td << ")";
            break;
        case 5: // PD
            r.num = {Kp * Td, Kp};
            r.den = {1.0};
            stream << "PD(" << Kp << ", " << Td << ")";
            break;
        case 6: // ID
            r.num = {Td, 0.0, 1.0 / Tu};
            r.den = {1.0, 0.0};
            stream << "ID(" << Tu << ", " << Td << ")";
            break;
        case 7: // PID
            r.num = {Kp * Td, Kp, Kp / Tu};
            r.den = {1.0, 0.0};
            stream << "PID(" << Kp << ", " << Tu << ", " << Td << ")";
            break;
        default:
            r.num = {1.0};
            r.den = {1.0};
            stream << "1";
            break;
    }
    r.title = stream.str();
    return r;
}
} // namespace regulator_factory
