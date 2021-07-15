/** \file
 * Control-flow structure decomposition implementation (i.e. conversion to basic
 * block form).
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Decomposes the control-flow structure of the program in the given IR such
 * that it is in basic block form. Specifically:
 *
 *  - all blocks consist of only instructions (no control-flow statements like
 *    loops or if-conditionals); and
 *  - only the last instruction of each block may be a goto instruction.
 *
 * The ir tree is not modified. Instead, a new program node is returned. This
 * node is such that the original program node in ir can be replaced with it.
 * Note that nodes/subtrees may be shared between the structured and basic block
 * representations of the programs.
 *
 * If check is set, a consistency and basic-block form check is done before
 * returning the created program. This is also done if debugging is enabled via
 * the loglevel.
 */
ir::ProgramRef decompose_structure(const ir::Ref &ir, utils::Bool check = false);

/**
 * Returns whether the given program is in basic block form, as defined by
 * decompose_structure(). Assumes that the program is otherwise consistent.
 */
utils::Bool is_in_basic_block_form(const ir::ProgramRef &program);

/**
 * Throws an appropriate exception if the given program is not in basic block
 * form. Assumes that the program is otherwise consistent.
 */
void check_basic_block_form(const ir::ProgramRef &program);

} // namespace dec
} // namespace com
} // namespace ql
