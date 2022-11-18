/** \file
 * FreeCycle implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/opt.h"
#include "ql/ir/ops.h"
#include "ql/ir/compat/compat.h"
#include "ql/rmgr/manager.h"
#include "ql/com/map/qubit_mapping.h"
#include "options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * FreeCycle: map each real qubit to the first cycle that it is free for use.
 *
 * In scheduling gates, qubit dependencies cause latencies. For each real qubit,
 * the first cycle that it is free to use is the cycle that the last gate that
 * was scheduled in the qubit, has just finished (i.e. in the previous cycle);
 * the map serves as a summary to ease scheduling next gates.
 *
 * Likewise, while mapping, swaps are scheduled just before a non-NN two-qubit
 * gate. Moreover, such swaps may involve real qubits on the path between the
 * real operand qubits of the gate, which may be different from the real operand
 * qubits. The evaluation of which path of swaps is best is, among other data,
 * based on which path causes the latency of the whole circuit to be extended
 * the least. This latency extension is measured from the data in the FreeCycle
 * map; so a FreeCycle map is part of each path of swaps that is evaluated for a
 * particular non-NN 2-qubit gate next to a FreeCycle map that is part of the
 * output stream (the main past).
 *
 * Since gate durations are in nanoseconds, and one cycle is some fixed number
 * of nanoseconds, the duration is converted to a rounded-up number of cycles
 * when computing the added latency.
 */
class FreeCycle {
public:
    void initialize(const ir::PlatformRef &p, const OptionsRef &opt);

    /**
     * Returns the depth of the FreeCycle map. Equals the max of all entries
     * minus the min of all entries not used yet; would be used to compute the
     * max size of a top window on the past.
     */
    utils::UInt get_depth() const;

    /**
     * Returns the minimum cycle of the FreeCycle map; equals the min of all
     * entries.
     */
    utils::UInt get_min() const;

    /**
     * Returns the maximum cycle of the FreeCycle map; equals the max of all
     * entries.
     */
    utils::UInt get_max() const;

    /**
     * Prints the state of this object along with the given string.
     */
    void print(const utils::Str &s) const;

    /**
     * Calls print only if the loglevel is debug or more verbose.
     */
    void debug_print(const utils::Str &s) const;

    /**
     * Return whether gate with first operand qubit r0 can be scheduled earlier
     * than with operand qubit r1.
     */
    utils::Bool is_first_operand_earlier(utils::UInt r0, utils::UInt r1) const;

    /**
     * Returns whether swap(fr0,fr1) start earlier than a swap(sr0,sr1). Is
     * really a short-cut ignoring config file and perhaps several other
     * details.
     */
    utils::Bool is_first_swap_earliest(
        utils::UInt fr0,
        utils::UInt fr1,
        utils::UInt sr0,
        utils::UInt sr1) const;

    /**
     * Returns what the start cycle would be when we would schedule the given
     * gate. Purely
     * functional, doesn't affect state.
     */
    utils::UInt get_start_cycle(const ir::CustomInstructionRef &g) const;

    /**
     * Schedules the given gate in the FreeCycle map. The FreeCycle map
     * is updated. startcycle must be the result of an
     * earlier StartCycle call (with rc!)
     */
    void add(const ir::CustomInstructionRef &g, utils::UInt start_cycle);

private:
    static std::array<utils::UInt, 5> get_indices(const ir::Reference &ref) {
        std::array<utils::UInt, 5> result;

        for (utils::UInt i = 0; i < ref.indices.size(); ++i) {
            if (i >= result.size()) {
                QL_FATAL("Cannot handle more than 5 dimensions");
            }

            auto* int_lit = ref.indices[i]->as_int_literal();

            if (!int_lit) {
                QL_FATAL("Indices must be int lit");
            }

            result[i] = int_lit->value;
        }

        return result;
    }

    utils::UInt& get_for_qubit(utils::UInt i) {
        return get_for_reference(*make_qubit_ref(platform, i));
    }

    utils::UInt get_for_qubit(utils::UInt i) const {
        return get_for_reference(*make_qubit_ref(platform, i));
    }

    utils::UInt& get_for_reference(const ir::Reference &ref) {
        auto it = std::find_if(fcv.begin(), fcv.end(), [&ref](std::pair<ir::Reference, utils::UInt> p) {
            return p.first.equals(ref);
        });

        if (it == fcv.end()) {
            fcv.push_back(std::make_pair(ref, 1));
            // It's important that fcv is a list so that references are not invalidated.
            return fcv.back().second;
        }

        return it->second;
    }

    utils::UInt get_for_reference(const ir::Reference &ref) const {
        auto it = std::find_if(fcv.begin(), fcv.end(), [&ref](std::pair<ir::Reference, utils::UInt> p) {
            return p.first.equals(ref);
        });

        if (it == fcv.end()) {
            return 0;
        }

        return it->second;
    }

    ir::PlatformRef platform;

    OptionsRef options;

    std::list<std::pair<ir::Reference, utils::UInt>> fcv;

    /**
     * Actual resources occupied by scheduled gates, if resource-aware.
     */
    utils::Opt<rmgr::State> rs;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
