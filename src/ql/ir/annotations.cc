/** \file
 * Defines common types and utility functions related to the statistics passes.
 */

#include "ql/ir/annotations.h"

namespace ql {
namespace ir {

/**
 * Attaches a statistic to the given node.
 */
static void push_node(
    utils::tree::annotatable::Annotatable &node,
    const utils::Str &line
) {
    if (!node.has_annotation<AdditionalStats>()) {
        node.set_annotation<AdditionalStats>({});
    }
    node.get_annotation<AdditionalStats>().stats.push_back(line);
}

/**
 * Attaches a statistic to the given kernel node.
 */
void AdditionalStats::push(const ir::BlockRef &block, const utils::Str &line) {
    push_node(*block, line);
}

/**
 * Attaches a statistic to the given program node.
 */
void AdditionalStats::push(const ir::ProgramRef &program, const utils::Str &line) {
    push_node(*program, line);
}

/**
 * Attaches a statistic to the given kernel node.
 */
void AdditionalStats::push(const ir::compat::KernelRef &kernel, const utils::Str &line) {
    push_node(*kernel, line);
}

/**
 * Attaches a statistic to the given program node.
 */
void AdditionalStats::push(const ir::compat::ProgramRef &program, const utils::Str &line) {
    push_node(*program, line);
}

/**
 * Pops all statistics annotations from the given node.
 */
static utils::List<utils::Str> pop_node(utils::tree::annotatable::Annotatable &node) {
    if (auto s = node.get_annotation_ptr<AdditionalStats>()) {
        auto stats = std::move(s->stats);
        node.erase_annotation<AdditionalStats>();
        return stats;
    } else {
        return {};
    }
}

/**
 * Pops all statistics annotations from the given kernel.
 */
utils::List<utils::Str> AdditionalStats::pop(const ir::BlockRef &kernel) {
    return pop_node(*kernel);
}

/**
 * Pops all statistics annotations from the given program.
 */
utils::List<utils::Str> AdditionalStats::pop(const ir::ProgramRef &program) {
    return pop_node(*program);
}

} // namespace ir
} // namespace ql
