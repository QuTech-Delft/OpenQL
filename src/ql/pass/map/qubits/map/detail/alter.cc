#include "alter.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

Alter::Alter(ir::PlatformRef pl, const ir::BlockBaseRef &b, const OptionsRef &opt, ir::CustomInstructionRef g,
                std::shared_ptr<std::list<utils::UInt>> pa, std::list<utils::UInt>::iterator l,
                std::list<utils::UInt>::reverse_iterator r) :
        platform(pl),
        block(b),
        options(opt),
        target_gate(g),
        path(std::move(pa)),
        leftOpIt(l), rightOpIt(r) {}

void Alter::add_swaps(Past &past, utils::Any<ir::Statement> *output_circuit) const {
    const auto& mode = options->swap_selection_mode;
    if (mode == SwapSelectionMode::ONE || mode == SwapSelectionMode::ALL) {
        utils::UInt max_num_to_add = (mode == SwapSelectionMode::ONE ? 1 : utils::MAX);

        // Add a maximum of max_num_to_add swaps to "past" from the 2 paths represented by this (split) Alter.
        // So a maximum of 2 * max_num_to_add swaps are added to "past".
        // It stops when the paths are completely covered, or when the max number of swaps is reached.
        utils::UInt swaps_added = 0;
        for (auto it = path->begin();
                swaps_added < max_num_to_add && it != leftOpIt;
                ++swaps_added, it = std::next(it)) {
            past.add_swap(*it, *std::next(it), output_circuit);
        }

        swaps_added = 0;
        for (auto it = path->rbegin();
                swaps_added < max_num_to_add && it != rightOpIt;
                ++swaps_added, it = std::next(it)) {
            past.add_swap(*it, *std::next(it), output_circuit);
        }
    } else {
        QL_ASSERT(mode == SwapSelectionMode::EARLIEST);

        if (leftOpIt != path->begin() && rightOpIt != path->rbegin()) {
            // Both left and right operands of the 2q gate need to get closer.
            if (past.is_first_swap_earliest(*path->begin(), *std::next(path->begin()),
                                            *path->rbegin(), *std::next(path->rbegin()))) {
                past.add_swap(*path->begin(), *std::next(path->begin()), output_circuit);
            } else {
                past.add_swap(*path->rbegin(), *std::next(path->rbegin()), output_circuit);
            }
        } else if (leftOpIt != path->begin()) {
            // Right operand of the 2q gate does not move, only left does.
            past.add_swap(*path->begin(), *std::next(path->begin()), output_circuit);
        } else if (rightOpIt != path->rbegin()) {
            // Left operand of the 2q gate does not move, only right does.
            past.add_swap(*path->rbegin(), *std::next(path->rbegin()), output_circuit);
        }
    }
}

void Alter::extend(Past curr_past, const Past &base_past) {
    QL_ASSERT(!score_valid && "Alter::extend() can only be called once!");
    add_swaps(curr_past);
    set_score(curr_past.get_max_free_cycle() - base_past.get_max_free_cycle());
}

utils::List<Alter> Alter::create_from_path(const ir::PlatformRef &platform, const ir::BlockBaseRef &block, const OptionsRef &options, ir::CustomInstructionRef gate, std::list<utils::UInt> path) {
    utils::List<Alter> result;

    std::shared_ptr<std::list<utils::UInt>> shared_path(new std::list<utils::UInt>(path));

    QL_ASSERT (shared_path->size() >= 2);

    auto leftOpIt = shared_path->begin();
    auto rightOpIt = std::prev(std::prev(shared_path->rend()));
    while (leftOpIt != std::prev(shared_path->end())) {
        QL_ASSERT(*std::next(leftOpIt) == *rightOpIt);
        QL_ASSERT(platform->topology->get_distance(*leftOpIt, *rightOpIt) == 1);

        // An inter-core hop cannot execute a two-qubit gate, so is not a valid alternative.
        if (!platform->topology->is_inter_core_hop(*leftOpIt, *rightOpIt)) {
            result.push_back(Alter(platform, block, options, gate, shared_path, leftOpIt, rightOpIt));
        }

        if (rightOpIt != shared_path->rbegin()) {
            rightOpIt = std::prev(rightOpIt);
        }
        leftOpIt = std::next(leftOpIt);
    }

    return result;
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
