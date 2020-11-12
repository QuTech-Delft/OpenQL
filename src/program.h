/** \file
 * Quantum program abstraction implementation.
 */

#pragma once

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
    bool                        default_config;
    utils::Str                  config_file_name;
    utils::Vec<quantum_kernel>  kernels;
    utils::Str                  name;
    utils::Str                  unique_name;
    utils::Vec<float>           sweep_points;
    quantum_platform        platform;
    bool                        platformInitialized;
    size_t                      qubit_count;
    size_t                      creg_count;
    utils::Str                  eqasm_compiler_name;
    bool                        needs_backend_compiler;
    eqasm_compiler          *backend_compiler;

public:
    quantum_program(const utils::Str &n);
    quantum_program(const utils::Str &n, const quantum_platform &platf, size_t nqubits, size_t ncregs = 0);

    void add(const quantum_kernel &k);
    void add_program(const quantum_program &p);
    void add_if(const quantum_kernel &k, const operation &cond);
    void add_if(const quantum_program &p, const operation &cond);
    void add_if_else(const quantum_kernel &k_if, const quantum_kernel &k_else, const operation &cond);
    void add_if_else(const quantum_program &p_if, const quantum_program &p_else, const operation &cond);
    void add_do_while(const quantum_kernel &k, const operation &cond);
    void add_do_while(const quantum_program &p, const operation &cond);
    void add_for(const quantum_kernel &k, size_t iterations);
    void add_for(const quantum_program &p, size_t iterations);

    void set_config_file(const utils::Str &file_name);
    void set_platform(const quantum_platform &platform);

    int compile();
    int compile_modular();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    void set_sweep_points(const float *swpts, size_t size);

    utils::Vec<quantum_kernel> &get_kernels();
    const utils::Vec<quantum_kernel> &get_kernels() const;

};

} // namespace ql
