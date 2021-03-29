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
    plat::PlatformRef           platform;
    utils::Bool                 platformInitialized;
    utils::UInt                 qubit_count;
    utils::UInt                 creg_count;
    utils::UInt                 breg_count;

public:
    explicit Program(const utils::Str &n);
    Program(const utils::Str &n, const plat::PlatformRef &platf, utils::UInt nqubits, utils::UInt ncregs = 0, utils::UInt nbregs = 0);

    void add(const KernelRef &k);
    void add_program(const ProgramRef &p);
    void add_if(const KernelRef &k, const ClassicalOperation &cond);
    void add_if(const ProgramRef &p, const ClassicalOperation &cond);
    void add_if_else(const KernelRef &k_if, const KernelRef &k_else, const ClassicalOperation &cond);
    void add_if_else(const ProgramRef &p_if, const ProgramRef &p_else, const ClassicalOperation &cond);
    void add_do_while(const KernelRef &k, const ClassicalOperation &cond);
    void add_do_while(const ProgramRef &p, const ClassicalOperation &cond);
    void add_for(const KernelRef &k, utils::UInt iterations);
    void add_for(const ProgramRef &p, utils::UInt iterations);

    void set_config_file(const utils::Str &file_name);
    void set_platform(const plat::PlatformRef &platform);

    void compile();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    void set_sweep_points(const utils::Real *swpts, utils::UInt size);

    KernelRefs &get_kernels();
    const KernelRefs &get_kernels() const;

};

} // namespace ir
} // namespace ql
