#include <openql.h>

#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


namespace example_rb_single {

// clifford inverse lookup table for grounded state
const size_t inv_clifford_lut_gs[] = {0, 2, 1, 3, 8, 10, 6, 11, 4, 9, 5, 7, 12, 16, 23, 21, 13, 17, 18, 19, 20, 15, 22, 14};
//const size_t inv_clifford_lut_es[] = {3, 8, 10, 0, 2, 1, 9, 5, 7, 6, 11, 4, 21, 13, 17, 12, 16, 23, 15, 22, 14, 18, 19, 20};

typedef std::vector<int> cliffords_t;


/**
 * build rb circuit
 */
void build_rb(int num_cliffords, ql::Kernel &k) {
    assert((num_cliffords%2) == 0);
    int n = num_cliffords/2;

    cliffords_t cl;
    cliffords_t inv_cl;

    // add the clifford and its reverse
    for (int i=0; i<n; ++i) {
        int r = rand()%24;
        cl.push_back(r);
        inv_cl.insert(inv_cl.begin(), inv_clifford_lut_gs[r]);
    }
    cl.insert(cl.begin(),inv_cl.begin(),inv_cl.end());

    k.prepz(0);
    // build the circuit
    for (int i=0; i<num_cliffords; ++i) {
        k.clifford(cl[i], 0);
    }
    k.measure(0);
}

}  // namespace example_rb_single


TEST(v1_example, rb_single) {
    using namespace example_rb_single;

    srand(0);

    // create platform
    auto qx_platform = ql::Platform("qx_simulator", "none");

    // print info
    qx_platform.get_info();

    auto rb = ql::Program("rb", qx_platform, 1);

    auto kernel = ql::Kernel("rb1024", qx_platform, 1);

    build_rb(1024, kernel);

    rb.add_kernel(kernel);
    rb.compile();

    // std::cout<< rb.qasm() << std::endl;
    // std::cout << rb.qasm() << std::endl;
}
