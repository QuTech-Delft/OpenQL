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
     * TODO JvS: still not sure what this is.
     */
    utils::Vec<utils::Real> sweep_points;

    /**
     * Configuration file name for the sweep points pass. Leave empty to use the
     * default generated filename.
     *
     * FIXME: should not be here, should be a pass option if anything.
     */
    utils::Str sweep_points_config_file_name;

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

    void add_if(const KernelRef &k, const ClassicalOperation &cond);
    void add_if(const ProgramRef &p, const ClassicalOperation &cond);
    void add_if_else(const KernelRef &k_if, const KernelRef &k_else, const ClassicalOperation &cond);
    void add_if_else(const ProgramRef &p_if, const ProgramRef &p_else, const ClassicalOperation &cond);
    void add_do_while(const KernelRef &k, const ClassicalOperation &cond);
    void add_do_while(const ProgramRef &p, const ClassicalOperation &cond);
    void add_for(const KernelRef &k, utils::UInt iterations);
    void add_for(const ProgramRef &p, utils::UInt iterations);

    void compile();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;

    void set_config_file(const utils::Str &config_file);
    void set_sweep_points(const utils::Real *swpts, utils::UInt size);

    KernelRefs &get_kernels();
    const KernelRefs &get_kernels() const;

};

} // namespace ir
} // namespace ql
