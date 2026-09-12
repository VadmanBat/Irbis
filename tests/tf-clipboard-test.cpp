#include "irbis/util/tf-clipboard.hpp"

#include "irbis/util/format.hxx"

#include <cstdio>
#include <cstdlib>
#include <QString>

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

void expect_eq(const char* name, double got, double expected) {
    if (got != expected) {
        std::fprintf(stderr, "FAIL %s: got %g, expected %g\n", name, got, expected);
        ++g_failed;
    }
    else {
        std::printf("ok   %s\n", name);
    }
}
}

int main() {
    tf_clipboard::Vec high;
    QString err;
    expect_true("high-first 1 2 3", tf_clipboard::parsePolyLine(QStringLiteral("1 2 3"), true, high, &err));
    expect_true("high size 3", high.size() == 3);
    expect_eq("high[0]", high[0], 1.0);
    expect_eq("high[2]", high[2], 3.0);

    tf_clipboard::Vec low;
    expect_true("low-first 1 2 3", tf_clipboard::parsePolyLine(QStringLiteral("1 2 3"), false, low, &err));
    expect_true("low size 3", low.size() == 3);
    expect_eq("low[0] highest", low[0], 3.0);
    expect_eq("low[2] const", low[2], 1.0);
    expect_eq("low[1]", low[1], 2.0);

    tf_clipboard::Vec leading;
    expect_true("strip leading zeros", tf_clipboard::parsePolyLine(QStringLiteral("0 0 3 1"), true, leading, &err));
    expect_true("stripped size 2", leading.size() == 2);
    expect_eq("stripped lead", leading[0], 3.0);

    tf_clipboard::Vec zeros;
    expect_true("all zeros parse", tf_clipboard::parsePolyLine(QStringLiteral("0 0 0"), true, zeros, &err));
    expect_true("all zeros stripped empty", zeros.empty());

    const auto text = tf_clipboard::format({1.0, 2.0}, {1.0, 3.0, 2.0}, 0.5);
    const auto data = tf_clipboard::parse(text);
    expect_true("roundtrip ok", data.ok);
    expect_true("roundtrip num size", data.num.size() == 2);
    expect_eq("roundtrip tau", data.tau, 0.5);
    expect_eq("roundtrip den[0]", data.den[0], 1.0);

    expect_true("poly high skips zero", num_format::polyPlainHighFirst({1.0, 0.0, 2.0}) == QStringLiteral("p^2 + 2"));
    expect_true("poly low skips zero", num_format::polyPlainLowFirst({1.0, 0.0, 2.0}) == QStringLiteral("2 + p^2"));

    expect_true("bad token", !tf_clipboard::parsePolyLine(QStringLiteral("1 x 2"), true, high, &err));

    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}