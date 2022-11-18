/** \file
 * Alter implementation.
 */

#include "alter.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

Alter::Alter(ir::PlatformRef p, const ir::BlockBaseRef &b, const OptionsRef &opt, ir::CustomInstructionRef g) :
        platform(p),
        block(b),
        options(opt),
        target_gate(g) {}

void Alter::add_to_front(utils::UInt q) {
    total.insert(total.begin(), q); // hopelessly inefficient
}

void Alter::add_swaps(Past &past) const {
    const auto& mode = options->swap_selection_mode;
    if (mode == SwapSelectionMode::ONE || mode == SwapSelectionMode::ALL) {
        utils::UInt max_num_to_add = (mode == SwapSelectionMode::ONE ? 1 : utils::MAX);

        for (utils::UInt i = 1; i < from_source.size() && i <= max_num_to_add; i++) {
            auto from = from_source[i - 1];
            auto to = from_source[i];
            past.add_swap(from, to);
        }

        for (utils::UInt i = 1; i < from_target.size() && i <= max_num_to_add; i++) {
            auto from = from_target[i - 1];
            auto to = from_target[i];
            past.add_swap(from, to);
        }
    } else {
        QL_ASSERT(mode == SwapSelectionMode::EARLIEST);

        if (from_source.size() >= 2 && from_target.size() >= 2) {
            if (past.is_first_swap_earliest(from_source[0], from_source[1],
                                            from_target[0], from_target[1])) {
                past.add_swap(from_source[0], from_source[1]);
            } else {
                past.add_swap(from_target[0], from_target[1]);
            }
        } else if (from_source.size() >= 2) {
            past.add_swap(from_source[0], from_source[1]);
        } else if (from_target.size() >= 2) {
            past.add_swap(from_target[0], from_target[1]);
        }
    }

    past.schedule();
}

void Alter::extend(Past curr_past, const Past &base_past) {
    add_swaps(curr_past);
    set_score(curr_past.get_max_free_cycle() - base_past.get_max_free_cycle());
}

utils::List<Alter> Alter::split() const {
     // This is ugly.

    utils::List<Alter> result;

    utils::UInt length = total.size();
    QL_ASSERT (length >= 2);
    for (utils::UInt rightopi = length - 1; rightopi >= 1; rightopi--) {
        utils::UInt leftopi = rightopi - 1;
        QL_ASSERT (leftopi >= 0);
        if (platform->topology->is_inter_core_hop(total[leftopi], total[rightopi])) {
            // an inter-core hop cannot execute a two-qubit gate, so is not a valid alternative
            continue;
        }

        Alter na = *this;

        utils::UInt fromi, toi;

        na.from_source.resize(leftopi + 1);
        for (fromi = 0, toi = 0; fromi <= leftopi; fromi++, toi++) {
            na.from_source[toi] = na.total[fromi];
        }

        na.from_target.resize(length - leftopi - 1);
        for (fromi = length-1, toi = 0; fromi > leftopi; fromi--, toi++) {
            na.from_target[toi] = na.total[fromi];
        }

        result.push_back(na);
    }

    return result;
}

utils::List<Alter> Alter::create_from_path(const ir::PlatformRef &platform, const ir::BlockBaseRef &block, const OptionsRef &options, ir::CustomInstructionRef gate, Path path) {
    Alter a(platform, block, options, gate); // FIXME: after construction, Alter instance is not well-formed...
    utils::RawPtr<Path> it = &path;
    while (it) {
        a.add_to_front(it->qubit);
        it = it->prev;
    }
    return a.split();
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
