#pragma once

#include "numina/classes/control/controller-designer.h"

#include <vector>

namespace controller_design {
using Designer  = numina::ControllerDesigner;
using Spec      = Designer::Spec;
using Design    = Designer::Design;
using Law       = Designer::Law;
using Criterion = Designer::Criterion;
using Region    = Designer::Region;
using Settings  = Designer::Settings;

[[nodiscard]] bool usesPidFace(const Designer& des, Law law, Designer::Type w_hi_hint = 1e3) noexcept;

[[nodiscard]] std::vector<Settings> locus(const Designer& des, Law law, std::size_t n_points = 160);

[[nodiscard]] Design run(const Designer& des, Spec spec, Criterion criterion);

struct Bundle {
    Design lik;
    Design ikk;
    Design sko;
    Criterion chosen{Criterion::Lik};
    bool gamma_pi{false};
    bool face{false};

    [[nodiscard]] const Design& selected() const noexcept;
};

[[nodiscard]] Bundle synthesize(const Designer& des, const Spec& spec, Criterion criterion);
}
