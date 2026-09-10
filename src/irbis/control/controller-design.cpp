#include "irbis/control/controller-design.hpp"

#include <cmath>

namespace controller_design {
namespace {
std::vector<Settings> sample_band(const Designer& des, const Law law, double lo, double hi, const bool face,
                                  std::size_t n_points) {
    std::vector<Settings> out;
    if (!(hi > lo) || !std::isfinite(lo) || !std::isfinite(hi))
        return out;
    if (n_points < 2)
        n_points = 2;
    out.reserve(n_points);
    for (std::size_t i = 0; i < n_points; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(n_points - 1);
        const double w = lo + t * (hi - lo);
        if (!std::isfinite(w) || (law == Law::Pd && !(w > 0.0)))
            continue;
        const auto s = face ? des.settingsOnFace(w) : des.settingsAt(law, w);
        if (!std::isfinite(s.c1))
            continue;
        if (law == Law::Pd) {
            if (!std::isfinite(s.c2))
                continue;
        }
        else if (!std::isfinite(s.c0))
            continue;
        out.push_back(s);
    }
    return out;
}
}

bool usesPidFace(const Designer& des, const Law law, const Designer::Type w_hi_hint) noexcept {
    if (law == Law::Pid)
        return true;
    return law == Law::Auto && des.needsPid(w_hi_hint);
}

std::vector<Settings> locus(const Designer& des, const Law law, std::size_t n_points) {
    if (!drawsRegion(law))
        return {};

    if (law == Law::Pd) {
        const auto bands = des.pdPositiveBands(/*w_hi_hint=*/1e3);
        if (bands.empty())
            return {};
        return sample_band(des, Law::Pd, bands.front().first, bands.front().second, false, n_points);
    }

    const bool face = usesPidFace(des, law);
    double w_hi     = des.maxFrequency(/*w_hi_hint=*/1e3);
    if (!(w_hi > 1e-9) || !std::isfinite(w_hi)) {
        if (!face)
            return {};
        w_hi = 1e3;
    }
    return sample_band(des, Law::Pi, 0.0, w_hi, face, n_points);
}

Design run(const Designer& des, Spec spec, const Criterion criterion) {
    spec.criterion = criterion;
    return des.designByRkch(spec);
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
    b.face   = usesPidFace(des, spec.law);
    b.ikk    = run(des, spec, Criterion::Ikk);
    b.sko    = run(des, spec, Criterion::Sko);
    b.lik    = run(des, spec, Criterion::Lik);
    b.chosen = criterion;
    return b;
}
}
