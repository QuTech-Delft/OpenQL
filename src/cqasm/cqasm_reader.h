#ifndef _QL_CQASM_READER_H
#define _QL_CQASM_READER_H

#include <string>
#include "qasm_semantic.hpp"

namespace ql
{
    class quantum_kernel;
    class quantum_platform;
    class quantum_program;

    class cqasm_reader
    {
    public:
        cqasm_reader(const ql::quantum_platform& q_platform, ql::quantum_program& q_program);
        ~cqasm_reader();

        void string2circuit(const std::string& cqasm_str);
        void file2circuit(const std::string& cqasm_file_path);
    private:
        std::string translate_gate_type(const std::string& gate_type);

        void add_cqasm(compiler::QasmRepresentation cqasm_repr);
        void add_kernel_operation(ql::quantum_kernel& kernel, const compiler::Operation& operation, int number_of_qubits);
        void add_single_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation);
        void add_parameterized_single_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation);
        void add_dual_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& op);
        void add_parameterized_dual_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& operation);
        void add_triple_bit_kernel_operation(ql::quantum_kernel& kernel, const std::string& gate_type, const compiler::Operation& op);

        bool test_translate_gate_type();

        const ql::quantum_platform& platform;
        ql::quantum_program& program;
        int number_of_qubits;
        size_t sub_circuits_default_nr;
    };
}

#endif  //_QL_CQASM_READER_H
