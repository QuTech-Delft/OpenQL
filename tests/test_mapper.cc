#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include "ql/openql.h"
#include "ql/utils.h"

// all cnots with operands that are neighbors in s7
void
test_0()
{
    // create and set platform
    ql::quantum_platform starmon("starmon","test_cfg_none_s7.json");
    ql::set_platform(starmon);

    // create program
    ql::quantum_program prog(("test_0_"), 7, starmon);
    ql::quantum_kernel k("kernel_0",starmon);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    // a list of all cnots that are ok in trivial mapping
    k.gate("cnot", 0,2);
    k.gate("cnot", 0,3);
    k.gate("cnot", 1,3);
    k.gate("cnot", 1,4);
    k.gate("cnot", 2,0);
    k.gate("cnot", 2,5);
    k.gate("cnot", 3,0);
    k.gate("cnot", 3,1);
    k.gate("cnot", 3,5);
    k.gate("cnot", 3,6);
    k.gate("cnot", 4,1);
    k.gate("cnot", 4,6);
    k.gate("cnot", 5,2);
    k.gate("cnot", 5,3);
    k.gate("cnot", 6,3);
    k.gate("cnot", 6,4);

    prog.add(k);

    ql::options::set("mapper", "base");
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_1()
{
    // create and set platform
    ql::quantum_platform starmon("starmon","test_cfg_none_s7.json");
    ql::set_platform(starmon);

    // create program
    ql::quantum_program prog(("test_1_"), 7, starmon);
    ql::quantum_kernel k("kernel_1",starmon);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    // one cnot, but needs several swaps
    k.gate("cnot", 2,4);

    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", "base");
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_3()
{
    int n = 7;

    // create and set platform
    ql::quantum_platform starmon("starmon","test_cfg_none_s7.json");
    ql::set_platform(starmon);

    // create program
    ql::quantum_program prog(("test_3_"), 7, starmon);
    ql::quantum_kernel k("kernel_3",starmon);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    for (int i=0; i<n; i++)
        for (int j=0; j<n; j++)
	    if (i != j)
		k.gate("cnot", i,j);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", "base");
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");

    // test_0();

    test_1();

    // test_3();

    return 0;
}
