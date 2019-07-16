/**
 * @file   program.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql program
 */

#ifndef QL_PROGRAM_H
#define QL_PROGRAM_H

#include <compile_options.h>
#include <platform.h>
#include <kernel.h>
#include <eqasm_compiler.h>

namespace ql
{

/**
 * quantum_program_
 */
class quantum_program
{
    protected:
        bool                        default_config;
        std::string                 config_file_name;
        std::vector<quantum_kernel> kernels;

    public:
        std::string           name;
        std::vector<float>    sweep_points;
        ql::quantum_platform  platform;
        size_t                qubit_count;
        size_t                creg_count;
        std::string           eqasm_compiler_name;
        ql::eqasm_compiler *  backend_compiler;

    public:
        quantum_program(std::string n, quantum_platform platf, size_t nqubits, size_t ncregs = 0);

        void add(ql::quantum_kernel &k);
        void add_program(ql::quantum_program p);
        void add_if(ql::quantum_kernel &k, ql::operation & cond);
        void add_if(ql::quantum_program p, ql::operation & cond);
        void add_if_else(ql::quantum_kernel &k_if, ql::quantum_kernel &k_else, ql::operation & cond);
        void add_if_else(ql::quantum_program &p_if, ql::quantum_program &p_else, ql::operation & cond);
        void add_do_while(ql::quantum_kernel &k, ql::operation & cond);
        void add_do_while(ql::quantum_program p, ql::operation & cond);
        void add_for(ql::quantum_kernel &k, size_t iterations);
        void add_for(ql::quantum_program p, size_t iterations);

        void set_config_file(std::string file_name);
        void set_platform(quantum_platform & platform);
        std::string qasm();

#if OPT_MICRO_CODE
        std::string microcode();
        std::string uc_header();
#endif

        int compile();
        void schedule();        // schedule and write scheduled qasm. Note that the backend may use a different scheduler with different results
        void print_interaction_matrix();
        void write_interaction_matrix();
        void set_sweep_points(float * swpts, size_t size);
};

} // ql

#endif //QL_PROGRAM_H
