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

// test resource constraints mapping
void
test_rm(std::string v, std::string mapopt, std::string schedopt)
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

    // no dependency, only a conflict in qwg resource
    k.gate("x", 0);
    k.gate("y", 1);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are neighbors in s7
void
test_0(std::string v, std::string mapopt, std::string schedopt)
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

    k.gate("x", 0);
    k.gate("x", 2);

    // one cnot that is ok in trivial mapping
    k.gate("cnot", 0,2);

    k.gate("x", 0);
    k.gate("x", 2);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// all cnots with operands that are neighbors in s7
void
test_1(std::string v, std::string mapopt, std::string schedopt)
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

    for (int j=0; j<7; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are at distance 2 in s7
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

    // one cnot, but needs one swap
    k.gate("cnot", 2,3);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// one cnot with operands that are at distance 4 in s7
void
test_3(std::string v, std::string mapopt, std::string schedopt)
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

    // one cnot, but needs several swaps
    k.gate("cnot", 2,4);

    for (int j=0; j<n; j++)
        k.gate("x", j);

    prog.add(k);

    ql::options::set("mapper", mapopt);
    ql::options::set("scheduler", schedopt);
    prog.compile( );
}

// all possible cnots in s7, in lexicographic order
// requires many swaps
void
test_4(std::string v, std::string mapopt, std::string schedopt)
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

//    test_rm("rm", "minextendrc", "no");

//    test_0("0", "base", "ASAP");
//    test_0("0", "minextend", "ASAP");
//    test_0("0", "minextend", "no");
//    test_0("0", "minextendrc", "no");

//    test_1("1", "base", "ASAP");
//    test_1("1", "minextend", "ASAP");
//    test_1("1", "minextend", "no");
//    test_1("1", "minextendrc", "no");

//    test_2("2", "base", "ASAP");
//    test_2("2", "minextend", "ASAP");
//    test_2("2", "minextend", "no");
//    test_2("2", "minextendrc", "no");

//    test_3("3", "base", "ASAP");
//    test_3("3", "minextend", "ASAP");
//    test_3("3", "minextend", "no");
//    test_3("3", "minextendrc", "no");

//    test_4("4", "base", "ASAP");
//    test_4("4", "minextend", "ASAP");
    test_4("4", "minextend", "no");
    test_4("4", "minextendrc", "no");

    return 0;
}
