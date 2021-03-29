/** \file
 * Quantum program abstraction implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/tree.h"
#include "ql/ir/kernel.h"
#include "platform.h"

namespace ql {

class eqasm_compiler;

namespace ir {

class Program;
using ProgramRef = utils::One<Program>;

/**
 * quantum_program_
 */
class Program : public utils::Node {
public:
    utils::Bool                 default_config;
    utils::Str                  config_file_name;
    KernelRefs                  kernels;
    utils::Str                  name;
    utils::Str                  unique_name;
    utils::Vec<utils::Real>     sweep_points;
    quantum_platform            platform;
    utils::Bool                 platformInitialized;
    utils::UInt                 qubit_count;
    utils::UInt                 creg_count;
    utils::UInt                 breg_count;
    utils::Str                  eqasm_compiler_name;
    utils::Bool                 needs_backend_compiler;
    utils::Ptr<eqasm_compiler>  backend_compiler;

public:
    Program(const utils::Str &n);
    Program(const utils::Str &n, const quantum_platform &platf, utils::UInt nqubits, utils::UInt ncregs = 0, utils::UInt nbregs = 0);

    void add(KernelRef &k);
    void add_program(ProgramRef &p);
    void add_if(KernelRef &k, const ClassicalOperation &cond);
    void add_if(ProgramRef &p, const ClassicalOperation &cond);
    void add_if_else(KernelRef &k_if, KernelRef &k_else, const ClassicalOperation &cond);
    void add_if_else(ProgramRef &p_if, ProgramRef &p_else, const ClassicalOperation &cond);
    void add_do_while(KernelRef &k, const ClassicalOperation &cond);
    void add_do_while(ProgramRef &p, const ClassicalOperation &cond);
    void add_for(KernelRef &k, utils::UInt iterations);
    void add_for(ProgramRef &p, utils::UInt iterations);

    void set_config_file(const utils::Str &file_name);
    void set_platform(const quantum_platform &platform);

    void compile();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    void set_sweep_points(const utils::Real *swpts, utils::UInt size);

    KernelRefs &get_kernels();
    const KernelRefs &get_kernels() const;

};

} // namespace ir
} // namespace ql
