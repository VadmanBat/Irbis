#include "code/util/data-file-parser.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
int g_failed = 0;

void expect_true(const char* name, bool cond) {
    if (!cond) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

void expect_eq(const char* name, int got, int expected) {
    if (got != expected) {
        std::fprintf(stderr, "FAIL %s: got %d, expected %d\n", name, got, expected);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}

bool near(double a, double b) {
    return std::abs(a - b) <= 1e-12 * (1.0 + std::abs(b));
}
}

int main() {
    using data_file_parser::asPairs;
    using data_file_parser::extractNumbers;

    {
        const QString text = QString::fromUtf8(
            "Name: Инерционное 1-го порядка (апериодическое)\n"
            "0, 0\n"
            "1, 0.0951626\n"
            "2, 0.181269\n");
        const auto nums  = extractNumbers(text);
        const auto pairs = asPairs(nums);
        expect_eq("1-го header count", static_cast<int>(pairs.size()), 3);
        expect_true("1-го t0", !pairs.empty() && near(pairs[0].first, 0.0));
        expect_true("1-го h0", !pairs.empty() && near(pairs[0].second, 0.0));
        expect_true("1-го t1", pairs.size() > 1 && near(pairs[1].first, 1.0));
        expect_true("1-го h1", pairs.size() > 1 && near(pairs[1].second, 0.0951626));
    }

    {
        const QString text = QString::fromUtf8(
            "Name: Инерционное 2-го порядка (апериодическое)\n"
            "0, 0\n"
            "1, 0.0042578\n");
        const auto pairs = asPairs(extractNumbers(text));
        expect_eq("2-го header count", static_cast<int>(pairs.size()), 2);
        expect_true("2-го t0", !pairs.empty() && near(pairs[0].first, 0.0));
        expect_true("2-го t1", pairs.size() > 1 && near(pairs[1].first, 1.0));
    }

    {
        const QString text = QString::fromUtf8(
            "Name: Колебательное\n"
            "0, 0\n"
            "1, 0.00483342\n");
        const auto pairs = asPairs(extractNumbers(text));
        expect_eq("no-digit title count", static_cast<int>(pairs.size()), 2);
        expect_true("no-digit t0", !pairs.empty() && near(pairs[0].first, 0.0));
    }

    {
        const QString text = QString::fromUtf8("0, -2.55351e-15\n1.00503, 0\n");
        const auto pairs   = asPairs(extractNumbers(text));
        expect_eq("sci count", static_cast<int>(pairs.size()), 2);
        expect_true("sci h0", !pairs.empty() && near(pairs[0].second, -2.55351e-15));
        expect_true("sci t1", pairs.size() > 1 && near(pairs[1].first, 1.00503));
    }

    if (g_failed) {
        std::fprintf(stderr, "%d failed\n", g_failed);
        return EXIT_FAILURE;
    }
    std::printf("all ok\n");
    return EXIT_SUCCESS;
}
