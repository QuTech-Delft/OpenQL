/** \file
 * FreeCycle implementation.
 */

#include "free_cycle.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

using namespace utils;
using namespace com;

// access free cycle value of qubit q[i] or breg b[i-nq]
UInt &FreeCycle::operator[](UInt i) {
    return fcv[i];
}

const UInt &FreeCycle::operator[](UInt i) const {
    return fcv[i];
}

// explicit FreeCycle constructor
// needed for virgin construction
// default constructor was deleted because it cannot construct resource_manager_t without parameters
FreeCycle::FreeCycle() {
    QL_DOUT("Constructing FreeCycle");
}

void FreeCycle::Init(const plat::PlatformRef &p, const OptionsRef &opt) {
    QL_DOUT("FreeCycle::Init()");
    auto rm = plat::resource::Manager::from_defaults(p);   // allocated here and copied below to rm because of platform parameter
                                                           // JvS: I have no idea what ^ means
    QL_DOUT("... created FreeCycle Init local resource_manager");
    options = opt;
    platformp = p;
    nq = platformp->qubit_count;
    nb = platformp->breg_count;
    ct = platformp->cycle_time;
    QL_DOUT("... FreeCycle: nq=" << nq << ", nb=" << nb << ", ct=" << ct << "), initializing to all 0 cycles");
    fcv.clear();
    fcv.resize(nq+nb, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
    QL_DOUT("... about to copy FreeCycle Init local resource_manager to FreeCycle member rm");
    rs = rm.build(plat::resource::Direction::FORWARD);
    QL_DOUT("... done copy FreeCycle Init local resource_manager to FreeCycle member rm");
}

// depth of the FreeCycle map
// equals the max of all entries minus the min of all entries
// not used yet; would be used to compute the max size of a top window on the past
UInt FreeCycle::Depth() const {
    return Max() - Min();
}

// min of the FreeCycle map equals the min of all entries;
UInt FreeCycle::Min() const {
    UInt  minFreeCycle = ir::MAX_CYCLE;
    for (const auto &v : fcv) {
        if (v < minFreeCycle) {
            minFreeCycle = v;
        }
    }
    return minFreeCycle;
}

// max of the FreeCycle map equals the max of all entries;
UInt FreeCycle::Max() const {
    UInt maxFreeCycle = 0;
    for (const auto &v : fcv) {
        if (maxFreeCycle < v) {
            maxFreeCycle = v;
        }
    }
    return maxFreeCycle;
}

void FreeCycle::DPRINT(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s);
    }
}

void FreeCycle::Print(const Str &s) const {
    UInt  minFreeCycle = Min();
    UInt  maxFreeCycle = Max();
    std::cout << "... FreeCycle" << s << ":";
    for (UInt i = 0; i < nq; i++) {
        UInt v = fcv[i];
        std::cout << " [" << i << "]=";
        if (v == minFreeCycle) {
            std::cout << "_";
        }
        if (v == maxFreeCycle) {
            std::cout << "^";
        }
        std::cout << v;
    }
    std::cout << std::endl;
    // rm.Print("... in FreeCycle: ");
}

// return whether gate with first operand qubit r0 can be scheduled earlier than with operand qubit r1
Bool FreeCycle::IsFirstOperandEarlier(UInt r0, UInt r1) const {
    QL_DOUT("... fcv[" << r0 << "]=" << fcv[r0] << " fcv[" << r1 << "]=" << fcv[r1] << " IsFirstOperandEarlier=" << (fcv[r0] < fcv[r1]));
    return fcv[r0] < fcv[r1];
}

// will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
// is really a short-cut ignoring config file and perhaps several other details
Bool FreeCycle::IsFirstSwapEarliest(UInt fr0, UInt fr1, UInt sr0, UInt sr1) const {
    if (options->reverse_swap_if_better) {
        if (fcv[fr0] < fcv[fr1]) {
            UInt  tmp = fr1; fr1 = fr0; fr0 = tmp;
        }
        if (fcv[sr0] < fcv[sr1]) {
            UInt  tmp = sr1; sr1 = sr0; sr0 = tmp;
        }
    }
    UInt startCycleFirstSwap = max(fcv[fr0]-1, fcv[fr1]);
    UInt startCycleSecondSwap = max(fcv[sr0]-1, fcv[sr1]);

    QL_DOUT("... fcv[" << fr0 << "]=" << fcv[fr0] << " fcv[" << fr1 << "]=" << fcv[fr1] << " start=" << startCycleFirstSwap << " fcv[" << sr0 << "]=" << fcv[sr0] << " fcv[" << sr1 << "]=" << fcv[sr1] << " start=" << startCycleSecondSwap << " IsFirstSwapEarliest=" << (startCycleFirstSwap < startCycleSecondSwap));
    return startCycleFirstSwap < startCycleSecondSwap;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// is purely functional, doesn't affect state
UInt FreeCycle::StartCycleNoRc(const ir::GateRef &g) const {
    UInt startCycle = 1;
    for (auto qreg : g->operands) {
        startCycle = max(startCycle, fcv[qreg]);
    }
    for (auto breg : g->breg_operands) {
        startCycle = max(startCycle, fcv[nq+breg]);
    }
    if (g->is_conditional()) {
        for (auto breg : g->cond_operands) {
            startCycle = max(startCycle, fcv[nq+breg]);
        }
    }
    QL_ASSERT (startCycle < ir::MAX_CYCLE);

    return startCycle;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// is purely functional, doesn't affect state
UInt FreeCycle::StartCycle(const ir::GateRef &g) const {
    UInt startCycle = StartCycleNoRc(g);

    if (options->heuristic == Heuristic::BASE_RC || options->heuristic == Heuristic::MIN_EXTEND_RC) {
        UInt baseStartCycle = startCycle;

        while (startCycle < ir::MAX_CYCLE) {
            // QL_DOUT("Startcycle for " << g->qasm() << ": available? at startCycle=" << startCycle);
            if (rs->available(startCycle, g)) {
                // QL_DOUT(" ... [" << startCycle << "] resources available for " << g->qasm());
                break;
            } else {
                // QL_DOUT(" ... [" << startCycle << "] Busy resource for " << g->qasm());
                startCycle++;
            }
        }
        if (baseStartCycle != startCycle) {
            // QL_DOUT(" ... from [" << baseStartCycle << "] to [" << startCycle-1 << "] busy resource(s) for " << g->qasm());
        }
    }
    QL_ASSERT (startCycle < ir::MAX_CYCLE);

    return startCycle;
}

// schedule gate g in the FreeCycle map
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// the FreeCycle map is updated, not the resource map for operands updated by the gate
// this is done, because AddNoRc is used to represent just gate dependences, avoiding a build of a dep graph
void FreeCycle::AddNoRc(const ir::GateRef &g, UInt startCycle) {
    UInt duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division
    UInt freeCycle = startCycle + duration;
    for (auto qreg : g->operands) {
        fcv[qreg] = freeCycle;
    }
    for (auto breg : g->breg_operands) {
        fcv[nq+breg] = freeCycle;
    }
}

// schedule gate g in the FreeCycle and resource maps
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// both the FreeCycle map and the resource map are updated
// startcycle must be the result of an earlier StartCycle call (with rc!)
void FreeCycle::Add(const ir::GateRef &g, UInt startCycle) {
    AddNoRc(g, startCycle);

    if (options->heuristic == Heuristic::BASE_RC || options->heuristic == Heuristic::MIN_EXTEND_RC) {
        rs->reserve(startCycle, g);
    }
}

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
