#include <openql>
#include <ql/utils/exception.h>

#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

namespace fs = std::filesystem;
using namespace ql::utils;


namespace test_qi2_integration_test {

/**
 * Reads the given file into the given string buffer and returns true if it exists,
 * otherwise do nothing with the buffer and return false.
 */
bool read_file(const fs::path &file_path, std::string &output) {
    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        return false;
    }
    output.clear();
    ifs.seekg(0, std::ios::end);
    output.reserve(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    output.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return true;
}

void test_x90_q12__cnot_q1_q0() {
    QL_IOUT("test_x90_q12__cnot_q1_q0");

    auto platform = ql::Platform{ "qi2_integration_test", "res/v1x/json/spin-4.json" };
    auto compiler = platform.get_compiler();
    compiler.prefix_pass("io.cqasm.Read", "input", {
            { "cqasm_file", "res/v1x/cq/test_x90_q12__cnot_q1_q0.cq" }
    });
    auto program = ql::Program{ "test_x90_q12__cnot_q1_q0", platform };
    program.get_compiler().insert_pass_after("input", "dec.Instructions", "decomposition");
    program.get_compiler().set_option("initialqasmwriter.cqasm_version", "3.0");
    program.get_compiler().set_option("initialqasmwriter.with_metadata", "no");
    program.compile();

    auto output_file_path = fs::path{ "test_output" } / "program.qasm";
    auto golden_file_path = fs::path{ "res" } / "v1x" / "qasm" / "golden" / "test_x90_q12__cnot_q1_q0.qasm";
    std::string output_file_contents{};
    std::string golden_file_contents{};
    EXPECT_TRUE(read_file(output_file_path, output_file_contents));
    EXPECT_TRUE(read_file(golden_file_path, golden_file_contents));
    EXPECT_TRUE(output_file_contents == golden_file_contents);
}

}  // namespace test_qi2_integration_test


TEST(v1x, test_qi2_integration_test) {
    using namespace test_qi2_integration_test;

    ql::initialize();
    ql::utils::logger::set_log_level("LOG_WARNING");
    ql::set_option("write_qasm_files", "yes");

    try {
        QL_COUT("Testing QI2 integration test");
        //test_instruction_not_recognized();
        test_x90_q12__cnot_q1_q0();

    } catch (const std::runtime_error& e) {
        QL_EOUT(e.what());
        std::cerr << e.what() << std::endl;
        std::cerr << std::flush;
    }
}
