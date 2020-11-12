/** \file
 * Common IR implementation.
 */

#include "ir.h"

#include "options.h"

namespace ql {
namespace ir {

using namespace utils;

/**
 * Create a circuit with valid cycle values from the bundled internal
 * representation.
 */
circuit circuiter(const bundles_t &bundles) {
    circuit circ;

    for (const bundle_t &abundle : bundles) {
        for (auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it) {
            for (auto gp : *sec_it) {
                gp->cycle = abundle.start_cycle;
                circ.push_back(gp);
            }
        }
    }
    // the bundles are in increasing order of their start_cycle
    // so the gates in the new circuit are also in non-decreasing cycle value order;
    // hence it doesn't need to be sorted and is valid
    // FIXME HvS cycles_valid should be set after this
    return circ;
}

/**
 * Create a bundled-qasm external representation from the bundled internal
 * representation.
 */
Str qasm(const bundles_t &bundles) {
    StrStrm ssqasm;
    UInt curr_cycle=1;        // FIXME HvS prefer to start at 0; also see depgraph creation
    Str skipgate = "wait";
    if (options::get("issue_skip_319") == "yes") {
        skipgate = "skip";
    }

    for (const bundle_t &abundle : bundles) {
        auto st_cycle = abundle.start_cycle;
        auto delta = st_cycle - curr_cycle;
        if (delta > 1) {
            ssqasm << "    " << skipgate << " " << delta - 1 << std::endl;
        }

        auto ngates = 0;
        for (auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it) {
            ngates += sec_it->size();
        }
        ssqasm << "    ";
        if (ngates > 1) ssqasm << "{ ";
        auto isfirst = 1;
        for (auto sec_it = abundle.parallel_sections.begin(); sec_it != abundle.parallel_sections.end(); ++sec_it) {
            for (auto gp : *sec_it) {
                if (isfirst == 0) {
                    ssqasm << " | ";
                }
                ssqasm << gp->qasm();
                isfirst = 0;
            }
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
 * Create a bundled internal representation from the circuit with valid cycle
 * information.
 *
 * assumes gatep->cycle attribute reflects the cycle assignment;
 * assumes circuit being a vector of gate pointers is ordered by this cycle value;
 * create bundles in a single scan over the circuit, using currBundle and currCycle as state:
 *  - currBundle: bundle that is being put together; currBundle copied into output bundles when the bundle has been done
 *  - currCycle: cycle at which currBundle will be put; equals cycle value of all contained gates
 *
 * FIXME HvS cycles_valid must be true before each call to this bundler
 */
bundles_t bundler(const circuit &circ, UInt cycle_time) {
    bundles_t bundles;          // result bundles

    bundle_t    currBundle;     // current bundle at currCycle that is being filled
    UInt      currCycle = 0;  // cycle at which bundle is to be scheduled

    currBundle.start_cycle = currCycle; // starts off as empty bundle starting at currCycle
    currBundle.duration_in_cycles = 0;

    QL_DOUT("bundler ...");

    for (auto &gp : circ) {
        QL_DOUT(". adding gate(@" << gp->cycle << ")  " << gp->qasm());
        if (gp->type() == gate_type_t::__wait_gate__ ||    // FIXME HvS: wait must be written as well
            gp->type() == gate_type_t::__dummy_gate__
        ) {
            QL_DOUT("... ignoring: " << gp->qasm());
            continue;
        }
        UInt newCycle = gp->cycle;        // taking cycle values from circuit, so excludes SOURCE and SINK!
        if (newCycle < currCycle) {
            QL_FATAL("Error: circuit not ordered by cycle value");
        }
        if (newCycle > currCycle) {
            if (!currBundle.parallel_sections.empty()) {
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
                currBundle.parallel_sections.clear();
            }

            // new empty currBundle at newCycle
            currCycle = newCycle;
            // DOUT(".. bundling at cycle: " << currCycle);
            currBundle.start_cycle = currCycle;
            currBundle.duration_in_cycles = 0;
        }

        // add gp to currBundle
        section_t asec;
        asec.push_back(gp);
        currBundle.parallel_sections.push_back(asec);
        // DOUT("... gate: " << gp->qasm() << " in private parallel section");
        currBundle.duration_in_cycles = max(currBundle.duration_in_cycles, (gp->duration+cycle_time-1)/cycle_time);
    }
    if (!currBundle.parallel_sections.empty()) {
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
void DebugBundles(const Str &at, const bundles_t &bundles) {
    QL_DOUT("DebugBundles at: " << at << " showing " << bundles.size() << " bundles");
    for (const auto& abundle : bundles) {
        QL_DOUT("... bundle with nsections: " << abundle.parallel_sections.size());
        for (auto secIt = abundle.parallel_sections.begin(); secIt != abundle.parallel_sections.end(); ++secIt) {
            QL_DOUT("... section with ngates: " << secIt->size());
            for (auto gp : *secIt) {
                // auto n = get_cc_light_instruction_name(gp->name, platform);
                QL_DOUT("... ... gate: " << gp->qasm() << " name: " << gp->name << " cc_light_iname: " << "?");
            }
        }
    }
}

} // namespace ir
} //namespace ql
