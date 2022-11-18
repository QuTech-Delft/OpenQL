#pragma once

#include "options.h"
#include "past.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

class Alter {
public:
    /**
     * Add swap gates for the current path to the given past, up to the maximum
     * specified by the swap selection mode. This past can be a path-local one
     * or the main past. After having added them, schedule the result into that
     * past.
     */
    void add_swaps(Past &past, utils::Any<ir::Statement> *output_circuit = nullptr) const;

    /**
     * Compute cycle extension of the current alternative in curr_past relative
     * to base_past.
     *
     * extend can be called in a deep exploration where pasts have been
     * extended, each one on top of a previous one, starting from the base past.
     * The curr_past here is the last extended one, i.e. on top of which this
     * extension should be done; the base_past is the ultimate base past
     * relative to which the total extension is to be computed.
     *
     * Do this by adding the swaps described by this alternative, and fill score.
     */
    void extend(Past curr_past, const Past &base_past);

    /**
     * Split the given routing path into alters where the target gate is executed at every possible hop along the path.
     *
     * When at one hop along the path a two-qubit gate cannot be placed, the split
     * is not done there. This means at the end that, when all hops are
     * inter-core, the resulting list of alters is empty.
     */
   static utils::List<Alter> create_from_path(const ir::PlatformRef &platform, const ir::BlockBaseRef &block, const OptionsRef &options, ir::CustomInstructionRef gate, std::list<utils::UInt> path);

    ir::CustomInstructionRef get_target_gate() {
        return target_gate;
    }

    utils::UInt get_score() const {
        QL_ASSERT(score_valid);
        return score;
    }

    void set_score(utils::UInt s) {
        score = s;
        score_valid = true;
    }

private:
    Alter(ir::PlatformRef pl, const ir::BlockBaseRef &b, const OptionsRef &opt, ir::CustomInstructionRef g, std::shared_ptr<std::list<utils::UInt>> pa,
        std::list<utils::UInt>::iterator l, std::list<utils::UInt>::reverse_iterator r);

    ir::PlatformRef platform;
    ir::BlockBaseRef block;
    OptionsRef options;

    /**
     * The gate that this variation aims to make nearest-neighbor.
     */
    ir::CustomInstructionRef target_gate;

    std::shared_ptr<std::list<utils::UInt>> path;

    std::list<utils::UInt>::iterator leftOpIt;

    std::list<utils::UInt>::reverse_iterator rightOpIt;

    /**
     * The latency extension caused by the path.
     */
    utils::UInt score = 0;

    /**
     * Initially false, true after assignment to score.
     */
    utils::Bool score_valid = false;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
