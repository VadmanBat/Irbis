#pragma once

#include <cmath>
#include <QCoreApplication>
#include <QString>
#include <vector>

/// Classic link name from the pattern of non-zero powers (same table as the old TranFuncForm).
namespace tf_link_name {
inline int power_mask(const std::vector<double>& high_to_low, int max_bits = 6) {
    int id      = 0;
    const int n = static_cast<int>(high_to_low.size());
    if (n <= 0)
        return 0;
    const int deg = n - 1;
    for (int p = 0; p < max_bits; ++p) {
        if (p > deg)
            break;
        if (high_to_low[static_cast<std::size_t>(deg - p)] != 0.0)
            id += 1 << p;
    }
    return id;
}

inline double coeff_of(const std::vector<double>& high_to_low, int power) {
    const int n = static_cast<int>(high_to_low.size());
    if (n <= 0)
        return 0.0;
    const int deg = n - 1;
    if (power < 0 || power > deg)
        return 0.0;
    return high_to_low[static_cast<std::size_t>(deg - power)];
}

[[nodiscard]] inline QString describe(const std::vector<double>& num, const std::vector<double>& den, double tau) {
    const int id = power_mask(num) + power_mask(den) * 64;
    QString name;
    switch (id) {
        case 1 + 1 * 64:
            name = QCoreApplication::translate("tf_link_name", "Усилительное (безынерционное)");
            break;
        case 1 + 2 * 64:
            name = QCoreApplication::translate("tf_link_name", "Идеальное интегрирующее (астатическое)");
            break;
        case 1 + 3 * 64:
            name = QCoreApplication::translate("tf_link_name", "Инерционное 1-го порядка (апериодическое)");
            break;
        case 1 + 5 * 64:
            name = QCoreApplication::translate("tf_link_name", "Вырожденное колебательное (консервативное)");
            break;
        case 1 + 6 * 64:
            name = QCoreApplication::translate("tf_link_name", "Реальное интегрирующее (инерционное)");
            break;
        case 1 + 7 * 64: {
            const double a1 = coeff_of(den, 1);
            const double a2 = coeff_of(den, 2);
            if (a2 > 0.0 && a1 / (2.0 * std::sqrt(a2)) < 1.0)
                name = QCoreApplication::translate("tf_link_name", "Колебательное");
            else
                name = QCoreApplication::translate("tf_link_name", "Инерционное 2-го порядка (апериодическое)");
            break;
        }
        case 2 + 1 * 64:
            name = QCoreApplication::translate("tf_link_name", "Идеальное дифференцирующее");
            break;
        case 2 + 3 * 64:
            name = QCoreApplication::translate("tf_link_name", "Инерционное (реальное) дифференцирующее");
            break;
        case 4 + 3 * 64:
            name = QCoreApplication::translate("tf_link_name", "Реальное дифференцирующее 2-го порядка");
            break;
        case 3 + 2 * 64:
            name = QCoreApplication::translate("tf_link_name", "Изодромное");
            break;
        case 3 + 1 * 64:
            name = QCoreApplication::translate("tf_link_name", "Форсирующее");
            break;
        case 3 + 3 * 64:
            name = QCoreApplication::translate("tf_link_name", "Инерционно-форсирующее");
            break;
        case 63 + 21 * 64:
            name = QCoreApplication::translate("tf_link_name", "Пропорционально-дифференциальное 2-го порядка");
            break;
        default:
            name = QCoreApplication::translate("tf_link_name", "Неизвестно");
            break;
    }
    if (tau > 0.0)
        name += QCoreApplication::translate("tf_link_name", " с\u00A0запаздыванием");
    return name;
}
} // namespace tf_link_name
