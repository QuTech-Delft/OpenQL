/** \file
 * Header for Python interface.
 */

#pragma once

#include "openql.h"
#include "classical.h"
#include "unitary.h"
#include "compiler.h"
#include "options.h"

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
    std::string name;
    std::string config_file;
    ql::quantum_platform *platform;
    Platform();
    Platform(const std::string &name, const std::string &config_file);
    size_t get_qubit_number() const;
};

class CReg {
public:
    ql::creg *creg;
    CReg(size_t id);
    ~CReg();
};

class Operation {
public:
    ql::operation *operation;
    Operation(const CReg &lop, const std::string &op, const CReg &rop);
    Operation(const std::string &op, const CReg &rop);
    Operation(const CReg &lop);
    Operation(int val);
    ~Operation();
};

typedef std::complex<double> Complex;

/**
 * quantum unitary matrix interface
 */
class Unitary {
public:
    std::string name;
    ql::unitary *unitary;

    Unitary(const std::string &name, const std::vector<std::complex<double>> &matrix);
    ~Unitary();
    void decompose();
    static bool is_decompose_support_enabled();
};

/**
 * quantum kernel interface
 */
class Kernel {
public:
    std::string name;
    Platform platform;
    size_t qubit_count;
    size_t creg_count;
    size_t breg_count;
    ql::quantum_kernel *kernel;

    Kernel(const std::string &name);
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
    ~Kernel();
};


/**
 * quantum program interface
 */
class Program
{
public:
    std::string name;
    Platform platform;
    size_t qubit_count;
    size_t creg_count;
    size_t breg_count;
    ql::quantum_program *program;

    Program(const std::string &name);
    Program(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count,
        size_t creg_count = 0,
        size_t breg_count = 0
    );
    void set_sweep_points(const std::vector<double> &sweep_points);
    std::vector<double> get_sweep_points() const;
    void add_kernel(const Kernel &k);
    void add_program(const Program &p);
    void add_if(const Kernel &k, const Operation &operation);
    void add_if(const Program &p, const Operation &operation);
    void add_if_else(const Kernel &k_if, const Kernel &k_else, const Operation &operation);
    void add_if_else(const Program &p_if, const Program &p_else, const Operation &operation);
    void add_do_while(const Kernel &k, const Operation &operation);
    void add_do_while(const Program &p, const Operation &operation);
    void add_for(const Kernel &k, size_t iterations);
    void add_for(const Program &p, size_t iterations);
    void compile();
    std::string microcode() const;
    void print_interaction_matrix() const;
    void write_interaction_matrix() const;
    ~Program();
};

/**
 * cqasm reader interface
 */
class cQasmReader {
public:
    ql::cqasm_reader *cqasm_reader_;
    Platform platform;
    Program program;

    cQasmReader(const Platform &q_platform, const Program &q_program);
    cQasmReader(const Platform &q_platform, const Program &q_program, const std::string &gateset_fname);
    void string2circuit(const std::string &cqasm_str);
    void file2circuit(const std::string &cqasm_file_path);
    ~cQasmReader();
};


/**
 * quantum compiler interface
 */
class Compiler {
public:
    std::string name;
    ql::quantum_compiler *compiler;

    Compiler(const std::string &name);
    void compile(Program &program);
    void add_pass_alias(const std::string &realPassName, const std::string &symbolicPassName);
    void add_pass(const std::string &realPassName);
    void set_pass_option(
        const std::string &passName,
        const std::string &optionName,
        const std::string &optionValue
    );
};
