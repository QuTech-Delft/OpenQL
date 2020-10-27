#pragma once

#include <string>
#include "qasm_semantic.hpp"

namespace ql {

class quantum_kernel;
class quantum_platform;
class quantum_program;

class cqasm_reader {
public:
    cqasm_reader(const quantum_platform &q_platform, quantum_program &q_program);
    ~cqasm_reader() = default;

    void string2circuit(const std::string &cqasm_str);
    void file2circuit(const std::string &cqasm_file_path);

private:
    void add_cqasm(compiler::QasmRepresentation cqasm_repr);
    static void add_kernel_operation(quantum_kernel &kernel, const compiler::Operation &operation, int number_of_qubits);
    static void add_single_bit_kernel_operation(quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation &operation);
    static void add_parameterized_single_bit_kernel_operation(quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation &operation);
    static void add_dual_bit_kernel_operation(quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation &op);
    static void add_parameterized_dual_bit_kernel_operation(quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation &operation);
    static void add_triple_bit_kernel_operation(quantum_kernel &kernel, const std::string &gate_type, const compiler::Operation &op);

    static std::string translate_gate_type(const std::string &gate_type);

    bool test_translate_gate_type();
    
    const quantum_platform &platform;
    quantum_program &program;
    int number_of_qubits;
    size_t sub_circuits_default_nr;
};

} // namespace ql
