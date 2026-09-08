#pragma once

#include <cmath>
#include <limits>
#include <QString>
#include <QStringList>
#include <sstream>
#include <string>
#include <vector>

namespace num_format {
/// Display significant digits (C++ iostream defaultfloat style).
inline constexpr int SIGNIFICANT_DIGITS = 6;
/// Stored user precision: keep 16 significant digits, round the 17th.
inline constexpr int STORED_DIGITS = 16;
/// Clipboard / interchange: full double precision.
inline constexpr int FULL_DIGITS = std::numeric_limits<double>::max_digits10; // typically 17

/// Round to `digits` significant decimal digits (half away from zero via std::round).
[[nodiscard]] inline double roundSignificant(double value, int digits = STORED_DIGITS) noexcept {
    if (!std::isfinite(value) || value == 0.0 || digits < 1)
        return value;
    const double ax    = std::abs(value);
    const double exp10 = std::floor(std::log10(ax));
    const double scale = std::pow(10.0, static_cast<double>(digits) - 1.0 - exp10);
    return std::copysign(std::round(ax * scale) / scale, value);
}

/// Format like `std::ostream << setprecision(digits) << defaultfloat`:
/// decimal when short, scientific when needed.
[[nodiscard]] inline QString format(double value, int digits = SIGNIFICANT_DIGITS) {
    if (!std::isfinite(value)) {
        if (std::isnan(value))
            return QStringLiteral("nan");
        return value > 0 ? QStringLiteral("inf") : QStringLiteral("-inf");
    }
    if (value == 0.0)
        return QStringLiteral("0");

    std::ostringstream os;
    os.precision(digits);
    // defaultfloat is the default for ostream
    os << value;
    std::string s = os.str();
    // Normalize locale decimal comma if any
    for (char& c : s) {
        if (c == ',')
            c = '.';
    }
    return QString::fromStdString(s);
}

[[nodiscard]] inline QString formatFull(double value) {
    return format(value, FULL_DIGITS);
}

/// Coefficient display: optional leading '+' for positive (TF form UX).
[[nodiscard]] inline QString formatCoeff(double value, int digits = SIGNIFICANT_DIGITS, bool plus_sign = true) {
    if (value == 0.0)
        return plus_sign ? QStringLiteral("+0") : QStringLiteral("0");
    QString s = format(value, digits);
    if (plus_sign && !s.startsWith(QLatin1Char('-')) && !s.startsWith(QLatin1Char('+')))
        s.prepend(QLatin1Char('+'));
    return s;
}

/// Parse user/file token: '.' or ',' decimal, scientific notation, optional lone '+' / '-'.
[[nodiscard]] inline double parse(QString text, bool* ok = nullptr) {
    text = text.trimmed();
    text.replace(',', '.');
    text.remove(' ');
    if (text.isEmpty()) {
        if (ok)
            *ok = false;
        return 0.0;
    }
    // Lone sign → treat as ±1 (legacy coeff UX)
    if (text == QLatin1String("+") || text == QLatin1String("-")) {
        if (ok)
            *ok = true;
        return text.startsWith(QLatin1Char('-')) ? -1.0 : 1.0;
    }
    bool local_ok  = false;
    const double v = text.toDouble(&local_ok);
    if (ok)
        *ok = local_ok;
    if (!local_ok)
        return 0.0;
    return roundSignificant(v, STORED_DIGITS);
}

/// HTML poly term powers: low→high display  c0 + c1·p + c2·p<sup>2</sup> …
[[nodiscard]] inline QString polyHtmlLowFirst(const std::vector<double>& high_to_low, int digits = SIGNIFICANT_DIGITS) {
    if (high_to_low.empty())
        return QStringLiteral("0");

    // Convert high→low to low→high for display order 1 + p + p²
    const int deg = static_cast<int>(high_to_low.size()) - 1;
    QStringList terms;
    for (int power = 0; power <= deg; ++power) {
        const double c = high_to_low[static_cast<std::size_t>(deg - power)];
        if (c == 0.0)
            continue;
        QString term;
        if (power == 0) {
            term = format(c, digits);
        }
        else if (power == 1) {
            if (c == 1.0)
                term = QStringLiteral("p");
            else if (c == -1.0)
                term = QStringLiteral("−p");
            else
                term = format(c, digits) + QStringLiteral("·p");
        }
        else {
            if (c == 1.0)
                term = QStringLiteral("p<sup>%1</sup>").arg(power);
            else if (c == -1.0)
                term = QStringLiteral("−p<sup>%1</sup>").arg(power);
            else
                term = format(c, digits) + QStringLiteral("·p<sup>%1</sup>").arg(power);
        }
        terms.push_back(term);
    }
    if (terms.isEmpty())
        return QStringLiteral("0");

    QString out       = terms.front();
    const int n_terms = terms.size();
    for (int i = 1; i < n_terms; ++i) {
        const QString& t = terms[i];
        if (t.startsWith(QLatin1Char('-')) || t.startsWith(QStringLiteral("−")))
            out += QStringLiteral(" ") + t;
        else
            out += QStringLiteral(" + ") + t;
    }
    return out;
}

/// Plain (non-HTML) poly for clipboard human line, low→high.
[[nodiscard]] inline QString polyPlainLowFirst(const std::vector<double>& high_to_low,
                                               int digits = SIGNIFICANT_DIGITS) {
    QString html = polyHtmlLowFirst(high_to_low, digits);
    html.replace(QStringLiteral("<sup>"), QStringLiteral("^"));
    html.replace(QStringLiteral("</sup>"), QString());
    return html;
}
} // namespace num_format
