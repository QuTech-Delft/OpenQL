#include <iostream>
#include <utils.h>
#include <openql.h>


void test_single_bit_kernel_operations()
{
    IOUT("test_single_bit_kernel_operations");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_single_bit_kernel_operations", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_parameterized_single_bit_kernel_operations()
{
    IOUT("test_parameterized_single_bit_kernel_operations");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_parameterized_single_bit_kernel_operations", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_dual_bit_kernel_operations()
{
    IOUT("test_dual_bit_kernel_operations");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_dual_bit_kernel_operations", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_parameterized_dual_bit_kernel_operations()
{
    IOUT("test_parameterized_dual_bit_kernel_operations");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_parameterized_dual_bit_kernel_operations", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_triple_bit_kernel_operations()
{
    IOUT("test_triple_bit_kernel_operations");

    ql::options::set("decompose_toffoli", "AM");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_triple_bit_kernel_operations", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_sub_circuit_program()
{
    IOUT("test_sub_circuit_program");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_sub_circuit_program", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_parallel_program()
{
    IOUT("test_parallel_program");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_parallel_program", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_special_gates()
{
    IOUT("test_special_gates");
    IOUT("test_parallel_programs");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_special_gates", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_add_multiple_parts_of_cqasm()
{
    IOUT("test_add_multiple_programs");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_add_multiple_programs", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

void test_qi_example()
{
    IOUT("test_qi_example");
    // create platform
    ql::quantum_platform platform("seven_qubits_chip", "hardware_config_cc_light.json");
    size_t num_qubits = platform.get_qubit_number();
    // create program
    ql::quantum_program program("qasm_qi_example", platform, num_qubits);

    ql::cqasm_reader cqasm_rdr(platform, program);
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

int main(int argc, char ** argv)
{
    //ql::utils::logger::set_log_level("LOG_NOTHING");
    ql::utils::logger::set_log_level("LOG_WARNING");
    ql::options::set("write_qasm_files", "yes");

    try {
        COUT("Testing cqasm_reader");
        //the following tests run successfully for configuration hardware_config_cc_light.json
        test_qi_example();
        test_add_multiple_parts_of_cqasm();
        test_single_bit_kernel_operations();
        test_sub_circuit_program();
        test_parallel_program();

        //the following tests are not suitable for configuration hardware_config_cc_light.json
        //test_parameterized_single_bit_kernel_operations();
        //test_dual_bit_kernel_operations();
        //test_parameterized_dual_bit_kernel_operations();
        //test_triple_bit_kernel_operations();
        //test_special_gates();
    }
    catch (const std::runtime_error& e)
    {
        EOUT(e.what());
        std::cerr << e.what() << std::endl;
        std::cerr << std::flush;
    }
    catch (const ql::exception& e)
    {
        EOUT(e.what());
        std::cerr << e.what() << std::endl;
        std::cerr << std::flush;
    }
    return 0;
}
