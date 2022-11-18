/** \file
 * Future implementation.
 */

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
    virtual void advance(const ir::CustomInstructionRef& gate) = 0;

    virtual utils::List<ir::CustomInstructionRef> getCurrent() = 0;

    static std::shared_ptr<GateIterator> make(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options) {
        if (options->lookahead_mode == LookaheadMode::DISABLED) {
            return makeCircuitOrderIterator(block);
        }

        // FIXME: platform shouldn't be needed. Currently DDG needs implicit_bit_type to work, unfortunate...
        // implicit_bit_type should just be bit_type.
        return makeTopologicalOrderIterator(platform, block, options);
    }

private:
    static std::shared_ptr<GateIterator> makeCircuitOrderIterator(const ir::BlockBaseRef &block);

    static std::shared_ptr<GateIterator> makeTopologicalOrderIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options);
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

private:
    const ir::BlockBaseRef block;
    utils::Many<ir::Statement>::iterator it;
};

std::shared_ptr<GateIterator> GateIterator::makeCircuitOrderIterator(const ir::BlockBaseRef &block) {
    return std::shared_ptr<CircuitOrderGateIterator>(new CircuitOrderGateIterator(block));
}

class TopologicalOrderGateIterator : public GateIterator {
public:
    TopologicalOrderGateIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &b, const OptionsRef &options) :
        block(b) { 
        // Build DDG and add it as annotation to IR.
        com::ddg::build(platform, block, options->commute_multi_qubit, options->commute_single_qubit);
        
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
            QL_ASSERT(succ->get_annotation<com::ddg::NodeRef>()->predecessors.count(graph.source) == 1);
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
                next.push_back(succ);
            }
        }
    }

    virtual utils::List<ir::CustomInstructionRef> getCurrent() override {
        return next;
    }

private:
    const ir::BlockBaseRef block;
    std::set<ir::StatementRef> done;
    std::list<ir::CustomInstructionRef> next;
};

std::shared_ptr<GateIterator> GateIterator::makeTopologicalOrderIterator(const ir::PlatformRef& platform, const ir::BlockBaseRef &block, const OptionsRef &options) {
    return std::shared_ptr<TopologicalOrderGateIterator>(new TopologicalOrderGateIterator(platform, block, options));
}

Future::Future(const ir::PlatformRef &p, const OptionsRef &opt, const ir::BlockBaseRef &block) :
    platform(p), options(opt), gateIterator(GateIterator::make(platform, block, options)),
    approx_gates_total(block->statements.size()), approx_gates_remaining(approx_gates_total) {}

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

ir::CustomInstructionRef Future::get_most_critical(const std::vector<ir::CustomInstructionRef> &lag) const {
    QL_ASSERT(!lag.empty());
    
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        return lag.front();
    } else {
        return lag.front();
        // FIXME: implement remaining in current DDG
        // return scheduler->find_mostcritical(lag);
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
