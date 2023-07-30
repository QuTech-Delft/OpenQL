#include <openql>
#include <ql/utils/exception.h>

#include <gtest/gtest.h>
#include <iostream>

using namespace ql::utils;


namespace test_cqasm_reader {

void test_single_bit_kernel_operations() {
    QL_IOUT("test_single_bit_kernel_operations");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_single_bit_kernel_operations", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    "error_model depolarizing_channel, 0.001\n"
    "wait 6\n"
    "prep_z q[0:3]\n"
    "prep_z q[4]\n"
    "prep_z q[5]\n"
    "i q[0:3]\n"
    "h q[4]\n"
    "x q[1:3]\n"
    "y q[5]\n"
    "z q[0:1]\n"
    "s q[1]\n"
    "sdag q[2:3]\n"
    "t q[5]\n"
    "tdag q[1:3]\n"
    "x90 q[3]\n"
    "mx90 q[1]\n"
    "y90 q[0]\n"
    "my90 q[2:3]\n"
    "measure_z q[2:3]\n"
    "measure_z q[0, 1]\n"
    "measure_z q[4]\n"
    "measure_z q[5]\n");

    // compile the resulting program
    program.compile();
}

void test_parameterized_single_bit_kernel_operations() {
    QL_IOUT("test_parameterized_single_bit_kernel_operations");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_parameterized_single_bit_kernel_operations", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    "rx q[0], 3.14\n"
    "ry q[2], 1.2\n"
    "rz q[1], 3.14\n"
    "rx q[0:3], 3.14\n"
    "ry q[2, 5], 1.2\n"
    "rz q[0, 1], 3.14\n"
    "measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_dual_bit_kernel_operations() {
    QL_IOUT("test_dual_bit_kernel_operations");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_dual_bit_kernel_operations", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    "cz q[1], q[3]\n"
    "cnot q[0], q[1]\n"
    "cnot q[0:2], q[3:5]\n"
    "cz q[0,3], q[2,5]\n"
    "swap q[0:1], q[2:3]\n"
    "swap q[0], q[1]\n"
    "measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_parameterized_dual_bit_kernel_operations() {
    QL_IOUT("test_parameterized_dual_bit_kernel_operations");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_parameterized_dual_bit_kernel_operations", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    "crk q[0], q[1], 3\n"
    "crk q[0:1], q[2:3], 3\n"
    "cr q[2], q[3], 3.14\n"
    "cr q[0:2], q[3:5], 3.14\n"
    "measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_triple_bit_kernel_operations() {
    QL_IOUT("test_triple_bit_kernel_operations");

    ql::set_option("decompose_toffoli", "AM");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_triple_bit_kernel_operations", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    "h q[4]\n"
    "toffoli q[0:1], q[2:3], q[4:5]\n"
    "toffoli q[0], q[3], q[5]\n"
    "toffoli q[1], q[2], q[5]\n"
    "measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_sub_circuit_program() {
    QL_IOUT("test_sub_circuit_program");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_sub_circuit_program", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    ".init\n"
    "  prep_z q[0]\n"
    "  prep_z q[1]\n"
    ".do_somework(3)\n"
    "  x q[0]\n"
    "  h q[1]\n"
    ".do_measurement\n"
    "  measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_parallel_program() {
    QL_IOUT("test_parallel_program");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_parallel_program", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    ".init\n"
    "  { prep_z q[0] | prep_z q[1] }\n"
    ".do_somework(3)\n"
    "  { x q[0] | h q[1] }\n"
    ".do_measurement\n"
    "  { measure_z q[0] | measure_z q[1] }\n");

    // compile the resulting program
    program.compile();
}

void test_special_gates() {
    QL_IOUT("test_special_gates");
    QL_IOUT("test_parallel_programs");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_special_gates", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    ".init\n"
    "  { prep_z q[0] | prep_z q[1] }\n"
    "  display\n"
    ".do_somework(3)\n"
    "  { x q[0] | h q[1] }\n"
    "  wait 6\n"
    ".do_measurement\n"
    "  display\n"
    "  { measure_z q[0] | measure_z q[1] }\n"
    "  display_binary b[0]\n"
    "  display_binary b[2:3]\n"
    "  display_binary b[1]\n");

    // compile the resulting program
    program.compile();
}

void test_add_multiple_parts_of_cqasm() {
    QL_IOUT("test_add_multiple_programs");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_add_multiple_programs", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    ".init\n"
    "  prep_z q[0]\n"
    "  prep_z q[1]\n"
    ".do_somework(3)\n"
    "  x q[0]\n"
    "  h q[1]\n");
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 6\n"
    ".init\n"
    "  prep_z q[2]\n"
    "  prep_z q[3]\n"
    ".do_somework(3)\n"
    "  x q[2]\n"
    "  h q[3]\n"
    ".do_measurement\n"
    "  measure_all\n");

    // compile the resulting program
    program.compile();
}

void test_qi_example() {
    QL_IOUT("test_qi_example");
    // create platform
    ql::Platform platform("seven_qubits_chip",  os.path.join(json_dir, "config_cc_light.json"));
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::Program program("qasm_qi_example", platform, num_qubits);

    ql::cQasmReader cqasm_rdr(platform, program);
    cqasm_rdr.string2circuit(
    "version 1.0\n"
    "qubits 5\n"
    "prep_z q[0,1,2,3,4]\n"
    "y q[0,2]\n"
    "cz q[0], q[2]\n"
    "y90 q[2]\n"
    "measure_all\n");

    // compile the resulting program
    program.compile();
}

}  // namespace test_cqasm_reader


TEST(v1x, test_cqasm_reader) {
    using namespace test_cqasm_reader;

    //ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::utils::logger::set_log_level("LOG_WARNING");
    ql::set_option("write_qasm_files", "yes");

    try {
        QL_COUT("Testing cqasm_reader");
        //the following tests run successfully for configuration config_cc_light.json
        test_qi_example();
        test_add_multiple_parts_of_cqasm();
        test_single_bit_kernel_operations();
        test_sub_circuit_program();
        test_parallel_program();

        //the following tests are not suitable for configuration config_cc_light.json
        //test_parameterized_single_bit_kernel_operations();
        //test_dual_bit_kernel_operations();
        //test_parameterized_dual_bit_kernel_operations();
        //test_triple_bit_kernel_operations();
        //test_special_gates();
    } catch (const std::runtime_error& e) {
        QL_EOUT(e.what());
        std::cerr << e.what() << std::endl;
        std::cerr << std::flush;
    }
}
