/**
 * @file   program.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql program
 */

#pragma once

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
    std::string                 config_file_name;
    std::vector<quantum_kernel> kernels;
    std::string           name;
    std::string           unique_name;
    std::vector<float>    sweep_points;
    ql::quantum_platform  platform;
    bool                  platformInitialized;
    size_t                qubit_count;
    size_t                creg_count;
    size_t                breg_count;
    std::string           eqasm_compiler_name;
    bool                  needs_backend_compiler;
    ql::eqasm_compiler    *backend_compiler;

public:
    quantum_program(const std::string &n);
    quantum_program(const std::string &n, const quantum_platform &platf, size_t nqubits, size_t ncregs = 0, size_t nbregs = 0);

    void add(const ql::quantum_kernel &k);
    void add_program(const ql::quantum_program &p);
    void add_if(const ql::quantum_kernel &k, const ql::operation &cond);
    void add_if(const ql::quantum_program &p, const ql::operation &cond);
    void add_if_else(const ql::quantum_kernel &k_if, const ql::quantum_kernel &k_else, const ql::operation &cond);
    void add_if_else(const ql::quantum_program &p_if, const ql::quantum_program &p_else, const ql::operation &cond);
    void add_do_while(const ql::quantum_kernel &k, const ql::operation &cond);
    void add_do_while(const ql::quantum_program &p, const ql::operation &cond);
    void add_for(const ql::quantum_kernel &k, size_t iterations);
    void add_for(const ql::quantum_program &p, size_t iterations);

    void set_config_file(const std::string &file_name);
    void set_platform(const quantum_platform &platform);

    int compile();
    int compile_modular();

    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    void set_sweep_points(const float *swpts, size_t size);

    std::vector<quantum_kernel> &get_kernels();
    const std::vector<quantum_kernel> &get_kernels() const;

};

} // namespace ql
