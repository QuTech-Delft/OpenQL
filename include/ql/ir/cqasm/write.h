/** \file
 * cQASM 1.2 writer logic as human-readable complement of the IR.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace ir {
namespace cqasm {

/**
 * Options for writing cQASM files.
 */
struct WriteOptions {

    /**
     * The target cQASM version. Lowering this disables usage of cQASM features
     * as appropriate, but reduces the set of programs that can be represented.
     * Note that versions less than 1.2 support no control flow *at all*; not
     * even subcircuit repetition count is then supported.
     */
    utils::Vec<utils::UInt> version = {1, 2};

    /**
     * Whether to include an annotation that includes the (preprocessed) JSON
     * description of the platform.
     */
    utils::Bool include_platform = false;

    /**
     * Whether to include variable declarations for registers. If the file is
     * to be passed to a target that doesn't programmatically define mappings
     * for registers, this must be enabled. Note that the size of the main qubit
     * register is always printed for version 1.0, because it can't legally be
     * omitted for that version. Also note that this is a lossy operation if the
     * file is later read by OpenQL again, because register indices are lost
     * (since only scalar variables are supported by cQASM).
     */
    utils::Bool registers_as_variables = false;

    /**
     * Whether to include kernel and program statistics in comments.
     */
    utils::Bool include_statistics = false;

    /**
     * Whether to include metadata supported by the IR but not by cQASM as
     * annotations, to allow the IR to be more accurately reproduced when read
     * again via the cQASM reader.
     */
    utils::Bool include_metadata = true;

    /**
     * Whether to include wait and barrier instructions. These are only needed
     * when the program will be fed to another compiler later on. For cQASM
     * 1.1+, the syntax is:
     *
     *  - `barrier`: wait for all previous instructions to complete.
     *  - `wait N`: wait for all previous instructions to complete, then wait
     *    N cycles.
     *  - `barrier [...]`: wait for all previous instructions operating on the
     *    given objects to complete.
     *  - `wait N, [...]`: wait for all previous instructions operating on the
     *    given objects to complete, when wait N cycles.
     *
     * For cQASM 1.0, the syntax is simpler, but more restricted:
     *
     *  - `wait N`: wait for all previous instructions to complete, then wait
     *    N cycles (including 0 cycles, for a barrier on everything).
     *  - `barrier q[...]`: wait for all instructions operating on the qubits
     *    in the SGMQ list to complete.
     *
     * Note that cQASM 1.0 lacks a barrier instruction that includes a delay.
     */
    utils::Bool include_barriers = true;

};

/**
 * Writes a cQASM representation of the IR to the given stream with the given
 * line prefix.
 */
void write(
    const Ref &ir,
    const WriteOptions &options = {},
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace cqasm
} // namespace ir
} // namespace ql
