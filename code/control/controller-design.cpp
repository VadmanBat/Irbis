#include "code/control/controller-design.hpp"

#include <cmath>

namespace controller_design {
bool usesPidFace(const Designer& des, const Law law, const Designer::Type w_hi_hint) noexcept {
    if (law == Law::Pid)
        return true;
    return law == Law::Auto && des.needsPid(w_hi_hint);
}

std::vector<Settings> locus(const Designer& des, const Law law, std::size_t n_points) {
    std::vector<Settings> out;
    const bool face = usesPidFace(des, law);
    double w_hi     = des.maxFrequency(/*w_hi_hint=*/1e3);
    if (!(w_hi > 1e-9) || !std::isfinite(w_hi)) {
        if (!face)
            return out;
        w_hi = 1e3;
    }

    if (n_points < 2)
        n_points = 2;
    out.reserve(n_points);
    for (std::size_t i = 0; i < n_points; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(n_points - 1);
        const auto s   = face ? des.settingsOnFace(t * w_hi) : des.settingsAt(t * w_hi);
        if (!std::isfinite(s.c0) || !std::isfinite(s.c1))
            continue;
        out.push_back(s);
    }
    return out;
}

Design run(const Designer& des, Spec spec, const Criterion criterion) {
    spec.criterion   = criterion;
    const bool gamma = spec.region == Region::Gamma;
    switch (spec.law) {
        case Law::Pid:
            return des.designPid(spec);
        case Law::Auto:
            if (des.needsPid(spec.w_hi_hint))
                return des.designPid(spec);
            return gamma ? des.designByGamma(spec) : des.designPi(spec);
        case Law::Pi:
        default:
            return gamma ? des.designByGamma(spec) : des.designPi(spec);
    }
}

const Design& Bundle::selected() const noexcept {
    switch (chosen) {
        case Criterion::Ikk:
            return ikk;
        case Criterion::Sko:
            return sko;
        case Criterion::Lik:
        default:
            return lik;
    }
}

Bundle synthesize(const Designer& des, const Spec& spec, Criterion criterion) {
    Bundle b;
    b.face     = usesPidFace(des, spec.law);
    b.gamma_pi = spec.region == Region::Gamma && !b.face;
    b.ikk      = run(des, spec, Criterion::Ikk);
    b.sko      = run(des, spec, Criterion::Sko);
    b.lik      = b.gamma_pi ? Design{} : run(des, spec, Criterion::Lik);
    if (b.gamma_pi && criterion == Criterion::Lik)
        criterion = Criterion::Ikk;
    b.chosen = criterion;
    return b;
}
}
