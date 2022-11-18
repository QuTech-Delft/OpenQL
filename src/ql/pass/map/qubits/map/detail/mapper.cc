#include "mapper.h"

#include <chrono>
#include "ql/utils/filesystem.h"
#include "ql/pass/ana/statistics/annotations.h"
#include "ql/com/ddg/dot.h"
#include "ql/ir/ops.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

namespace {
void assign_increasing_cycle_numbers_to_routed_circuit(ir::PlatformRef const& platform, utils::Any<ir::Statement>& circuit) {
    utils::UInt cycle = 0;
    for (auto &gate : circuit) {
        ir::OperandsHelper ops(platform, *gate.as<ir::CustomInstruction>());
        if (ops.numberOfQubitOperands() == 2) {
            auto qs = ops.get2QGateOperands();
            QL_ASSERT(platform->topology->get_distance(qs.first, qs.second) == 1 && "Circuit is not routed");
        }

        gate->cycle = cycle++;
    }
}
}

std::ostream &operator<<(std::ostream &os, PathStrategy p) {
    switch (p) {
        case PathStrategy::ALL:        os << "all";        break;
        case PathStrategy::LEFT:       os << "left";       break;
        case PathStrategy::RIGHT:      os << "right";      break;
        case PathStrategy::LEFT_RIGHT: os << "left-right"; break;
        case PathStrategy::RANDOM:     os << "random";     break;
    }
    return os;
}

using namespace utils;
using namespace com;

List<Alter> Mapper::gen_shortest_paths(
    const ir::CustomInstructionRef &gate,
    std::list<utils::UInt> path,
    UInt src,
    UInt tgt,
    UInt budget,
    UInt max_alters,
    PathStrategy strategy
) {
    QL_ASSERT(path.empty() || path.back() != src);
    path.push_back(src);
    
    if (src == tgt) {
        return Alter::create_from_path(platform, block, options, gate, path);
    }

    // Start looking around at neighbors for serious paths.
    UInt d = platform->topology->get_distance(src, tgt);
    QL_DOUT("gen_shortest_paths: distance(src=" << src << ", tgt=" << tgt << ") = " << d);
    QL_ASSERT(d >= 1);

    // Reduce neighbors nbs to those n continuing a path within budget.
    // src=>tgt is distance d, budget>=d is allowed, attempt src->n=>tgt
    // src->n is one hop, budget from n is one less so distance(n,tgt) <= budget-1 (i.e. distance < budget)
    // when budget==d, this defaults to distance(n,tgt) <= d-1
    auto neighbors = platform->topology->get_neighbors(src);

    neighbors.remove_if([this,budget,tgt](const UInt& n) { return platform->topology->get_distance(n, tgt) >= budget; });

    // Update the neighbor list according to the path strategy.
    if (strategy == PathStrategy::RANDOM) {
        // Shuffle the neighbor list. We have to go through a vector to do that,
        // otherwise std::shuffle doesn't work.
        utils::Vec<utils::UInt> neighbors_vec{neighbors.begin(), neighbors.end()};
        std::shuffle(neighbors_vec.begin(), neighbors_vec.end(), rng);
        neighbors.clear();
        neighbors.insert(neighbors.begin(), neighbors_vec.begin(), neighbors_vec.end());
    } else {
        // Rotate neighbor list nbl such that largest difference between angles
        // of adjacent elements is beyond back(). This only makes sense when
        // there is an underlying xy grid; when not, only the ALL strategy is
        // supported.
        QL_ASSERT(platform->topology->has_coordinates() || strategy == PathStrategy::ALL);
        platform->topology->sort_neighbors_by_angle(src, neighbors);

        // Select the subset of those neighbors that continue in direction(s) we
        // want.
        if (strategy == PathStrategy::LEFT) {
            neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.front(); } );
        } else if (strategy == PathStrategy::RIGHT) {
            neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.back(); } );
        } else if (strategy == PathStrategy::LEFT_RIGHT) {
            neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.front() && n != neighbors.back(); } );
        }

    }

    // For all resulting neighbors, find all continuations of a shortest path by
    // recursively calling ourselves.
    List<Alter> result;
    for (auto &n : neighbors) {
        PathStrategy new_strategy = strategy;

        // For each neighbor, only look in desired direction, if any.
        if (strategy == PathStrategy::LEFT_RIGHT && neighbors.size() != 1) {
            // When looking both left and right still, and there is a choice
            // now, split into left and right.
            if (n == neighbors.front()) {
                new_strategy = PathStrategy::LEFT;
            } else {
                new_strategy = PathStrategy::RIGHT;
            }
        }

        // Select maximum number of sub-alternatives to build. If our incoming
        // max_alters is 0 there is no limit.
        UInt max_sub_alters = 0;
        if (max_alters > 0) {
            QL_ASSERT(max_alters > result.size());
            max_sub_alters = max_alters - result.size();
        }

        // Get list of possible paths in budget-1 from n to tgt.
        auto rec = gen_shortest_paths(gate, path, n, tgt, budget - 1, max_sub_alters, new_strategy);
        result.splice(result.end(), rec);

        // Check whether we've found enough alternatives already.
        if (max_alters && result.size() >= max_alters) {
            break;
        }
    }

    return result;
}

List<Alter> Mapper::gen_shortest_paths(const ir::CustomInstructionRef &gate, UInt src, UInt tgt) {
    QL_ASSERT(src != tgt);

    UInt budget = platform->topology->get_min_hops(src, tgt);
    auto compute = [&gate, src, tgt, budget, this](PathStrategy s) {
        return gen_shortest_paths(gate, {}, src, tgt, budget, options->max_alters, s);
    };

    if (options->path_selection_mode == PathSelectionMode::ALL) {
        return compute(PathStrategy::ALL);
    }
    
    if (options->path_selection_mode == PathSelectionMode::BORDERS) {
        return compute(PathStrategy::LEFT_RIGHT);
    }
    
    if (options->path_selection_mode == PathSelectionMode::RANDOM) {
        return compute(PathStrategy::RANDOM);
    }

    QL_FATAL("Unknown value of path selection mode option " << options->path_selection_mode);
}

List<Alter> Mapper::gen_alters_gate(const ir::CustomInstructionRef &gate, Past &past) {
    auto qubits = ir::OperandsHelper(platform, *gate).get2QGateOperands();

    UInt src = past.get_real_qubit(qubits.first);
    UInt tgt = past.get_real_qubit(qubits.second);

    return gen_shortest_paths(gate, src, tgt);
}

List<Alter> Mapper::gen_alters(const utils::List<ir::CustomInstructionRef> &gates, Past &past) {
    if (options->lookahead_mode != LookaheadMode::ALL) {
        return gen_alters_gate(gates.front(), past);
    }

    List<Alter> result;
    for (const auto &gate : gates) {
        auto gate_alters = gen_alters_gate(gate, past);
        result.splice(result.end(), gate_alters);
    }

    return result;
}

Alter Mapper::tie_break_alter(List<Alter> &alters, Future &future) {
    QL_ASSERT(!alters.empty());

    if (alters.size() == 1) {
        return alters.front();
    }

    switch (options->tie_break_method) {
        case TieBreakMethod::CRITICAL: {
            std::vector<ir::CustomInstructionRef> target_gates;
            target_gates.reserve(alters.size());
            for (auto &a : alters) {
                target_gates.push_back(a.get_target_gate());
            }
            ir::CustomInstructionRef gate = future.get_most_critical(target_gates);

            for (auto &a : alters) {
                if (a.get_target_gate().get_ptr() == gate.get_ptr()) {
                    return a;
                }
            }
            
            QL_FATAL("This should not happen");
        }

        case TieBreakMethod::RANDOM: {
            std::uniform_int_distribution<> dis(0, (alters.size() - 1));
            UInt choice = dis(rng);
            return *(std::next(alters.begin(), choice));
        }

        case TieBreakMethod::LAST:
            return alters.back();

        case TieBreakMethod::FIRST:
        default:
            return alters.front();
    }
}

void Mapper::map_routed_gate(const ir::CustomInstructionRef &gate, Past &past, utils::Any<ir::Statement> *output_circuit) {
    auto cloned_gate = gate->clone();
    past.make_real(cloned_gate);
    past.add(cloned_gate, output_circuit);
}

void Mapper::commit_alter(Alter &alter, Future &future, Past &past, utils::Any<ir::Statement> *output_circuit) {
    const auto& target = alter.get_target_gate();

    alter.add_swaps(past, output_circuit);

    // When only some swaps were added (based on configuration), the target
    // might not yet be nearest-neighbor, so recheck.
    auto ops = ir::OperandsHelper(platform, *target);
    QL_ASSERT(ops.numberOfQubitOperands() == 2);
    if (ops.isNN2QGate([&past](utils::UInt q) { return past.get_real_qubit(q); })) {
        map_routed_gate(target, past, output_circuit);
        future.completed_gate(target);
    }
}

List<ir::CustomInstructionRef> Mapper::map_mappable_gates(
    Future &future,
    Past &past,
    Bool also_nn_two_qubit_gates,
    utils::Any<ir::Statement> *output_circuit
) {
    utils::List<ir::CustomInstructionRef> available_gates;
    while (!(available_gates = future.get_schedulable_gates()).empty()) {
        routing_progress.feed(future.get_progress());

        bool hasMappedGate = false;
        for (const auto &gate : available_gates) {
            ir::OperandsHelper ops(platform, *gate);

            if (ops.numberOfQubitOperands() >= 3) {
                QL_FATAL("Mapper/router does not handle gates with more than 2 qubits operands.");
            }

            // FIXME: wait gates as well.
            hasMappedGate = ops.numberOfQubitOperands() < 2 || (
                also_nn_two_qubit_gates && ops.isNN2QGate([&past](utils::UInt q) { return past.get_real_qubit(q); }));

            if (hasMappedGate) {
                map_routed_gate(gate, past, output_circuit);
                future.completed_gate(gate);
                break;
            }
        }

        if (!hasMappedGate) {
            // Remaining gates require actual routing.
            return available_gates;
        }
    }
    
    return {};
}


Alter Mapper::select_alter(
    List<Alter> &alters,
    Future &future,
    Past &past,
    Past &base_past,
    UInt recursion_depth
) {
    QL_ASSERT(!alters.empty());

    if (options->heuristic == Heuristic::BASE) {
        return tie_break_alter(alters, future);
    }

    QL_ASSERT(options->heuristic == Heuristic::MIN_EXTEND);

    for (auto &a : alters) {
        a.extend(past, base_past); // This fills a.score.
    }
    alters.sort([this](const Alter &a1, const Alter &a2) { return a1.get_score() < a2.get_score(); });

    auto factor = options->recursion_width_factor * utils::pow(options->recursion_width_exponent, recursion_depth);
    auto keep_real = utils::max(1.0, utils::ceil(factor * alters.size()));
    auto keep = (keep_real < static_cast<utils::Real>(utils::MAX)) ? static_cast<utils::UInt>(keep_real) : utils::MAX;

    QL_ASSERT(keep > 0);

    while (alters.size() > keep) {
        alters.pop_back();
    }

    QL_ASSERT(alters.size() <= keep);

    if (recursion_depth >= options->recursion_depth_limit) {
        while (alters.back().get_score() > alters.front().get_score()) {
            alters.pop_back();
        }

        return tie_break_alter(alters, future);
    }

    for (auto &a : alters) {
        // Copy of current state for recursion.
        Future sub_future = future;
        Past sub_past = past;
        
        commit_alter(a, sub_future, sub_past);

        Bool also_nn_two_qubit_gates = options->recurse_on_nn_two_qubit
                     && (
                         options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST
                         || options->lookahead_mode == LookaheadMode::ALL
                     );

        auto gates = map_mappable_gates(sub_future, sub_past, also_nn_two_qubit_gates);

        if (!gates.empty()) {
            auto sub_alters = gen_alters(gates, sub_past);
            QL_ASSERT(!sub_alters.empty() && "No suitable routing path");
            auto sub_result = select_alter(sub_alters, sub_future, sub_past, base_past, recursion_depth + 1);

            a.set_score(sub_result.get_score());
        } else {
            a.set_score(sub_past.get_max_free_cycle() - base_past.get_max_free_cycle());
        }
    }

    alters.sort([this](const Alter &a1, const Alter &a2) { return a1.get_score() < a2.get_score(); });

    while (alters.back().get_score() > alters.front().get_score()) {
        alters.pop_back();
    }

    return tie_break_alter(alters, future);
}

utils::Any<ir::Statement> Mapper::route_gates(Future &future, Past &past) {
    Bool also_nn_two_qubit_gates = (
        options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST
        || options->lookahead_mode == LookaheadMode::ALL
    );

    routing_progress = Progress("router", 1000);

    List<ir::CustomInstructionRef> gates;
    utils::Any<ir::Statement> output_circuit;
    while (!(gates = map_mappable_gates(future, past, also_nn_two_qubit_gates, &output_circuit)).empty()) {
        auto alters = gen_alters(gates, past);
        QL_ASSERT(!alters.empty() && "No suitable routing path");

        auto selected_alter = select_alter(alters, future, past, past, 0);

        commit_alter(selected_alter, future, past, &output_circuit);

        routing_progress.feed(future.get_progress());
    }

    routing_progress.complete();

    assign_increasing_cycle_numbers_to_routed_circuit(platform, output_circuit);

    return output_circuit;
}

Mapper::RoutingStatistics Mapper::route(ir::BlockBaseRef block, com::map::QubitMapping &v2r) {
    Future future(platform, options, block);

    Past past(platform, options);
    past.import_mapping(v2r);

    block->statements = route_gates(future, past);

    past.export_mapping(v2r_out);

    Mapper::RoutingStatistics stats;
    
    stats.num_swaps_added = past.get_num_swaps_added();
    stats.num_moves_added = past.get_num_moves_added();

    return stats;
}

Mapper::RoutingStatistics Mapper::map_block(ir::BlockBaseRef block) {
    com::map::QubitMapping v2r{
        get_num_qubits(platform),
        true,
        options->assume_initialized ? com::map::QubitState::INITIALIZED : com::map::QubitState::NONE
    };

    v2r_in = v2r;
    return route(block, v2r);
}

void Mapper::map(ir::ProgramRef program) {
    if (program->blocks.size() >= 2) {
        QL_FATAL("Inter-block mapping is not implemented. The mapper/router will only work for programs which consist of a single block.");
        QL_ASSERT(false);
        return;
    }

    auto& block = program->blocks[0];

    using clock = std::chrono::high_resolution_clock;
    auto t1 = clock::now();

    auto stats = map_block(block.as<ir::BlockBase>());

    auto t2 = clock::now();
    std::chrono::duration<Real> time_span = t2 - t1;
    auto time_taken = time_span.count();

    using pass::ana::statistics::AdditionalStats;
    AdditionalStats::push(block, "swaps added: " + to_string(stats.num_swaps_added));
    AdditionalStats::push(block, "of which moves added: " + to_string(stats.num_moves_added));
    AdditionalStats::push(block, "virt2real map before mapper:" + to_string(v2r_in.get_virt_to_real()));
    AdditionalStats::push(block, "virt2real map after mapper:" + to_string(v2r_out.get_virt_to_real()));
    AdditionalStats::push(block, "realqubit states before mapper:" + to_string(v2r_in.get_state()));
    AdditionalStats::push(block, "realqubit states after mapper:" + to_string(v2r_out.get_state()));
    AdditionalStats::push(block, "time taken: " + to_string(time_taken));
    AdditionalStats::push(program, "Total no. of swaps added by router pass: " + to_string(stats.num_swaps_added));
    AdditionalStats::push(program, "Total no. of moves added by router pass: " + to_string(stats.num_moves_added));
    AdditionalStats::push(program, "Total time taken by router pass: " + to_string(time_taken));
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
