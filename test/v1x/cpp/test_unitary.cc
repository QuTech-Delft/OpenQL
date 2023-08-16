#include <openql.h>

#include <gtest/gtest.h>
#include <numeric>


// Fails in CI on some platforms.
TEST(v1x, decomposition_controlled_u) {
    if (!ql::Unitary::is_decompose_support_enabled()) {
        GTEST_SKIP();
    }

    auto platform = ql::Platform("platform_none", "res/v1x/json/test_cfg_none_simple.json");
    auto num_qubits = 3;
    auto p = ql::Program("test_using_qx_toffoli", platform, num_qubits);
    auto k = ql::Kernel("aKernel", platform, num_qubits);

    std::vector<std::complex<double>> matrix = {
        1, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 0, 0, 0,
        0, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, std::complex<double>(0.30279949, -0.60010283), std::complex<double>(-0.58058628, -0.45946559),
        0, 0, 0, 0, 0 ,0, std::complex<double>(0.04481146, -0.73904059), std::complex<double>(0.64910478, 0.17456782)};

    auto u1 = ql::Unitary("using_qx_toffoli", matrix);
    u1.decompose();
    k.hadamard(0);
    k.hadamard(1);
    k.hadamard(2);
    k.gate(u1, { 0, 1, 2 });
    
    p.add_kernel(k);
    p.get_compiler().set_option("initialqasmwriter.cqasm_version", "1.0");
    p.get_compiler().set_option("initialqasmwriter.with_metadata", "no");
    p.compile();

    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix), std::begin(matrix) + 7, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 8, std::begin(matrix) + 15, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 16, std::begin(matrix) + 23, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 24, std::begin(matrix) + 31, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 32, std::begin(matrix) + 39, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 40, std::begin(matrix) + 47, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 48, std::begin(matrix) + 55, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(-0.38284984896211677, 0.058372391728338066)));
    EXPECT_DOUBLE_EQ(0.125 * std::norm(std::accumulate(std::begin(matrix) + 56, std::begin(matrix) + 63, std::complex<double>{0.0, 0.0})),
                     std::norm(std::complex<double>(-0.17273355873910606, -0.2649184303119007)));
}
