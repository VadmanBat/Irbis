#pragma once

#include "irbis/util/format.hxx"

#include <algorithm>
#include <cstddef>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <vector>

/// Irbis-TF-v1 clipboard interchange (also reads legacy RegValve-TF-v1).
/// Coefficient vectors are high→low (leading = highest power), leading zeros stripped.
namespace tf_clipboard {
using Vec = std::vector<double>;

struct Data {
    Vec num;
    Vec den;
    double tau{0.0};
    bool ok{false};
};

inline void stripLeadingZeros(Vec& v) {
    std::size_t i = 0;
    while (i + 1 < v.size() && v[i] == 0.0)
        ++i;
    if (i > 0)
        v.erase(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(i));
    if (v.size() == 1 && v[0] == 0.0)
        v.clear();
}

/// Parse space-separated numbers. Comma is a decimal mark, not a separator.
[[nodiscard]] inline bool parseCoeffLine(QString line, Vec& out, QString* error = nullptr) {
    out.clear();
    line.replace(';', ' ');
    const auto parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        if (error)
            *error = QObject::tr("Введите коэффициенты через пробел.");
        return false;
    }
    out.reserve(static_cast<std::size_t>(parts.size()));
    for (const QString& p : parts) {
        bool ok        = false;
        const double v = num_format::parse(p, &ok);
        if (!ok) {
            if (error)
                *error = QObject::tr("Некорректное число: «%1».").arg(p);
            out.clear();
            return false;
        }
        out.push_back(v);
    }
    return true;
}

/// `high_first`: first token is the highest power (MATLAB). Otherwise first token is p⁰.
[[nodiscard]] inline bool parsePolyLine(const QString& line, bool high_first, Vec& high_to_low,
                                        QString* error = nullptr) {
    if (!parseCoeffLine(line, high_to_low, error))
        return false;
    if (!high_first)
        std::reverse(high_to_low.begin(), high_to_low.end());
    stripLeadingZeros(high_to_low);
    return true;
}

[[nodiscard]] inline QString formatCoeffLine(const Vec& high_to_low, bool high_first,
                                             int digits = num_format::STORED_DIGITS) {
    if (high_to_low.empty())
        return {};
    Vec ordered = high_to_low;
    if (!high_first)
        std::reverse(ordered.begin(), ordered.end());
    QStringList parts;
    parts.reserve(static_cast<int>(ordered.size()));
    for (double v : ordered)
        parts << num_format::format(v, digits);
    return parts.join(QLatin1Char(' '));
}

[[nodiscard]] inline QString format(const Vec& num, const Vec& den, double tau,
                                    const QString& lhs = QStringLiteral("W(p)")) {
    QStringList num_parts;
    QStringList den_parts;
    for (double v : num)
        num_parts << num_format::formatFull(v);
    for (double v : den)
        den_parts << num_format::formatFull(v);

    QString human = QStringLiteral("%1 = (%2) / (%3)")
                        .arg(lhs, num_format::polyPlainLowFirst(num, num_format::SIGNIFICANT_DIGITS),
                             num_format::polyPlainLowFirst(den, num_format::SIGNIFICANT_DIGITS));
    if (tau > 0.0)
        human += QStringLiteral(" · e^(-%1 p)").arg(num_format::format(tau, num_format::SIGNIFICANT_DIGITS));

    return QStringLiteral(
               "Irbis-TF-v1\n"
               "num: %1\n"
               "den: %2\n"
               "tau: %3\n"
               "\n"
               "%4\n")
        .arg(num_parts.join(QLatin1Char(' ')), den_parts.join(QLatin1Char(' ')), num_format::formatFull(tau), human);
}

[[nodiscard]] inline Data parse(const QString& text) {
    Data data;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
    bool has_header         = false;
    bool got_num            = false;
    bool got_den            = false;

    for (QString line : lines) {
        line = line.trimmed();
        if (line.startsWith(QStringLiteral("Irbis-TF-v1"), Qt::CaseInsensitive) ||
            line.startsWith(QStringLiteral("RegValve-TF-v1"), Qt::CaseInsensitive)) {
            has_header = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("num:"), Qt::CaseInsensitive)) {
            QString err;
            got_num = parsePolyLine(line.mid(4), true, data.num, &err);
            continue;
        }
        if (line.startsWith(QStringLiteral("den:"), Qt::CaseInsensitive)) {
            QString err;
            got_den = parsePolyLine(line.mid(4), true, data.den, &err);
            continue;
        }
        if (line.startsWith(QStringLiteral("tau:"), Qt::CaseInsensitive)) {
            bool ok  = false;
            data.tau = num_format::parse(line.mid(4), &ok);
            if (!ok || data.tau < 0.0)
                data.tau = 0.0;
            continue;
        }
    }
    data.ok = has_header && got_num && got_den && !data.num.empty() && !data.den.empty();
    return data;
}
} // namespace tf_clipboard
