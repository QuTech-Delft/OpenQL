#include "future.h"

#include "ql/utils/filesystem.h"
#include "ql/com/ddg/build.h"
#include "ql/com/ddg/types.h"
#include "ql/com/ddg/dot.h"
#include "ql/ir/describe.h"
#include "ql/ir/ops.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

class GateIterator {
public:
    virtual ~GateIterator() = default;

    virtual void advance(const ir::CustomInstructionRef& gate) = 0;

    virtual utils::List<ir::CustomInstructionRef> getCurrent() = 0;

    static std::unique_ptr<GateIterator> make(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options) {
        if (options->lookahead_mode == LookaheadMode::DISABLED) {
            return makeCircuitOrderIterator(block);
        }

        // FIXME: platform shouldn't be needed. Currently DDG needs implicit_bit_type to work, unfortunate...
        // implicit_bit_type should just be bit_type.
        return makeTopologicalOrderIterator(platform, block, options);
    }

    virtual std::unique_ptr<GateIterator> clone() = 0;

private:
    static std::unique_ptr<GateIterator> makeCircuitOrderIterator(const ir::BlockBaseRef &block);

    static std::unique_ptr<GateIterator> makeTopologicalOrderIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options);
};

class CircuitOrderGateIterator : public GateIterator {
public:
    CircuitOrderGateIterator(const ir::BlockBaseRef &b) : block(b) {
        it = block->statements.begin();
    }

    virtual void advance(const ir::CustomInstructionRef& gate) override {
        QL_ASSERT(gate == it->as<ir::CustomInstruction>());
        ++it;
    }

    virtual utils::List<ir::CustomInstructionRef> getCurrent() override {
        if (it == block->statements.end()) {
            return {};
        }

        auto asCustomInstruction = it->as<ir::CustomInstruction>();

        if (asCustomInstruction.empty()) {
            QL_FATAL("Statement currently not supported by router: " << ir::describe(*it));
        }

        return { asCustomInstruction };
    }

    virtual std::unique_ptr<GateIterator> clone() override {
        return std::unique_ptr<GateIterator>(new CircuitOrderGateIterator(*this));
    }

private:
    const ir::BlockBaseRef block;
    utils::Many<ir::Statement>::iterator it;
};

std::unique_ptr<GateIterator> GateIterator::makeCircuitOrderIterator(const ir::BlockBaseRef &block) {
    return std::unique_ptr<CircuitOrderGateIterator>(new CircuitOrderGateIterator(block));
}

class TopologicalOrderGateIterator : public GateIterator {
public:
    TopologicalOrderGateIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &b, const OptionsRef &options) :
        block(b) { 
        // Build DDG and add it as annotation to IR.
        com::ddg::build(platform, block, options->commute_multi_qubit, options->commute_single_qubit);
        com::ddg::add_remaining(block);

        if (options->write_dot_graphs) {
            utils::StrStrm dot_graph;
            utils::StrStrm fname;

            com::ddg::dump_dot(block, dot_graph);

            fname << options->output_prefix << "_" << "mapper" << ".dot"; // FIXME: uniquify for multiple blocks
            utils::OutFile(fname.str()).write(dot_graph.str());
        }
        
        const auto& graph = block->get_annotation<com::ddg::Graph>();
        done.insert(graph.source);
        for (const auto& node_edge: graph.source->get_annotation<com::ddg::NodeRef>()->successors) {
            const auto& succ = node_edge.first;

            const auto& preds = succ->get_annotation<com::ddg::NodeRef>()->predecessors;
            QL_ASSERT(std::find_if(preds.begin(), preds.end(),
                [&graph](const std::pair<ir::StatementRef, com::ddg::EdgeRef> &x) { return x.first == graph.source; }) != preds.end());

            if (succ->get_annotation<com::ddg::NodeRef>()->predecessors.size() == 1) {
                next.push_back(succ);
            };
        }
    }

    virtual void advance(const ir::CustomInstructionRef& gate) override {
        QL_ASSERT(std::find(next.begin(), next.end(), gate) != next.end());
        next.remove(gate);
        done.insert(gate);

        for (const auto& node_edge: gate->get_annotation<com::ddg::NodeRef>()->successors) {
            if (node_edge.first->as_sentinel_statement()) {
                QL_ASSERT(node_edge.first == block->get_annotation<com::ddg::Graph>().sink);
                continue;
            }

            auto succ = node_edge.first.as<ir::CustomInstruction>();

            if (succ.empty()) {
                QL_FATAL("Statement currently not supported by router: " << ir::describe(*node_edge.first));
            }

            bool allPredAreDone = true;
            for (const auto& pred: succ->get_annotation<com::ddg::NodeRef>()->predecessors) {
                if (done.count(pred.first) < 1) {
                    allPredAreDone = false;
                }
            }

            if (allPredAreDone) {
                // Gate is "available", insert it into "next" while keeping "next" sorted by decreasing criticality and by topological order.
                // "Criticality" is the number of cycles of the shortest path to the sink DDG node (called "remaining"). In case of tie,
                // the statement with the highest number of successors is more critical.

                auto remainingOfSucc = succ->get_annotation<com::ddg::Remaining>().remaining;
                auto numberOfSuccessorsOfSucc = com::ddg::get_node(succ)->successors.size();
                auto limit = std::make_pair(remainingOfSucc, numberOfSuccessorsOfSucc);
                auto whereToInsert = std::upper_bound(next.begin(), next.end(), limit, [](std::pair<utils::UInt, std::size_t> const& c, ir::CustomInstructionRef const& statement) {
                    auto remainingOfStatement = statement->get_annotation<com::ddg::Remaining>().remaining;
                    auto numberOfSuccessorsOfStatement = com::ddg::get_node(statement)->successors.size();
                    
                    if (remainingOfStatement == c.first) {
                        return numberOfSuccessorsOfStatement < c.second;
                    }

                    return remainingOfStatement < c.first;
                });
                next.insert(whereToInsert, succ);
            }
        }
    }

    virtual std::unique_ptr<GateIterator> clone() override {
        return std::unique_ptr<GateIterator>(new TopologicalOrderGateIterator(*this));
    }

    virtual utils::List<ir::CustomInstructionRef> getCurrent() override {
        return next;
    }

private:
    const ir::BlockBaseRef block;
    std::set<ir::StatementRef> done;
    std::list<ir::CustomInstructionRef> next;
};

std::unique_ptr<GateIterator> GateIterator::makeTopologicalOrderIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options) {
    return std::unique_ptr<TopologicalOrderGateIterator>(new TopologicalOrderGateIterator(platform, block, options));
}

Future::Future(const ir::PlatformRef &p, const OptionsRef &opt, const ir::BlockBaseRef &block) :
    platform(p), options(opt), gateIterator(GateIterator::make(platform, block, options)),
    approx_gates_total(block->statements.size()), approx_gates_remaining(approx_gates_total) {}

Future::Future(const Future& rhs) :
    platform(rhs.platform), options(rhs.options), gateIterator(rhs.gateIterator->clone()),
    approx_gates_total(rhs.approx_gates_total), approx_gates_remaining(rhs.approx_gates_remaining) {};

Future::~Future() = default;

utils::List<ir::CustomInstructionRef> Future::get_schedulable_gates() const {
    return gateIterator->getCurrent();
}

void Future::completed_gate(const ir::CustomInstructionRef &gate) {
    QL_DOUT("Mapped input gate:  " << ir::describe(gate));

    if (approx_gates_remaining > 0) {
        approx_gates_remaining--;
    }

    gateIterator->advance(gate);
}

utils::Real Future::get_progress() {
    utils::Real progress = 1.0;
    if (approx_gates_total) {
        progress -= ((utils::Real)approx_gates_remaining / (utils::Real)approx_gates_total);
    }

    return progress;
}

ir::CustomInstructionRef Future::get_most_critical(const std::vector<ir::CustomInstructionRef> &gates) const {
    QL_ASSERT(!gates.empty());
    
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        return gates.front();
    } else {
        utils::UInt maxRemaining = 0;
        ir::CustomInstructionRef result;
        for (const auto& gate: gates) {
            auto remaining = gate->get_annotation<com::ddg::Remaining>().remaining;

            if (remaining > maxRemaining) {
                maxRemaining = remaining;
                result = gate;
            }
        }

        return result;
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
