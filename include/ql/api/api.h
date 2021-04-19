/** \file
 * Header for Python interface.
 */

#pragma once

#include "ql/ir/ir.h"
#include "ql/com/options.h"
#include "ql/com/unitary.h"
#include "ql/pass/io/cqasm/read.h"

namespace ql {
namespace api {

void initialize();

std::string get_version();

void set_option(const std::string &option_name, const std::string &option_value);
std::string get_option(const std::string &option_name);
void print_options();

/**
 * quantum program interface
 */
class Platform {
public:
    ql::plat::PlatformRef _platform;

    const std::string name;
    const std::string config_file;

    Platform(const std::string &name, const std::string &config_file);
    size_t get_qubit_number() const;
};

class CReg {
public:
    ql::utils::Ptr<ql::ir::ClassicalRegister> _creg;

    CReg(size_t id);
};

class Operation {
public:
    ql::utils::Ptr<ql::ir::ClassicalOperation> _operation;

    Operation(const CReg &lop, const std::string &op, const CReg &rop);
    Operation(const std::string &op, const CReg &rop);
    Operation(const CReg &lop);
    Operation(int val);
};

typedef std::complex<double> Complex;

/**
 * quantum unitary matrix interface
 */
class Unitary {
public:
    ql::utils::Ptr<ql::com::Unitary> _unitary;

    const std::string name;

    Unitary(const std::string &name, const std::vector<std::complex<double>> &matrix);
    void decompose();
    static bool is_decompose_support_enabled();
};

/**
 * quantum kernel interface
 */
class Kernel {
public:
    ql::ir::KernelRef _kernel;

    const std::string name;
    const Platform platform;
    const size_t qubit_count;
    const size_t creg_count;
    const size_t breg_count;

    Kernel(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    void identity(size_t q0);
    void hadamard(size_t q0);
    void s(size_t q0);
    void sdag(size_t q0);
    void t(size_t q0);
    void tdag(size_t q0);
    void x(size_t q0);
    void y(size_t q0);
    void z(size_t q0);
    void rx90(size_t q0);
    void mrx90(size_t q0);
    void rx180(size_t q0);
    void ry90(size_t q0);
    void mry90(size_t q0);
    void ry180(size_t q0);
    void rx(size_t q0, double angle);
    void ry(size_t q0, double angle);
    void rz(size_t q0, double angle);
    void measure(size_t q0);
    void measure(size_t q0, size_t b0);
    void prepz(size_t q0);
    void cnot(size_t q0, size_t q1);
    void cphase(size_t q0, size_t q1);
    void cz(size_t q0, size_t q1);
    void toffoli(size_t q0, size_t q1, size_t q2);
    void clifford(int id, size_t q0);
    void wait(const std::vector<size_t> &qubits, size_t duration);
    void barrier(const std::vector<size_t> &qubits = std::vector<size_t>());
    std::string get_custom_instructions() const;
    void display();
    void gate(const std::string &gname, size_t q0);
    void gate(const std::string &gname, size_t q0, size_t q1);
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const std::string &condstring = "COND_ALWAYS",
        const std::vector<size_t> &condregs = {}
    );
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const CReg &destination
    );
    void gate_preset_condition(
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );
    void gate_clear_condition();
    void condgate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );
    void gate(const Unitary &u, const std::vector<size_t> &qubits);
    void classical(const CReg &destination, const Operation &operation);
    void classical(const std::string &operation);
    void controlled(
        const Kernel &k,
        const std::vector<size_t> &control_qubits,
        const std::vector<size_t> &ancilla_qubits
    );
    void conjugate(const Kernel &k);
};


/**
 * quantum program interface
 */
class Program
{
public:
    ql::ir::ProgramRef _program;

    const std::string name;
    const Platform platform;
    const size_t qubit_count;
    const size_t creg_count;
    const size_t breg_count;

    Program(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count,
        size_t creg_count = 0,
        size_t breg_count = 0
    );
    void set_sweep_points(const std::vector<double> &sweep_points);
    std::vector<double> get_sweep_points() const;
    void set_config_file(const std::string &config_file_name);
    void add_kernel(Kernel &k);
    void add_program(Program &p);
    void add_if(Kernel &k, const Operation &operation);
    void add_if(Program &p, const Operation &operation);
    void add_if_else(Kernel &k_if, Kernel &k_else, const Operation &operation);
    void add_if_else(Program &p_if, Program &p_else, const Operation &operation);
    void add_do_while(Kernel &k, const Operation &operation);
    void add_do_while(Program &p, const Operation &operation);
    void add_for(Kernel &k, size_t iterations);
    void add_for(Program &p, size_t iterations);
    void compile();
    std::string microcode() const;
    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
};

/**
 * cqasm reader interface
 */
class cQasmReader {
public:
    ql::utils::Ptr<ql::pass::io::cqasm::read::Reader> _cqasm_reader;

    const Platform platform;
    const Program program;

    cQasmReader(const Platform &q_platform, const Program &q_program);
    cQasmReader(const Platform &q_platform, const Program &q_program, const std::string &gateset_fname);
    void string2circuit(const std::string &cqasm_str);
    void file2circuit(const std::string &cqasm_file_path);
};

} // namespace api
} // namespace ql
