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
    ql::quantum_platform starmon("starmon","test_mapper.json");
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

    ql::options::set("mapper", "minextend");
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_1()
{
    // create and set platform
    ql::quantum_platform starmon("starmon","test_mapper.json");
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

    ql::options::set("mapper", "minextend");
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_2(std::string v, std::string mapopt, std::string schedopt)
{
    int n = 7;
    std::string prog_name;
    std::string kernel_name;

    // create and set platform
    ql::quantum_platform starmon("starmon","test_mapper.json");
    ql::set_platform(starmon);

    // create program
    prog_name = "test_" + v + "_" + mapopt + "_" + schedopt;
    kernel_name = "kernel_" + v + "_" + mapopt + "_" + schedopt;
    ql::quantum_program prog(prog_name, n, starmon);
    ql::quantum_kernel k(kernel_name, starmon);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    for (int i=0; i<n; i++)
        for (int j=0; j<n; j++)
	    if (i != j)
		k.gate("cnot", i,j);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

int main(int argc, char ** argv)
{
    ql::utils::logger::set_log_level("LOG_DEBUG");

    test_0();

    test_1();

    test_2("2", "base", "ASAP");
    test_2("2", "minextend", "ASAP");
    test_2("2", "minextend", "no");

    return 0;
}
