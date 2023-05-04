#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "openql.h"

// Fails in CI on some platforms.
TEST_CASE("decomposition_controlled_U" * doctest::skip(!ql::Unitary::is_decompose_support_enabled())) {
        auto platform = ql::Platform("platform_none", "test_cfg_none_simple.json");
        auto num_qubits = 3;
        auto p = ql::Program("test_usingqx_toffoli", platform, num_qubits);
        auto k = ql::Kernel("akernel", platform, num_qubits);

        std::vector<std::complex<double>> matrix = { 1, 0, 0, 0, 0, 0, 0, 0,
                                                     0, 1, 0, 0, 0, 0, 0, 0,
                                                     0, 0, 1, 0, 0, 0, 0, 0,
                                                     0, 0, 0, 1, 0, 0, 0, 0,
                                                     0, 0, 0, 0, 1, 0, 0, 0,
                                                     0, 0, 0, 0, 0, 1, 0, 0,
                                                     0, 0, 0, 0, 0, 0, std::complex<double>(0.30279949, -0.60010283), std::complex<double>(-0.58058628, -0.45946559),
                                                     0, 0, 0, 0, 0 ,0, std::complex<double>(0.04481146, -0.73904059),  std::complex<double>(0.64910478, 0.17456782)};

        auto u1 = ql::Unitary("arbitrarycontrolled",matrix);
        u1.decompose();
        k.hadamard(0);
        k.hadamard(1);
        k.hadamard(2);
        k.gate(u1, { 0, 1, 2 });


        p.add_kernel(k);
        p.get_compiler().set_option("initialqasmwriter.cqasm_version", "1.0");
        p.get_compiler().set_option("initialqasmwriter.with_metadata", "no");
        p.compile();

        CHECK_EQ(0.125*std::norm((matrix[0]  + matrix[1] + matrix[2] + matrix[3] + matrix[4] + matrix[5] + matrix[6] + matrix[7])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[8]  + matrix[9] + matrix[10]+ matrix[11]+ matrix[12]+ matrix[13]+ matrix[14]+ matrix[15])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[16] + matrix[17]+ matrix[18]+ matrix[19]+ matrix[20]+ matrix[21]+ matrix[22]+ matrix[23])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[24] + matrix[25]+ matrix[26]+ matrix[27]+ matrix[28]+ matrix[29]+ matrix[30]+ matrix[31])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[32] + matrix[33]+ matrix[34]+ matrix[35]+ matrix[36]+ matrix[37]+ matrix[38]+ matrix[39])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[40] + matrix[41]+ matrix[42]+ matrix[43]+ matrix[44]+ matrix[45]+ matrix[46]+ matrix[47])), doctest::Approx(std::norm(std::complex<double>(0.03708885276142243, 0.3516026407762626))));
        CHECK_EQ(0.125*std::norm((matrix[48] + matrix[49]+ matrix[50]+ matrix[51]+ matrix[52]+ matrix[53]+ matrix[54]+ matrix[55])), doctest::Approx(std::norm(std::complex<double>(-0.38284984896211677, 0.058372391728338066))));
        CHECK_EQ(0.125*std::norm((matrix[56] + matrix[57]+ matrix[58]+ matrix[59]+ matrix[60]+ matrix[61]+ matrix[62]+ matrix[63])), doctest::Approx(std::norm(std::complex<double>(-0.17273355873910606, -0.2649184303119007))));
}