/** \file
 * Quantum program abstraction implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/tree.h"
#include "ql/ir/kernel.h"
#include "ql/plat/platform.h"

namespace ql {
namespace ir {

class Program;

/**
 * A "reference" (actually a smart pointer) to a single program node.
 */
using ProgramRef = utils::One<Program>;

/**
 * Toplevel IR node, representing a complete program.
 */
class Program : public utils::Node {
public:

    /**
     * Program name given by the user.
     */
    utils::Str name;

    /**
     * Program name as used for output file generation. If the global
     * `unique_output` option is set, this may receive a numerical suffix w.r.t.
     * name to prevent overwriting files from previous runs. Otherwise, it's
     * just a copy of name.
     */
    utils::Str unique_name;

    /**
     * The platform that this program is intended for.
     */
    plat::PlatformRef platform;

    /**
     * Number of (virtual) qubits used by this program. Must be less than or
     * equal to the number of qubits in the platform. When the qubits represent
     * physical qubits (post-mapping), this must equal the number of qubits in
     * the platform.
     */
    utils::UInt qubit_count;

    /**
     * Number of (virtual) 32-bit general-purpose classical registers used by
     * this program. Must be less than or equal to the number of registers in
     * the platform.
     */
    utils::UInt creg_count;

    /**
     * Number of (virtual) single-bit condition registers used by this program.
     * Must be less than or equal to the number of registers in the platform.
     *
     * FIXME: code is not consistent about what a breg means. I (JvS) thought we
     *  were using the first num_qubits bregs as registers that always exist and
     *  implicitly receive measurement results when no breg is manually
     *  specified, and use num_qubits..num_qubits+breg_count for user-specified
     *  state variables. But that's not how it works at all; bregs are still
     *  usually implicit, code all over the place assumes that bregs only range
     *  up to breg_count (exclusive), and breg_count defaults to zero. I don't
     *  get it.
     */
    utils::UInt breg_count;

    /**
     * The kernels that comprise the program.
     */
    KernelRefs kernels;

    /**
     * Constructs a new program.
     */
    Program(
        const utils::Str &name,
        const plat::PlatformRef &platform,
        utils::UInt qubit_count,
        utils::UInt creg_count = 0,
        utils::UInt breg_count = 0
    );

    /**
     * Adds the given kernel to the end of the program, after checking that it's
     * safe to add.
     */
    void add(const KernelRef &k);

    /**
     * Adds the kernels in the given (sub)program to the end of this program,
     * checking for each kernel whether it's safe to add.
     */
    void add_program(const ProgramRef &p);

    /**
     * Adds a conditional kernel, conditioned by a classical operation via
     * classical flow control.
     */
    void add_if(const KernelRef &k, const ClassicalOperation &cond);

    /**
     * Adds a conditional program, conditioned by a classical operation via
     * classical flow control.
     */
    void add_if(const ProgramRef &p, const ClassicalOperation &cond);

    /**
     * Adds two conditional kernels, conditioned by a classical operation and
     * its complement respectively via classical flow control.
     */
    void add_if_else(const KernelRef &k_if, const KernelRef &k_else, const ClassicalOperation &cond);

    /**
     * Adds two conditional programs, conditioned by a classical operation and
     * its complement respectively via classical flow control.
     */
    void add_if_else(const ProgramRef &p_if, const ProgramRef &p_else, const ClassicalOperation &cond);

    /**
     * Adds a do-while loop with the given kernel as the body.
     */
    void add_do_while(const KernelRef &k, const ClassicalOperation &cond);

    /**
     * Adds a do-while loop with the given program as the body.
     */
    void add_do_while(const ProgramRef &p, const ClassicalOperation &cond);

    /**
     * Adds a static for loop with the given kernel as the body.
     */
    void add_for(const KernelRef &k, utils::UInt iterations);

    /**
     * Adds a static for loop with the given program as the body.
     */
    void add_for(const ProgramRef &p, utils::UInt iterations);

    /**
     * Entry point for compilation.
     */
    void compile();

};

} // namespace ir
} // namespace ql
