/** \file
 * Latency compensation?
 */

#include "latency_compensation.h"

#include "utils/num.h"
#include "gate.h"
#include "kernel.h"
#include "circuit.h"
#include "report.h"

namespace ql {

using namespace utils;

static Bool lc_cycle_lessthan(gate* gp1, gate* gp2) {
    return gp1->cycle < gp2->cycle;
}

// sort circuit by the gates' cycle attribute in non-decreasing order
static void lc_sort_by_cycle(circuit *cp) {
    // std::sort doesn't preserve the original order of elements that have equal values but std::stable_sort does
    std::stable_sort(cp->begin(), cp->end(), lc_cycle_lessthan);
}

void latency_compensation_kernel(
    quantum_kernel &kernel,
    const quantum_platform &platform
) {
    QL_DOUT("Latency compensation ...");

    circuit* circp = &kernel.c;

    Bool compensated_one = false;
    for (auto &gp : *circp) {
        auto &id = gp->name;
        // DOUT("Latency compensating instruction: " << id);
        Int latency_cycles = 0;

        if (platform.instruction_settings.count(id) > 0) {
            if (platform.instruction_settings[id].count("latency") > 0) {
                Real latency_ns = platform.instruction_settings[id]["latency"];
                latency_cycles = Int(round_away_from_zero(latency_ns / platform.cycle_time));
                compensated_one = true;

                gp->cycle = gp->cycle + latency_cycles;
                QL_DOUT("... compensated to @" << gp->cycle << " <- " << id << " with " << latency_cycles );
            }
        }
    }

    if (compensated_one) {
        QL_DOUT("... sorting on cycle value after latency compensation");
        lc_sort_by_cycle(circp);

        QL_DOUT("... printing schedule after latency compensation");
        for (auto &gp : *circp) {
            QL_DOUT("...... @(" << gp->cycle << "): " << gp->qasm());
        }
    } else {
        QL_DOUT("... no gate latency compensated");
    }
    QL_DOUT("Latency compensation [DONE]");
}

void latency_compensation(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    for (UInt k = 0; k < programp->kernels.size(); ++k) {
        latency_compensation_kernel(programp->kernels[k], platform);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

} // namespace ql
