/** \file
 * Alter implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/ir/compat/compat.h"
#include "ql/rmgr/manager.h"
#include "options.h"
#include "free_cycle.h"
#include "past.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

struct Path {
    utils::UInt qubit;
    utils::RawPtr<Path> prev;
};

class Alter {
public:
    /**
     * Adds a node to the path in front, extending its length with one.
     */
    void add_to_front(utils::UInt q);

    /**
     * Add swap gates for the current path to the given past, up to the maximum
     * specified by the swap selection mode. This past can be a path-local one
     * or the main past. After having added them, schedule the result into that
     * past.
     */
    void add_swaps(Past &past) const;

    /**
     * Compute cycle extension of the current alternative in curr_past relative
     * to the given base past.
     *
     * extend can be called in a deep exploration where pasts have been
     * extended, each one on top of a previous one, starting from the base past.
     * The curr_past here is the last extended one, i.e. on top of which this
     * extension should be done; the base_past is the ultimate base past
     * relative to which the total extension is to be computed.
     *
     * Do this by adding the swaps described by this alternative. Keep this resulting past in
     * the current alternative (for later use). Compute the total extension of
     * all pasts relative to the base past, and store this extension in the
     * alternative's score for later use.
     */
    void extend(Past curr_past, const Past &base_past);

    static utils::List<Alter> create_from_path(const ir::PlatformRef &platform, const ir::BlockBaseRef &block, const OptionsRef &options, ir::CustomInstructionRef gate, Path path);

    ir::CustomInstructionRef get_target_gate() {
        return target_gate;
    }

    utils::Real get_score() const {
        QL_ASSERT(score_valid);
        return score;
    }

    void set_score(utils::Real s) {
        score = s;
        score_valid = true;
    }

private:
    Alter(ir::PlatformRef aPlatform, const ir::BlockBaseRef &block, const OptionsRef &opt, ir::CustomInstructionRef g);

    /**
     * Split the path. Starting from the representation in the total attribute,
     * generate all split path variations where each path is split once at any
     * hop in it. The intention is that the mapped two-qubit gate can be placed
     * at the position of that hop. All result paths are added/appended to the
     * given result list.
     *
     * When at the hop of a split a two-qubit gate cannot be placed, the split
     * is not done there. This means at the end that, when all hops are
     * inter-core, no split is added to the result.
     *
     * distance=5   means length=6  means 4 swaps + 1 CZ gate, e.g.
     * index in total:      0           1           2           length-3        length-2        length-1
     * qubit:               2   ->      5   ->      7   ->      3       ->      1       CZ      4
     */
    utils::List<Alter> split() const;

    ir::PlatformRef platform;
    ir::BlockBaseRef block;
    OptionsRef options;

    /**
     * The gate that this variation aims to make nearest-neighbor.
     */
    ir::CustomInstructionRef target_gate;

    /**
     * The full path, including source and target nodes.
     */
    utils::Vec<utils::UInt> total;

    /**
     * The partial path after split, starting at source.
     */
    utils::Vec<utils::UInt> from_source;

    /**
     * The partial path after split, starting at target, backward.
     */
    utils::Vec<utils::UInt> from_target;

    /**
     * The latency extension caused by the path.
     */
    utils::Real score = 0.0;

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
