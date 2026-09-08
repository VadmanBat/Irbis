#pragma once

#include <cstdlib>
#include <optional>
#include <QFile>
#include <QString>
#include <QStringView>
#include <utility>
#include <vector>

namespace data_file_parser {
using Pair    = std::pair<double, double>;
using VecPair = std::vector<Pair>;
using Vec     = std::vector<double>;

/// Title / comment line: any letter except scientific e/E.
/// «Name: … 1-го порядка» must not contribute a stray 1 and shift (t,h) pairs.
inline bool is_data_line(QStringView line) noexcept {
    for (const QChar c : line) {
        if (c.isLetter() && c.toLower() != QLatin1Char('e'))
            return false;
    }
    return true;
}

/// Scan a chunk for floating-point tokens. Accepts '.' and ',' as decimal separators;
/// other characters are ignored.
inline Vec extract_numbers_chunk(const QString& text) {
    Vec out;
    const QByteArray utf8 = text.toUtf8();
    const char* s         = utf8.constData();
    const char* end       = s + utf8.size();

    auto isDigit = [](unsigned char c) { return c >= '0' && c <= '9'; };

    while (s < end) {
        // Skip until a plausible number start
        while (s < end) {
            const unsigned char c = static_cast<unsigned char>(*s);
            if (isDigit(c) || *s == '.' || *s == ',')
                break;
            if ((*s == '+' || *s == '-') && s + 1 < end) {
                const unsigned char n = static_cast<unsigned char>(s[1]);
                if (isDigit(n) || s[1] == '.' || s[1] == ',')
                    break;
            }
            ++s;
        }
        if (s >= end)
            break;

        const char* start = s;
        bool sawDigit     = false;
        bool sawDot       = false;

        if (*s == '+' || *s == '-')
            ++s;

        // Integer / fractional part: digits with at most one decimal separator (. or ,)
        while (s < end) {
            const unsigned char c = static_cast<unsigned char>(*s);
            if (isDigit(c)) {
                sawDigit = true;
                ++s;
                continue;
            }
            if ((*s == '.' || *s == ',') && !sawDot) {
                // Decimal only if a digit follows (or already saw integer digits: "12," trailing ok)
                if (s + 1 < end && isDigit(static_cast<unsigned char>(s[1]))) {
                    sawDot = true;
                    ++s;
                    continue;
                }
                // "12," as separator after integer — stop before comma
                if (sawDigit)
                    break;
                // leading "." / "," only with following digit (handled above) — otherwise skip
                break;
            }
            break;
        }

        // Exponent
        if (sawDigit && s < end && (*s == 'e' || *s == 'E')) {
            const char* expStart = s;
            ++s;
            if (s < end && (*s == '+' || *s == '-'))
                ++s;
            const char* digStart = s;
            while (s < end && isDigit(static_cast<unsigned char>(*s)))
                ++s;
            if (s == digStart)
                s = expStart; // no exponent digits — roll back
        }

        if (!sawDigit) {
            // lone '.' / ',' — advance one char to avoid infinite loop
            s = (start + 1 < end) ? start + 1 : end;
            continue;
        }

        // Local buffer: avoid QString alloc for every token
        char buf[64];
        const auto n = static_cast<std::size_t>(s - start);
        if (n == 0 || n >= sizeof(buf))
            continue;
        for (std::size_t i = 0; i < n; ++i)
            buf[i] = (start[i] == ',') ? '.' : start[i];
        buf[n] = '\0';

        char* endptr   = nullptr;
        const double v = std::strtod(buf, &endptr);
        if (endptr != buf)
            out.push_back(v);
    }
    out.shrink_to_fit();
    return out;
}

/// Line-wise extract: skip titles/comments, then scan numbers on data rows.
inline Vec extractNumbers(const QString& text) {
    Vec out;
    qsizetype from          = 0;
    const qsizetype n_chars = text.size();
    while (from <= n_chars) {
        const qsizetype nl = text.indexOf(QLatin1Char('\n'), from);
        const qsizetype to = nl < 0 ? n_chars : nl;
        QStringView line(text.constData() + from, to - from);
        if (!line.isEmpty() && line.back() == QLatin1Char('\r'))
            line.chop(1);
        if (is_data_line(line) && !line.trimmed().isEmpty()) {
            const auto row = extract_numbers_chunk(QString{line});
            out.insert(out.end(), row.begin(), row.end());
        }
        if (nl < 0)
            break;
        from = to + 1;
    }
    out.shrink_to_fit();
    return out;
}

inline Vec extractNumbersFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return extractNumbers(QString::fromUtf8(file.readAll()));
}

/// time, value pairs (odd leftover numbers discarded).
inline VecPair asPairs(const Vec& numbers) {
    VecPair res;
    const std::size_t n = numbers.size();
    res.reserve(n / 2);
    for (std::size_t i = 0; i + 1 < n; i += 2)
        res.emplace_back(numbers[i], numbers[i + 1]);
    return res;
}

/// time, valve, value triples → (time,valve) and (time,value).
inline bool asValveSignal(const Vec& numbers, VecPair& valve, VecPair& signal) {
    valve.clear();
    signal.clear();
    const std::size_t n = numbers.size();
    if (n < 6)
        return false;
    const std::size_t triples = n / 3;
    valve.reserve(triples);
    signal.reserve(triples);
    for (std::size_t i = 0; i + 2 < n; i += 3) {
        const double t = numbers[i];
        valve.emplace_back(t, numbers[i + 1]);
        signal.emplace_back(t, numbers[i + 2]);
    }
    return valve.size() >= 2;
}

inline std::optional<VecPair> readStepResponse(const QString& filePath) {
    auto pairs = asPairs(extractNumbersFromFile(filePath));
    if (pairs.size() < 2)
        return std::nullopt;
    return pairs;
}
} // namespace data_file_parser
