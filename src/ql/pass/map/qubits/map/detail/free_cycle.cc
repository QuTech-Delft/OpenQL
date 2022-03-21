/** \file
 * FreeCycle implementation.
 */

#include "free_cycle.h"

// #define MULTI_LINE_LOG_DEBUG to enable multi-line dumping 
#undef MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Initializes this FreeCycle object.
 */
void FreeCycle::initialize(const ir::compat::PlatformRef &p, const OptionsRef &opt) {
    QL_DOUT("FreeCycle::initialize()");
    auto rm = rmgr::Manager::from_defaults(p);   // allocated here and copied below to rm because of platform parameter
                                                           // JvS: I have no idea what ^ means
    QL_DOUT("... created FreeCycle initialize local resource_manager");
    options = opt;
    platform = p;
    nq = platform->qubit_count;
    nb = platform->breg_count;
    ct = platform->cycle_time;
    QL_DOUT("... FreeCycle: nq=" << nq << ", nb=" << nb << ", ct=" << ct << "), initializing to all 0 cycles");
    fcv.clear();
    fcv.resize(nq+nb, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
    QL_DOUT("... about to copy FreeCycle initialize local resource_manager to FreeCycle member rm");
    rs = rm.build(rmgr::Direction::FORWARD);
    QL_DOUT("... done copy FreeCycle initialize local resource_manager to FreeCycle member rm");
}

/**
 * Returns the depth of the FreeCycle map. Equals the max of all entries
 * minus the min of all entries not used yet; would be used to compute the
 * max size of a top window on the past.
 */
utils::UInt FreeCycle::get_depth() const {
    return get_max() - get_min();
}

/**
 * Returns the minimum cycle of the FreeCycle map; equals the min of all
 * entries.
 */
utils::UInt FreeCycle::get_min() const {
    utils::UInt min_free_cycle = ir::compat::MAX_CYCLE;
    for (const auto &v : fcv) {
        if (v < min_free_cycle) {
            min_free_cycle = v;
        }
    }
    return min_free_cycle;
}

/**
 * Returns the maximum cycle of the FreeCycle map; equals the max of all
 * entries.
 */
utils::UInt FreeCycle::get_max() const {
    utils::UInt max_free_cycle = 0;
    for (const auto &v : fcv) {
        if (max_free_cycle < v) {
            max_free_cycle = v;
        }
    }
    return max_free_cycle;
}

/**
 * Prints the state of this object along with the given string.
 */
void FreeCycle::print(const utils::Str &s) const {
    utils::UInt  min_free_cycle = get_min();
    utils::UInt  max_free_cycle = get_max();
    std::cout << "... FreeCycle" << s << ":";
    for (utils::UInt i = 0; i < nq; i++) {
        utils::UInt v = fcv[i];
        std::cout << " [" << i << "]=";
        if (v == min_free_cycle) {
            std::cout << "_";
        }
        if (v == max_free_cycle) {
            std::cout << "^";
        }
        std::cout << v;
    }
    std::cout << std::endl;
    // rm.Print("... in FreeCycle: ");
}

/**
 * Calls print only if the loglevel is debug or more verbose.
 */
void FreeCycle::debug_print(const utils::Str &s) const {
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("Print FreeCycle state: ");
        print(s);
    }
#else
    QL_DOUT("Print FreeCycle state (disabled)");
#endif
}


/**
 * Return whether gate with first operand qubit r0 can be scheduled earlier
 * than with operand qubit r1.
 */
utils::Bool FreeCycle::is_first_operand_earlier(utils::UInt r0, utils::UInt r1) const {
    QL_DOUT("... fcv[" << r0 << "]=" << fcv[r0] << " fcv[" << r1 << "]=" << fcv[r1] << " is_first_operand_earlier=" << (fcv[r0] < fcv[r1]));
    return fcv[r0] < fcv[r1];
}

/**
 * Returns whether swap(fr0,fr1) start earlier than a swap(sr0,sr1). Is
 * really a short-cut ignoring config file and perhaps several other
 * details.
 */
utils::Bool FreeCycle::is_first_swap_earliest(
    utils::UInt fr0,
    utils::UInt fr1,
    utils::UInt sr0,
    utils::UInt sr1
) const {
    if (options->reverse_swap_if_better) {
        if (fcv[fr0] < fcv[fr1]) {
            utils::UInt  tmp = fr1; fr1 = fr0; fr0 = tmp;
        }
        if (fcv[sr0] < fcv[sr1]) {
            utils::UInt  tmp = sr1; sr1 = sr0; sr0 = tmp;
        }
    }
    utils::UInt start_cycle_first_swap = utils::max(fcv[fr0] - 1, fcv[fr1]);
    utils::UInt start_cycle_second_swap = utils::max(fcv[sr0] - 1, fcv[sr1]);

    QL_DOUT("... fcv[" << fr0 << "]=" << fcv[fr0] << " fcv[" << fr1 << "]=" << fcv[fr1] << " start=" << start_cycle_first_swap << " fcv[" << sr0 << "]=" << fcv[sr0] << " fcv[" << sr1 << "]=" << fcv[sr1] << " start=" << start_cycle_second_swap << " is_first_swap_earliest=" << (start_cycle_first_swap < start_cycle_second_swap));
    return start_cycle_first_swap < start_cycle_second_swap;
}

/**
 * Returns what the start cycle would be when we would schedule the given
 * gate, ignoring resource constraints. gate operands are real qubit indices
 * and breg indices. Purely functional, doesn't affect state.
 */
utils::UInt FreeCycle::get_start_cycle_no_rc(const ir::compat::GateRef &g) const {
    utils::UInt start_cycle = 1;
    for (auto qreg : g->operands) {
        start_cycle = utils::max(start_cycle, fcv[qreg]);
    }
    for (auto breg : g->breg_operands) {
        start_cycle = utils::max(start_cycle, fcv[nq + breg]);
    }
    if (g->is_conditional()) {
        for (auto breg : g->cond_operands) {
            start_cycle = utils::max(start_cycle, fcv[nq + breg]);
        }
    }
    QL_ASSERT (start_cycle < ir::compat::MAX_CYCLE);

    return start_cycle;
}

/**
 * Returns what the start cycle would be when we would schedule the given
 * gate. gate operands are real qubit indices and breg indices. Purely
 * functional, doesn't affect state.
 */
utils::UInt FreeCycle::get_start_cycle(const ir::compat::GateRef &g) const {
    utils::UInt start_cycle = get_start_cycle_no_rc(g);

    if (options->heuristic == Heuristic::BASE_RC || options->heuristic == Heuristic::MIN_EXTEND_RC) {
        utils::UInt base_start_cycle = start_cycle;

        while (start_cycle < ir::compat::MAX_CYCLE) {
            // QL_DOUT("Startcycle for " << g->qasm() << ": available? at startCycle=" << startCycle);
            if (rs->available(start_cycle, g)) {
                // QL_DOUT(" ... [" << startCycle << "] resources available for " << g->qasm());
                break;
            } else {
                // QL_DOUT(" ... [" << startCycle << "] Busy resource for " << g->qasm());
                start_cycle++;
            }
        }
        if (base_start_cycle != start_cycle) {
            // QL_DOUT(" ... from [" << baseStartCycle << "] to [" << startCycle-1 << "] busy resource(s) for " << g->qasm());
        }
    }
    QL_ASSERT (start_cycle < ir::compat::MAX_CYCLE);

    return start_cycle;
}

/**
 * Schedules the given gate in the FreeCycle map. The gate operands are real
 * qubit indices and breg indices. The FreeCycle map is updated, but not the
 * resource map. This is done because add_no_rc is used to represent just gate
 * dependencies, avoiding a build of a dep graph.
 */
void FreeCycle::add_no_rc(const ir::compat::GateRef &g, utils::UInt startCycle) {
    utils::UInt duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division
    utils::UInt freeCycle = startCycle + duration;
    for (auto qreg : g->operands) {
        fcv[qreg] = freeCycle;
    }
    for (auto breg : g->breg_operands) {
        fcv[nq+breg] = freeCycle;
    }
}

/**
 * Schedules the given gate in the FreeCycle and resource maps. The gate
 * operands are real qubit indices and breg indices. Both the FreeCycle map
 * and the resource map are updated. startcycle must be the result of an
 * earlier StartCycle call (with rc!)
 */
void FreeCycle::add(const ir::compat::GateRef &g, utils::UInt start_cycle) {
    add_no_rc(g, start_cycle);

    if (options->heuristic == Heuristic::BASE_RC || options->heuristic == Heuristic::MIN_EXTEND_RC) {
        rs->reserve(start_cycle, g);
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
