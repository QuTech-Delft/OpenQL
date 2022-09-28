/**
 * @file   arch/cc/pass/gen/vq1asm/detail/backend.h
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  'eqasm' backend for the Central Controller
 * @remark based on cc_light_eqasm_compiler.h, commit f34c0d9
 */

#pragma once

#include "ql/arch/cc/pass/gen/vq1asm/detail/types.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/codegen.h"

#include "ql/ir/ir.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class OperandContext;

class Backend {
public:
    /**
     * Compile for Central Controller.
     */
    Backend(const ir::Ref &ir, const OptionsRef &options);

    ~Backend() = default;

private:
    /*
     * Generate code for a single block. Recursively calls itself where necessary.
     *
     * Based on NewToOldConverter::convert_block
     * Note that a block sort of matches the concept of a Kernel in the old API
     */
    void codegen_block(const ir::BlockBaseRef &block, const Str &name, Int depth);

private: // vars
    Codegen codegen;
    Int bundleIdx = -1;     // effectively, numbering starts at 0 because of pre-increment
    Int block_number = 0;   // sequential block number to keep labels unique
    Vec<Str> loop_label;    // stack for loop labels (in conjunction with 'break'/'continue' instruction)

}; // class

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
