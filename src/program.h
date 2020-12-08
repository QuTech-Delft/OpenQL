/** \file
 * Quantum program abstraction implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "platform.h"
#include "kernel.h"

namespace ql {

class eqasm_compiler;

/**
 * quantum_program_
 */
class quantum_program {
public:
    utils::Bool                 default_config;
    utils::Str                  config_file_name;
    utils::Vec<quantum_kernel>  kernels;
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
    eqasm_compiler              *backend_compiler;

public:
    quantum_program(const utils::Str &n);
    quantum_program(const utils::Str &n, const quantum_platform &platf, utils::UInt nqubits, utils::UInt ncregs = 0, utils::UInt nbregs = 0);

    void add(const quantum_kernel &k);
    void add_program(const quantum_program &p);
    void add_if(const quantum_kernel &k, const operation &cond);
    void add_if(const quantum_program &p, const operation &cond);
    void add_if_else(const quantum_kernel &k_if, const quantum_kernel &k_else, const operation &cond);
    void add_if_else(const quantum_program &p_if, const quantum_program &p_else, const operation &cond);
    void add_do_while(const quantum_kernel &k, const operation &cond);
    void add_do_while(const quantum_program &p, const operation &cond);
    void add_for(const quantum_kernel &k, utils::UInt iterations);
    void add_for(const quantum_program &p, utils::UInt iterations);

    void set_config_file(const utils::Str &file_name);
    void set_platform(const quantum_platform &platform);

    void compile();
    void compile_modular();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    void set_sweep_points(const utils::Real *swpts, utils::UInt size);

    utils::Vec<quantum_kernel> &get_kernels();
    const utils::Vec<quantum_kernel> &get_kernels() const;

};

} // namespace ql
