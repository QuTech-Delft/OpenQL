/** \file
 * Common IR implementation.
 */

#include "ql/ir/bundle.h"

#include "ql/com/options.h"

namespace ql {
namespace ir {

using namespace utils;

/**
 * Create a circuit with valid cycle values from the bundled internal
 * representation. The bundles are assumed to be ordered by cycle number.
 */
ir::GateRefs circuiter(const ir::Bundles &bundles) {
    ir::GateRefs gates;

    UInt cycle = 0;
    for (const Bundle &bundle : bundles) {
        QL_ASSERT(bundle.start_cycle > cycle);
        cycle = bundle.start_cycle;
        for (const auto &gate : bundle.gates) {
            gate->cycle = cycle;
            gates.add(gate);
        }
    }

    // the bundles are in increasing order of their start_cycle
    // so the gates in the new circuit are also in non-decreasing cycle value order;
    // hence it doesn't need to be sorted and is valid
    // FIXME HvS cycles_valid should be set after this
    return gates;
}

/**
 * Create a bundled-qasm external representation from the bundled internal
 * representation.
 */
Str qasm(const ir::Bundles &bundles) {
    StrStrm ssqasm;
    UInt curr_cycle=1;        // FIXME HvS prefer to start at 0; also see depgraph creation
    Str skipgate = "wait";
    if (com::options::get("issue_skip_319") == "yes") {
        skipgate = "skip";
    }

    for (const Bundle &bundle : bundles) {
        auto st_cycle = bundle.start_cycle;
        auto delta = st_cycle - curr_cycle;
        if (delta > 1) {
            ssqasm << "    " << skipgate << " " << delta - 1 << std::endl;
        }

        auto ngates = 0;
        ngates += bundle.gates.size();
        ssqasm << "    ";
        if (ngates > 1) ssqasm << "{ ";
        auto isfirst = 1;
        for (const auto &gate : bundle.gates) {
            if (isfirst == 0) {
                ssqasm << " | ";
            }
            ssqasm << gate->qasm();
            isfirst = 0;
        }
        if (ngates > 1) ssqasm << " }";
        curr_cycle+=delta;
        ssqasm << "\n";
    }

    if (!bundles.empty()) {
        auto &last_bundle = bundles.back();
        UInt lsduration = last_bundle.duration_in_cycles;
        if (lsduration > 1) {
            ssqasm << "    " << skipgate << " " << lsduration - 1 << std::endl;
        }
    }

    return ssqasm.str();
}

/**
 * Create a bundled internal representation from the given kernel with valid
 * cycle information.
 */
ir::Bundles bundler(const KernelRef &kernel) {
    QL_ASSERT(kernel->cycles_valid);

    auto cycle_time = kernel->platform->cycle_time;

    ir::Bundles bundles;        // result bundles

    ir::Bundle  currBundle;     // current bundle at currCycle that is being filled
    UInt        currCycle = 0;  // cycle at which bundle is to be scheduled

    currBundle.start_cycle = currCycle; // starts off as empty bundle starting at currCycle
    currBundle.duration_in_cycles = 0;

    QL_DOUT("bundler ...");

    // Create bundles in a single scan over the circuit, using currBundle and
    // currCycle as state:
    //  - currBundle: bundle that is being put together; currBundle copied into
    //    output bundles when the bundle has been done
    //  - currCycle: cycle at which currBundle will be put; equals cycle value
    //    of all contained gates

    for (auto &gp : kernel->gates) {
        QL_DOUT(". adding gate(@" << gp->cycle << ")  " << gp->qasm());
        if (gp->type() == GateType::WAIT ||    // FIXME HvS: wait must be written as well
            gp->type() == GateType::DUMMY
        ) {
            QL_DOUT("... ignoring: " << gp->qasm());
            continue;
        }
        UInt newCycle = gp->cycle;        // taking cycle values from circuit, so excludes SOURCE and SINK!
        if (newCycle < currCycle) {
            QL_FATAL("Error: circuit not ordered by cycle value");
        }
        if (newCycle > currCycle) {
            if (!currBundle.gates.empty()) {
                // finish currBundle at currCycle
                // DOUT(".. bundle at cycle " << currCycle << " duration in cycles: " << currBundle.duration_in_cycles);
                // for (auto &s : currBundle.parallel_sections)
                // {
                //     for (auto &sgp: s)
                //     {
                //         DOUT("... with gate(@" << sgp->cycle << ")  " << sgp->qasm());
                //     }
                // }
                bundles.push_back(currBundle);
                QL_DOUT(".. ready with bundle at cycle " << currCycle);
                currBundle.gates.clear();
            }

            // new empty currBundle at newCycle
            currCycle = newCycle;
            // DOUT(".. bundling at cycle: " << currCycle);
            currBundle.start_cycle = currCycle;
            currBundle.duration_in_cycles = 0;
        }

        // add gp to currBundle
        currBundle.gates.push_back(gp);
        // DOUT("... gate: " << gp->qasm() << " in private parallel section");
        currBundle.duration_in_cycles = max(currBundle.duration_in_cycles, (gp->duration+cycle_time-1)/cycle_time);
    }
    if (!currBundle.gates.empty()) {
        // finish currBundle (which is last bundle) at currCycle
        // DOUT("... bundle at cycle " << currCycle << " duration in cycles: " << currBundle.duration_in_cycles);
        // for (auto &s : currBundle.parallel_sections)
        // {
        //     for (auto &sgp: s)
        //     {
        //         DOUT("... with gate(@" << sgp->cycle << ")  " << sgp->qasm());
        //     }
        // }
        bundles.push_back(currBundle);
        QL_DOUT(".. ready with bundle at cycle " << currCycle);
    }

    // currCycle == cycle of last gate of circuit scheduled
    // duration_in_cycles later the system starts idling
    // depth is the difference between the cycle in which it starts idling and the cycle it started execution
    if (bundles.empty()) {
        QL_DOUT("Depth: " << 0);
    } else {
        QL_DOUT("Depth: " << currCycle + currBundle.duration_in_cycles - bundles.front().start_cycle);
    }
    QL_DOUT("bundler [DONE]");
    return bundles;
}

/**
 * Print the bundles with an indication (taken from 'at') from where this
 * function was called.
 */
void debug_bundles(const Str &at, const ir::Bundles &bundles) {
    QL_DOUT("debug_bundles at: " << at << " showing " << bundles.size() << " bundles");
    for (const auto &bundle : bundles) {
        QL_DOUT("... bundle with ngates: " << bundle.gates.size());
        for (const auto &gate : bundle.gates) {
            // auto n = get_cc_light_instruction_name(gp->name, platform);
            QL_DOUT("... ... gate: " << gate->qasm() << " name: " << gate->name << " cc_light_iname: " << "?");
        }
    }
}

} // namespace ir
} //namespace ql
