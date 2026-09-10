#include "irbis/control/controller-design.hpp"

#include "numina/classes/control/models/transfer-function.h"
#include "numina/classes/polynomial/polynomial.h"

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
}

int main() {
    using controller_design::Law;
    using numina::ControllerDesigner;
    using numina::Polynomial;
    using numina::TransferFunction;

    expect_true("drawsRegion P", !controller_design::drawsRegion(Law::P));
    expect_true("drawsRegion I", !controller_design::drawsRegion(Law::I));
    expect_true("drawsRegion Pd", controller_design::drawsRegion(Law::Pd));
    expect_true("drawsRegion Pi", controller_design::drawsRegion(Law::Pi));

    const TransferFunction so(Polynomial({1}), Polynomial({4, 3, 1}));
    ControllerDesigner des(so, 0.3);

    expect_true("P locus empty", controller_design::locus(des, Law::P).empty());
    expect_true("I locus empty", controller_design::locus(des, Law::I).empty());

    const auto pd = controller_design::locus(des, Law::Pd);
    expect_true("PD locus nonempty", !pd.empty());
    bool pd_ok  = !pd.empty();
    bool pd_in  = false;
    for (const auto& s : pd) {
        if (s.law != Law::Pd || !std::isfinite(s.c1) || !std::isfinite(s.c2) || s.c1 < 0.0 || s.c2 < 0.0)
            pd_ok = false;
        if (s.c1 > 0.0 && s.c2 > 0.0)
            pd_in = true;
    }
    expect_true("PD locus C1,C2 >= 0", pd_ok);
    expect_true("PD locus interior C1>0 C2>0", pd_in);

    expect_true("PI locus nonempty", !controller_design::locus(des, Law::Pi).empty());

    const TransferFunction fo(Polynomial({1}), Polynomial({2, 1}));
    ControllerDesigner des_fo(fo, 0.3);
    expect_true("FO PD locus empty", controller_design::locus(des_fo, Law::Pd).empty());

    if (g_failed != 0) {
        std::fprintf(stderr, "%d failed\n", g_failed);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
