#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/opt.h"
#include "ql/ir/ops.h"
#include "options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * FreeCycle: map each reference to real qubit to the first cycle that it is free for use.
 *
 * This allows to compute, for each routing alternative, what the cost in terms of circuit depth
 * will be - the goal of the minextend option in this router being to minimize the overall circuit depth.
 * 
 * This is also used in the options base and baserc to check whether the operands of a swap should be swapped
 * or whether move should be used instead of swap when allowed.
 */
class FreeCycle {
public:
    void initialize(const ir::PlatformRef &p, const OptionsRef &opt);

    /**
     * Returns the maximum cycle of the FreeCycle map; that is, the cycle where all scheduled
     * operations are completed.
     */
    utils::UInt get_max() const;

    /**
     * Return whether qubit r0 is available strictly before qubit r1.
     */
    utils::Bool is_qubit_free_before(utils::UInt r0, utils::UInt r1) const;

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
     * Returns at what cycle the given gate can start when scheduled.
     */
    utils::UInt get_start_cycle(const ir::CustomInstructionRef &g) const;

    /**
     * Schedules the given gate in the FreeCycle map. The cycle map is updated.
     * startcycle must be obtained by get_start_cycle.
     */
    void add(const ir::CustomInstructionRef &g, utils::UInt start_cycle);

    /**
     * Returns the max cycle extension if g is scheduled.
     */
    utils::UInt cycle_extension(const ir::CustomInstructionRef &g) const;

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

    /**
     * The map from qubit references to the first cycle index where the given qubit is available.
     * This is encoded as an association list, which avoids the burden of defining a hash or an ordering.
     */
    std::list<std::pair<ir::Reference, utils::UInt>> fcv;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
